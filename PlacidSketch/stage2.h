#ifndef STAGE2_H
#define STAGE2_H

#include "parm.h"
#include "stage3.h"
#include "MurmurHash3.h"
#include <array>
#include <cstring>
#include <numeric>
#include <vector>

struct Stage2Bucket {
    std::array<uint8_t, SUBFLOW_WINDOWS + 1> cx{};
    uint8_t initialized_flags;
    uint8_t ck1;
    uint8_t ck2;
    uint8_t ck1_is_null : 1;
    uint8_t ck2_is_null : 1;

    Stage2Bucket() : ck1(1), ck2(1), ck1_is_null(0), ck2_is_null(0), initialized_flags(0) {
        cx.fill(0);
    }

    bool empty() const { return initialized_flags == 0; }

    bool isCounterNull(uint8_t index) const {
        return (initialized_flags & (1U << index)) == 0;
    }

    void reset() {
        cx.fill(0);
        initialized_flags = 0;
        ck1 = 1;
        ck2 = 1;
        ck1_is_null = 0;
        ck2_is_null = 0;
    }

    uint32_t countWindowNumber() {
        uint32_t num = 0;
        for (uint32_t i = 0; i < SUBFLOW_WINDOWS + 1; i++) {
            if (initialized_flags & (1U << i)) {
                num++;
            }
        }
        return num;
    }

    void initializeNewWindow(uint8_t window, uint32_t absoluteWindow) {
        cx[window] = 1;
        initialized_flags |= (1U << window);

        bool useCk1 = (absoluteWindow % 2 == 0);
        if (useCk1) {
            ck1 = 1;
            ck1_is_null = false;
        } else {
            ck2 = 1;
            ck2_is_null = false;
        }
    }

    bool checkStability(uint8_t y1, uint8_t y2, uint32_t absoluteWindow) const {
        const uint32_t base = (1u << COUNTER_BITS);
        const uint32_t cx1 = cx[y1];
        const uint32_t cx2 = cx[y2];

        bool useCk1 = (absoluteWindow % 2 == 0);
        if (isCounterNull(y1) || isCounterNull(y2)) {
            return false;
        }
        if (useCk1) {
            if (ck1_is_null) return false;
            if (ck1 == 0) {
                return (cx2 + base - cx1) <= ALPHA_THRESHOLD;
            } else if (ck1 == 1) {
                return static_cast<uint32_t>(abs(int(cx2) - int(cx1))) <= ALPHA_THRESHOLD;
            } else if (ck1 == 2) {
                return (cx1 + base - cx2) <= ALPHA_THRESHOLD;
            }
            return false;
        } else {
            if (ck2_is_null) return false;
            if (ck2 == 0) {
                return (cx1 + base - cx2) <= ALPHA_THRESHOLD;
            } else if (ck2 == 1) {
                return static_cast<uint32_t>(abs(int(cx2) - int(cx1))) <= ALPHA_THRESHOLD;
            } else if (ck2 == 2) {
                return (cx1 + base - cx2) <= ALPHA_THRESHOLD;
            }
            return false;
        }
    }
};

class Stage2Monitor {
private:
    std::vector<std::vector<Stage2Bucket>> buckets;
    std::vector<uint32_t> rowSeeds;
    uint32_t hashSeed;
    Stage3Merger& stage3;
    size_t rows = 0;
    size_t bucketsPerRow = 0;
    std::vector<uint16_t>* detectedFlowsCallback;

    struct SelectedBucket {
        Stage2Bucket* bucket;
    };

    static inline bool isPowerOfTwo(uint32_t v) {
        return v && ((v & (v - 1)) == 0);
    }

    static inline uint32_t fastReduce(uint32_t hash, uint32_t range) {
        return static_cast<uint32_t>((static_cast<uint64_t>(hash) * static_cast<uint64_t>(range)) >> 32);
    }

    inline uint32_t computeRowHash(const char* flowID, uint32_t row) const {
        uint32_t h;
        MurmurHash3_x86_32(flowID, KEY_LEN, rowSeeds[row], &h);
        h ^= (row * 0x9e3779b9u) ^ 0x85ebca6bu;
        return h;
    }

    inline uint32_t indexForRow(const char* flowID, uint32_t row, uint32_t range) const {
        uint32_t h = computeRowHash(flowID, row);
        if (isPowerOfTwo(range)) {
            return h & (range - 1);
        } else {
            return fastReduce(h, range);
        }
    }

    static uint32_t calculateConsecutiveWindows(const Stage2Bucket& bucket, uint32_t currentWindow) {
        const uint32_t R = SUBFLOW_WINDOWS + 1;
        uint32_t count = 0;
        for (uint32_t i = 1; i <= SUBFLOW_WINDOWS; ++i) {
            if (currentWindow < i) break;
            uint32_t window = currentWindow - i;
            uint8_t y = window % R;
            if (bucket.isCounterNull(y)) break;
            count++;
        }
        return count;
    }

