/**
 * @file SteadySketch.h
 * @brief SteadySketch 算法实现
 * 
 * 基于方差检测的稳定流识别算法
 */

#ifndef STEADY_SKETCH_H
#define STEADY_SKETCH_H

#include "parm.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace steady_internal {
    inline unsigned char minByte(unsigned char a, unsigned char b) {
        return static_cast<unsigned char>(std::min(a, b));
    }

    inline uint64_t MurmurHash64B(const void* key, int len, unsigned int seed) {
        constexpr unsigned int m = 0x5bd1e995;
        constexpr int r = 24;
        unsigned int h1 = seed ^ static_cast<unsigned int>(len);
        unsigned int h2 = 0;
        const unsigned int* data = static_cast<const unsigned int*>(key);

        while (len >= 8) {
            unsigned int k1 = *data++;
            k1 *= m; k1 ^= k1 >> r; k1 *= m;
            h1 *= m; h1 ^= k1;
            len -= 4;

            unsigned int k2 = *data++;
            k2 *= m; k2 ^= k2 >> r; k2 *= m;
            h2 *= m; h2 ^= k2;
            len -= 4;
        }

        if (len >= 4) {
            unsigned int k1 = *data++;
            k1 *= m; k1 ^= k1 >> r; k1 *= m;
            h1 *= m; h1 ^= k1;
            len -= 4;
        }

        const unsigned char* remaining = reinterpret_cast<const unsigned char*>(data);
        switch (len) {
            case 3: h2 ^= remaining[2] << 16;
            [[fallthrough]];
            case 2: h2 ^= remaining[1] << 8;
            [[fallthrough]];
            case 1: h2 ^= remaining[0]; h2 *= m; break;
            default: break;
        }

        h1 ^= h2 >> 18; h1 *= m;
        h2 ^= h1 >> 22; h2 *= m;
        h1 ^= h2 >> 17; h1 *= m;
        h2 ^= h1 >> 19; h2 *= m;

        uint64_t h = h1;
        h = (h << 32) | h2;
        return h;
    }
}  // namespace steady_internal

class SteadySketch {
public:
    SteadySketch(unsigned int hashSeed = steady::DefaultHashSeed);
    SteadySketch(int sketchArrays, int sketchLength, int rbfLength, int rbfArrays,
                 unsigned int hashSeed = steady::DefaultHashSeed);
    ~SteadySketch();

    void Insert(const char* flowID, uint32_t window, const char* quintuple = nullptr);
    void Flush();
    void ReportValue(double throughputKpps);
    void WriteReportToCsv(const std::string& filepath) const;

    std::multimap<std::string, std::pair<int, std::tuple<int, int, int>>> TopKReport;
    double RBF_size = 0.0;
    double GS_size = 0.0;
    double USS_size = 0.0;

    std::set<std::string> convertFingerprintsToQuintuples(
            const std::set<std::string>& fingerprints) const;

private:
    static constexpr unsigned WINDOW_SLOTS = steady::_Period + 2;
    static constexpr unsigned RBF_ALPHA = 10;
    static constexpr unsigned GS_ALPHA = 10;
    static constexpr unsigned GROUP_HISTORY = sizeof(short) << 3;
    static constexpr unsigned long long RBF_MOD = 0x0001000100010001ull;

    struct Item {
        std::array<char, KEY_LEN> id{};
        int window = 0;
    };

    struct ReportRecord {
        std::string itemID;
        int startWindow = 0;
        int endWindow = 0;
        int variance = 0;
    };

    struct StableElement {
        int startStableTime = 0;
        int recentStableTime = 0;
        std::array<char, KEY_LEN> itemID{};
        int variance = 0;

        bool empty() const { return itemID[0] == '\0'; }
        void setID(const std::array<char, KEY_LEN>& source) {
            itemID = source;
        }
        void clear() {
            startStableTime = 0;
            recentStableTime = 0;
            variance = 0;
            itemID.fill('\0');
        }
    };

    void insertInternal(const Item& item);
    void reportIfSmooth(StableElement& element);
    void pushReport(const StableElement& element);
    void addReport(std::string id, int start, int end, int variance);
    void updateFingerprintMapping(const char* fingerprint, const char* quintuple);
    static bool compareRecords(const ReportRecord& lhs, const ReportRecord& rhs);

    short* RBF = nullptr;
    unsigned char* GS = nullptr;
    StableElement** HeavyHitter = nullptr;

    int d = 0;
    int L = 0;
    int RBFNum = 0;
    int RBFL = 0;
    unsigned int hashSeed = steady::DefaultHashSeed;
    unsigned int timeStamp = 0;
    int lastProcessedWindow = -1;
    int numOfHeavyHitterBuckets = 0;
    int elementsPerBucket = 0;

    std::vector<ReportRecord> reportBuffer;
    std::map<std::string, std::string> fingerprintToQuintuple;
};

#endif  // STEADY_SKETCH_H