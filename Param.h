#ifndef XSKETCH_PARAM_H
#define XSKETCH_PARAM_H

// XSketch 全局参数配置
namespace xsketch {
    // 滑动窗口大小（与PlacidSketch的SUBFLOW_WINDOWS一致）
    constexpr int S = 5;
    
    // 最大合并次数（与PlacidSketch的P一致）
    constexpr int P = 400;
    
    // 多项式拟合的阶数
    constexpr int K = 2;
    
    // Stage2 桶大小
    constexpr int bucket_size_p = 4;
    
    // 方差阈值
    constexpr double var_thres_p = 5.0;
    
    // 误差阈值
    constexpr double error_thres_p = 0.1;
    
    // Stage1 内存占比
    constexpr double stage_ratio_p = 0.2;
    
    // 潜在流阈值
    constexpr double potential_thres_p = 0.0001;
    
    // 窗口大小
    constexpr int window_size = 1;
}  // namespace xsketch

#endif  // XSKETCH_PARAM_H