    static float calculateVariance(const std::vector<float>& data) {
        if (data.size() < 2) {
            return std::numeric_limits<float>::infinity();
        }

        float sum = std::accumulate(data.begin(), data.end(), 0.0f);
        float mean = sum / data.size();

        float variance = 0.0f;
        for (float val : data) {
            variance += (val - mean) * (val - mean);
        }
        return variance / (data.size() - 1);
    }

    static float calculateDirectVariance(const Stage2Bucket& bucket, uint32_t startWindow) {
        const uint32_t R = SUBFLOW_WINDOWS + 1;
        std::vector<float> directFreqs;
        directFreqs.reserve(SUBFLOW_WINDOWS);

        for (uint32_t i = 0; i < SUBFLOW_WINDOWS; ++i) {
            uint32_t windowIndex = (startWindow + i) % R;
            if (bucket.isCounterNull(static_cast<uint8_t>(windowIndex))) {
                return std::numeric_limits<float>::infinity();
            }
            uint8_t counter = bucket.cx[windowIndex];
            directFreqs.push_back(static_cast<float>(counter));
        }

        if (directFreqs.size() < 2) {
            return std::numeric_limits<float>::infinity();
        }
        return calculateVariance(directFreqs);
    }

    static float calculateOffsetVariance(const Stage2Bucket& bucket, uint32_t startWindow) {
        const uint32_t R = SUBFLOW_WINDOWS + 1;
        const uint32_t base = (1u << COUNTER_BITS);
        const uint32_t half = base >> 1;
        std::vector<float> offsetFreqs;
        offsetFreqs.reserve(SUBFLOW_WINDOWS);

        for (uint32_t i = 0; i < SUBFLOW_WINDOWS; ++i) {
            uint32_t windowIndex = (startWindow + i) % R;
            if (bucket.isCounterNull(static_cast<uint8_t>(windowIndex))) break;
            uint32_t v = bucket.cx[windowIndex];
            uint32_t adj = (v + half) % base;
            offsetFreqs.push_back(static_cast<float>(adj));
        }
        
        if (offsetFreqs.size() < 2) {
            return std::numeric_limits<float>::infinity();
        }
        return calculateVariance(offsetFreqs);
    }

    static float calculateMeanFrequency(const Stage2Bucket& bucket, uint32_t startWindow) {
        const uint32_t R = SUBFLOW_WINDOWS + 1;
        float sum = 0.0f;
        uint32_t count = 0;

        for (uint32_t i = 0; i < SUBFLOW_WINDOWS; ++i) {
            uint32_t windowIndex = (startWindow + i) % R;
            if (bucket.isCounterNull(static_cast<uint8_t>(windowIndex))) {
                return std::numeric_limits<float>::infinity();
            }
            sum += static_cast<float>(bucket.cx[windowIndex]);
            count++;
        }

        return count > 0 ? sum / count : 0.0f;
    }

    static bool updateCKOnRebirth(Stage2Bucket* bucket, uint32_t absoluteWindow) {
        bool useCk1 = (absoluteWindow % 2 == 0);

        if (useCk1) {
            if (!bucket->ck1_is_null && bucket->ck1 < 6) {
                bucket->ck1++;
            }
            uint32_t windowNum = bucket->countWindowNumber();
            if (windowNum != 1) {
                if (!bucket->ck2_is_null) {
                    if (bucket->ck2 != 0) {
                        bucket->ck2--;
                    } else {
                        bucket->ck2_is_null = true;
                        return false;
                    }
                } else {
                    return false;
                }
            }
        } else {
            if (!bucket->ck2_is_null && bucket->ck2 < 6) {
                bucket->ck2++;
            }
            uint32_t windowNum = bucket->countWindowNumber();
            if (windowNum != 1) {
                if (!bucket->ck1_is_null) {
                    if (bucket->ck1 != 0) {
                        bucket->ck1--;
                    } else {
                        bucket->ck1_is_null = true;
                        return false;
                    }
                } else {
                    return false;
                }
            }
        }
        return true;
    }

public:
    explicit Stage2Monitor(Stage3Merger& s3, size_t memoryBytes = STAGE2_MEMORY_BYTES) 
        : hashSeed(0x200), stage3(s3), detectedFlowsCallback(nullptr) {
        rows = STAGE2_ROWS;
        size_t bucketSize = sizeof(Stage2Bucket);
        size_t perRowBytes = (rows > 0) ? (memoryBytes / rows) : 0;
        bucketsPerRow = (bucketSize > 0) ? std::max<size_t>(1, perRowBytes / bucketSize) : 1;

        buckets.resize(rows);
        for (size_t i = 0; i < rows; ++i) {
            buckets[i].resize(bucketsPerRow);
        }
        rowSeeds.resize(rows);

        for (size_t i = 0; i < rows; ++i) {
            rowSeeds[i] = hashSeed + static_cast<uint32_t>(i);
        }
    }

