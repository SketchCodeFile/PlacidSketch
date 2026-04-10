
#ifndef STAGE1_H
#define STAGE1_H

#include "parm.h"
#include "MurmurHash3.h"

#include <cstring>
#include <vector>

struct Stage1Bucket {
    uint8_t continuity : 4;  
    uint8_t arrival    : 1;  
    uint8_t jump       : 1;  

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

class Stage1Filter {
private:
    std::vector<std::vector<Stage1Bucket>> buckets;
    std::vector<uint32_t> hashSeeds;

public:
    explicit Stage1Filter();

    bool processPacket(const char* flowID, uint32_t windowSeq);

    void resetBuckets(uint32_t windowSeq);
};

#endif  // STAGE1_H
