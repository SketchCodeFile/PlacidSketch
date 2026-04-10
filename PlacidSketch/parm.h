
#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <cstdint>
#include <cstring>

constexpr int KEY_LEN = 16;

constexpr int KEY_LEN1 = 128;

inline size_t STAGE1_STAGE2_TOTAL_MEMORY_BYTES = 600ull * 1024;

inline double r = 0.2;

inline size_t get_STAGE1_MEMORY_BYTES() { 
    return static_cast<size_t>(STAGE1_STAGE2_TOTAL_MEMORY_BYTES * r); 
}
constexpr int STAGE1_ROWS = 3;  

inline size_t get_STAGE2_MEMORY_BYTES() { 
    return STAGE1_STAGE2_TOTAL_MEMORY_BYTES - get_STAGE1_MEMORY_BYTES(); 
}
constexpr int STAGE2_ROWS = 2; 

constexpr size_t STAGE3_MEMORY_BYTES = 200ull * 1024;
constexpr int STAGE3_BUCKETS = 4; 

constexpr int SUBFLOW_WINDOWS = 5;

constexpr int COUNTER_BITS = 8;

constexpr int ALPHA_THRESHOLD = 10;

constexpr int MIN_SUBFLOWS = 5;

constexpr int Steady_FLOWS = 200;

constexpr int P = 400;

constexpr int Q = 40;

constexpr float STABLE_THRESHOLD = 5.0f;

namespace steady {
    constexpr int _Period = 5;
    constexpr int StableThreshold = 5;
    constexpr int SmoothThreshold = 200;
    constexpr int ElementPerBucket = 4;
    constexpr int _SketchArray = 3;
    constexpr int _FilterArray = 2;
    constexpr int _NumOfHeavyHitterBuckets = 2000;
    constexpr int TopK = 100000;
    constexpr unsigned int DefaultHashSeed = 0x100;
 
    inline size_t steady_filter_rolling_sketch_total_memory_kb = 300;
 
    inline double r = 0.8;
   
    inline size_t get_steady_filter_memory_kb() { 
        return static_cast<size_t>(steady_filter_rolling_sketch_total_memory_kb * r); 
    }
    inline size_t get_rolling_sketch_memory_kb() { 
        return steady_filter_rolling_sketch_total_memory_kb - get_steady_filter_memory_kb(); 
    }
    
    constexpr size_t uss_memory_kb = 240;  
    
    inline int get_memory_size() { 
        return static_cast<int>(steady_filter_rolling_sketch_total_memory_kb); 
    }
    inline double get_prop() { return r; }
}  // namespace steady

namespace xsketch {
    constexpr int K = 0;                   
    constexpr int bucket_size_p = 4;        
    constexpr double error_thres_p = 0.1;   
    constexpr double potential_thres_p = 0.01;  
    constexpr int window_size = 200;         
}  // namespace xsketch



struct Packet {
    char flowID[KEY_LEN];       
    char quintuple[KEY_LEN1];   
    uint32_t windowNumber;        

    Packet() {
        memset(flowID, 0, KEY_LEN);
        memset(quintuple, 0, KEY_LEN1);
        windowNumber = 0;
    }

    Packet(const char* fingerprint, const char* quintupleStr, uint32_t window) {
        strncpy(flowID, fingerprint, KEY_LEN);
        flowID[KEY_LEN - 1] = '\0';
        if (quintupleStr) {
            strncpy(quintuple, quintupleStr, KEY_LEN1);
            quintuple[KEY_LEN1 - 1] = '\0';
        } else {
            memset(quintuple, 0, KEY_LEN1);
        }
        windowNumber = window;
    }
    

    Packet(const char* id, uint32_t window) {
        strncpy(flowID, id, KEY_LEN);
        flowID[KEY_LEN - 1] = '\0';
        memset(quintuple, 0, KEY_LEN1);
        windowNumber = window;
    }
};

#endif  // PARAMETERS_H
