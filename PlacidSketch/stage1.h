/**
 * @file stage1.h
 * @brief Stage1: 快速过滤器
 * 
 * Stage1 检测候选稳定流，记录流的连续窗口到达情况
 */

#ifndef STAGE1_H
#define STAGE1_H

#include "parm.h"
#include "MurmurHash3.h"

#include <cstring>
#include <vector>

// Stage1 桶：快速记录流的连续性
struct Stage1Bucket {
    uint8_t continuity : 4;  // 连续窗口计数 (0-7)
    uint8_t arrival    : 1;  // 最近出现的窗口号 (0/1)
    uint8_t jump       : 1;  // 晋级标志 (0/1)

    Stage1Bucket() : continuity(0), arrival(0), jump(0) {}

    bool empty() const {
        return continuity == 0 && arrival == 0 && jump == 0;
    }

    void reset() {
        continuity = 0;
        arrival = 0;
        jump = 0;
    }
};

// Stage1: 检测候选稳定流
class Stage1Filter {
private:
    std::vector<std::vector<Stage1Bucket>> buckets;
    std::vector<uint32_t> hashSeeds;

public:
    explicit Stage1Filter();

    // 流到达，返回是否晋级 Stage2
    bool processPacket(const char* flowID, uint32_t windowSeq);

    // 重置在当前窗口未出现的桶
    void resetBuckets(uint32_t windowSeq);
};

#endif  // STAGE1_H