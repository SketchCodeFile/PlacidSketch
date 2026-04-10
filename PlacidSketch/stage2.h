/**
 * @file stage2.h
 * @brief Stage2: 监视器
 * 
 * Stage2 使用指纹过滤器检测稳定子流，进行计数器管理和重生判定
 */

#ifndef STAGE2_H
#define STAGE2_H

#include "para.h"
#include "stage3.h"
#include "MurmurHash3.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <numeric>
#include <string>
#include <vector>

// Stage2 桶结构
struct Stage2Bucket {
    std::array<uint8_t, SUBFLOW_WINDOWS + 1> cx{};
    uint8_t initialized_flags = 0;
    uint8_t ck1 : 3;
    uint8_t ck2 : 3;
    uint8_t ck1_is_null : 1;
    uint8_t ck2_is_null : 1;

    Stage2Bucket() : ck1(1), ck2(1), ck1_is_null(false), ck2_is_null(false) {
        cx.fill(0);
    }

    bool empty() const { return initialized_flags == 0; }

    bool isCounterNull(uint8_t index) const {
        return !(initialized_flags & (1U << index));
    }

    void reset() {
        cx.fill(0);
        initialized_flags = 0;
        ck1 = 1;
        ck2 = 1;
        ck1_is_null = false;
        ck2_is_null = false;
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
            if (ck1 > 2) return false;
            else if (ck1 == 2) return (cx1 + base - cx2) <= ALPHA_THRESHOLD;
            else if (ck1 == 1) return std::abs(int(cx2) - int(cx1)) <= ALPHA_THRESHOLD;
            else return (cx2 + base - cx1) <= ALPHA_THRESHOLD;
        } else {
            if (ck2_is_null) return false;
            if (ck2 > 2) return false;
            else if (ck2 == 2) return (cx1 + base - cx2) <= ALPHA_THRESHOLD;
            else if (ck2 == 1) return std::abs(int(cx2) - int(cx1)) <= ALPHA_THRESHOLD;
            else return (cx1 + base - cx2) <= ALPHA_THRESHOLD;
        }
    }
};

// Stage2Monitor: 大小为 r × z 的二维数组
class Stage2Monitor {
private:
    std::vector<std::vector<Stage2Bucket>> buckets;
    std::vector<uint32_t> rowSeeds;
    uint32_t hashSeed;
    Stage3Merger& stage3;
    size_t rows = 0;
    size_t bucketsPerRow = 0;

    struct SelectedBucket {
        Stage2Bucket* bucket;
        uint32_t rowIdx;
        uint32_t colIdx;
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

    static float calculateVarianceFromArray(const float* data, size_t count) {
        float mean = data[0];
        float m2 = 0.0f;
        for (size_t i = 1; i < count; ++i) {
            float delta = data[i] - mean;
            mean += delta / (i + 1);
            float delta2 = data[i] - mean;
            m2 += delta * delta2;
        }
        return m2 / (count - 1);
    }

    static float calculateMinVariance(const Stage2Bucket& bucket, uint32_t startWindow) {
        const uint32_t R = SUBFLOW_WINDOWS + 1;
        const uint32_t base = (1u << COUNTER_BITS);
        const uint32_t half = base >> 1;
        
        float directFreqs[SUBFLOW_WINDOWS];
        float offsetFreqs[SUBFLOW_WINDOWS];
        size_t directCount = 0;
        size_t offsetCount = 0;

        for (uint32_t i = 0; i < SUBFLOW_WINDOWS; ++i) {
            uint32_t windowIndex = (startWindow + i) % R;
            
            if (bucket.isCounterNull(static_cast<uint8_t>(windowIndex))) {
                directCount = 0;
                break;
            }
            
            uint8_t counter = bucket.cx[windowIndex];
            directFreqs[directCount] = static_cast<float>(counter);
            directCount++;
            
            uint32_t v = static_cast<uint32_t>(counter);
            uint32_t adj = (v + half) % base;
            offsetFreqs[offsetCount] = static_cast<float>(adj);
            offsetCount++;
        }

        float varDirect = std::numeric_limits<float>::infinity();
        if (directCount == SUBFLOW_WINDOWS) {
            varDirect = calculateVarianceFromArray(directFreqs, directCount);
        }

        float varOffset = std::numeric_limits<float>::infinity();
        if (offsetCount >= 2) {
            varOffset = calculateVarianceFromArray(offsetFreqs, offsetCount);
        }

        if (varDirect == std::numeric_limits<float>::infinity()) {
            return varOffset;
        }
        if (varOffset == std::numeric_limits<float>::infinity()) {
            return varDirect;
        }
        return std::min(varDirect, varOffset);
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
    explicit Stage2Monitor(Stage3Merger& s3);
    ~Stage2Monitor();

    void processPotentialFlow(const char* flowID, uint32_t currentWindow);
};

#endif  // STAGE2_H