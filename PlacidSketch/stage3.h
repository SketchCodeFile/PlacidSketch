
#ifndef STAGE3_H
#define STAGE3_H

#include "parm.h"
#include "MurmurHash3.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <random>
#include <set>
#include <string>
#include <vector>

struct Statistics {
    float mean = 0.0f;
    float variance = 0.0f;
    Statistics() = default;
    Statistics(float m, float v) : mean(m), variance(v) {}
};

struct Stage3Cell {
    char ID[KEY_LEN]{};
    uint16_t window = 0;
    Statistics s;
    uint16_t number = 0;

    bool empty() const { return ID[0] == 0; }
    void clear() {
        std::fill(std::begin(ID), std::end(ID), 0);
        window = 0;
        s = Statistics();
        number = 0;
    }
};

struct StableFlowRecord {
    std::string fingerprint;
    uint32_t startWindow;
    uint32_t endWindow;
    uint16_t subflowCount;
    float variance;
    float mean;
};

class Stage3Merger {
private:
    std::vector<std::vector<Stage3Cell>> buckets;
    uint32_t hashSeed;
    std::mt19937 gen;
    std::uniform_real_distribution<float> dist;
    std::set<std::string> stableFlowIDs;
    size_t totalStableIntervals = 0;
    size_t l = 0;
    size_t b = 0;
    std::vector<StableFlowRecord> stableRecords;

    static bool canMergeVariance(const Stage3Cell& cell, float newVar, float newMean);

    void clearCell(Stage3Cell& cell);
    static void initNewCell(Stage3Cell& cell, const char* flowID, uint32_t startW, float var, float mean);
    static void mergeCell(Stage3Cell& cell, float newVar, float newMean);

public:
    explicit Stage3Merger();
    ~Stage3Merger();

    void processSteadySubflow(const char* flowID, uint32_t startW, float var, float mean);
    void finalize();

    size_t getStableFlowCount() const;
    size_t getStableIntervalCount() const;
    const std::set<std::string>& getStableFlowIDs() const;
    const std::vector<StableFlowRecord>& getStableFlowRecords() const;
};

#endif  // STAGE3_H
