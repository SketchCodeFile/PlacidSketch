/**
 * @file parm.h
 * @brief 全局参数配置头文件
 * 
 * 定义了所有Sketch算法共享的配置参数，包括：
 * - Stage1, Stage2, Stage3 的内存配置
 * - 算法参数（窗口大小、计数器位数等）
 * - 数据包结构定义
 */

#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <cstdint>
#include <cstring>

// ============== 全局参数配置 ==============

// 指纹长度（16bit指纹，如"3199"或"3199b26a" + null终止符）
constexpr int KEY_LEN = 16;

// 五元组长度（用于评估准确率和召回率）
constexpr int KEY_LEN1 = 128;

// Stage1 和 Stage2 共享内存配置
// 运行时配置变量（可以在运行时修改）
inline size_t STAGE1_STAGE2_TOTAL_MEMORY_BYTES = 600ull * 1024;

// Stage1 占 Stage1+Stage2 总内存的百分比（例如0.2表示20%）
inline double r = 0.2;

// Stage1 配置（根据总内存和比例r自动计算）
inline size_t get_STAGE1_MEMORY_BYTES() { 
    return static_cast<size_t>(STAGE1_STAGE2_TOTAL_MEMORY_BYTES * r); 
}
constexpr int STAGE1_ROWS = 3;  // 哈希行数

// Stage2 配置（根据总内存和比例自动计算）
inline size_t get_STAGE2_MEMORY_BYTES() { 
    return STAGE1_STAGE2_TOTAL_MEMORY_BYTES - get_STAGE1_MEMORY_BYTES(); 
}
constexpr int STAGE2_ROWS = 2;  // 哈希行数

// Stage3 配置（固定内存）
constexpr size_t STAGE3_MEMORY_BYTES = 200ull * 1024;
constexpr int STAGE3_BUCKETS = 4;  // 桶数

// ============== 算法参数 ==============

// 滑动窗口大小
constexpr int SUBFLOW_WINDOWS = 5;

// 计数器位数
constexpr int COUNTER_BITS = 8;

// 相对重生差值的阈值
constexpr int ALPHA_THRESHOLD = 10;

// 最小子流数
constexpr int MIN_SUBFLOWS = 5;

// 基准方案稳定流
constexpr int Steady_FLOWS = 200;

// 最大合并次数
constexpr int P = 400;

// 当合并段数大于Q时才会报告，否则淘汰
constexpr int Q = 40;

// 稳定阈值（方差）
constexpr float STABLE_THRESHOLD = 5.0f;

// ============== SteadySketch 参数配置 ==============

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
    
    // steady_filter 和 rolling_sketch 的总内存配置
    inline size_t steady_filter_rolling_sketch_total_memory_kb = 600;
    
    // 参数r：steady_filter 占二者总内存的百分比（例如0.8表示80%）
    inline double r = 0.8;
    
    // 根据总内存和比例r自动计算 steady_filter 和 rolling_sketch 的内存
    inline size_t get_steady_filter_memory_kb() { 
        return static_cast<size_t>(steady_filter_rolling_sketch_total_memory_kb * r); 
    }
    inline size_t get_rolling_sketch_memory_kb() { 
        return steady_filter_rolling_sketch_total_memory_kb - get_steady_filter_memory_kb(); 
    }
    
    constexpr size_t uss_memory_kb = 2000;  // USS (HeavyHitter) 内存限制（KB）
    
    inline int get_memory_size() { 
        return static_cast<int>(steady_filter_rolling_sketch_total_memory_kb); 
    }
    inline double get_prop() { return r; }
}  // namespace steady

// ============== XSketch 参数配置 ==============

namespace xsketch {
    constexpr int K = 2;                    // 多项式拟合的阶数
    constexpr int bucket_size_p = 4;        // Stage2 桶大小
    constexpr double error_thres_p = 0.1;   // 误差阈值
    constexpr double potential_thres_p = 0.0001;  // 潜在流阈值
    constexpr int window_size = 1;          // 窗口大小
}  // namespace xsketch

// ============== 数据包结构 ==============

/**
 * @brief 数据包结构
 * 用于在各个阶段之间传递数据
 */
struct Packet {
    char flowID[KEY_LEN];        // 指纹（用于三个阶段处理）
    char quintuple[KEY_LEN1];    // 五元组（用于评估准确率和召回率）
    uint32_t windowNumber;        // 当前包对应的窗口号

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
    
    // 向后兼容：只提供指纹时，五元组为空
    Packet(const char* id, uint32_t window) {
        strncpy(flowID, id, KEY_LEN);
        flowID[KEY_LEN - 1] = '\0';
        memset(quintuple, 0, KEY_LEN1);
        windowNumber = window;
    }
};

#endif  // PARAMETERS_H