    void setDetectedFlowsCallback(std::vector<uint16_t>* callback) {
        detectedFlowsCallback = callback;
    }

    void recordDetectedFlow(const char* flowID) {
        if (detectedFlowsCallback) {
            uint16_t flowID16;
            memcpy(&flowID16, flowID, sizeof(uint16_t));
            detectedFlowsCallback->push_back(flowID16);
        }
    }

    void processPotentialFlow(const char* flowID, uint32_t currentWindow) {
        const uint32_t R = SUBFLOW_WINDOWS + 1;
        const uint8_t y_current = currentWindow % R;

        uint8_t y_prev = 0;
        uint8_t y_prev_prev = 0;
        bool hasPrevWindows = false;

        if (currentWindow >= 2) {
            y_prev = (currentWindow - 1) % R;
            y_prev_prev = (currentWindow - 2) % R;
            hasPrevWindows = true;
        } else if (currentWindow == 1) {
            y_prev = (currentWindow - 1) % R;
            hasPrevWindows = true;
        }

        std::vector<SelectedBucket> selected;

        for (uint32_t f = 0; f < rows; ++f) {
            uint32_t k = indexForRow(flowID, f, static_cast<uint32_t>(bucketsPerRow));
            Stage2Bucket* cell = &buckets[f][k];
            selected.push_back({cell});
        }

        bool hasEmpty = false;
        bool hasCurrentNull = false;

        for (auto& sel : selected) {
            if (!hasEmpty && sel.bucket->empty()) {
                hasEmpty = true;
            }
            if (!hasCurrentNull && sel.bucket->isCounterNull(y_current)) {
                hasCurrentNull = true;
            }
        }

        if (hasEmpty) {
            for (auto& sel : selected) {
                if (sel.bucket->empty()) {
                    sel.bucket->initializeNewWindow(y_current, currentWindow);
                }
            }
            return;
        }

        if (!hasCurrentNull) {
            for (auto& sel : selected) {
                Stage2Bucket* bucketPtr = sel.bucket;
                if (bucketPtr->cx[y_current] < ((1u << COUNTER_BITS) - 1)) {
                    bucketPtr->cx[y_current]++;
                } else {
                    bucketPtr->cx[y_current] = 0;
                    if (!updateCKOnRebirth(bucketPtr, currentWindow)) {
                        bucketPtr->reset();
                        bucketPtr->initializeNewWindow(y_current, currentWindow);
                    }
                }
            }
            return;
        }

        std::vector<Stage2Bucket*> nullBuckets;
        for (auto& sel : selected) {
            if (sel.bucket->isCounterNull(y_current)) {
                nullBuckets.push_back(sel.bucket);
            }
        }
        if (nullBuckets.empty()) return;

        bool havepassed = false;

        for (auto* b : nullBuckets) {
            uint32_t windowNum = b->countWindowNumber();

            if (!hasPrevWindows) {
                b->reset();
                b->initializeNewWindow(y_current, currentWindow);
                continue;
            }

            if (windowNum > 2 && (b->isCounterNull(y_prev) || b->isCounterNull(y_prev_prev))) {
                b->reset();
                b->initializeNewWindow(y_current, currentWindow);
            } else if (windowNum <= 2 && b->isCounterNull(y_prev)) {
                b->reset();
                b->initializeNewWindow(y_current, currentWindow);
            } else if (windowNum <= 2 && !b->isCounterNull(y_prev)) {
                b->initializeNewWindow(y_current, currentWindow);
            } else {
                if (!b->checkStability(y_prev, y_prev_prev, currentWindow)) {
                    b->reset();
                    b->initializeNewWindow(y_current, currentWindow);
                } else {
                    uint32_t consecutiveWindows = calculateConsecutiveWindows(*b, currentWindow);
                    if (consecutiveWindows == SUBFLOW_WINDOWS) {
                        if (!havepassed) {
                            uint32_t w = currentWindow - SUBFLOW_WINDOWS;
                            float meanFreq = calculateMeanFrequency(*b, w);
                            float varDirect = calculateDirectVariance(*b, w);
                            float varOffset = calculateOffsetVariance(*b, w);
                            float variance = std::min(varDirect, varOffset);

                            if (variance <= STABLE_THRESHOLD) {
                                uint16_t flowID16;
                                memcpy(&flowID16, flowID, sizeof(uint16_t));
                                stage3.processSteadySubflow(flowID16, w, meanFreq, variance);
                                recordDetectedFlow(flowID);
                                havepassed = true;
                            }
                        }

                        b->reset();
                        b->initializeNewWindow(y_current, currentWindow);
                    } else {
                        b->initializeNewWindow(y_current, currentWindow);
                    }
                }
            }
        }
    }
};

#endif
