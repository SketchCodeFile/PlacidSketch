# PlacidSketch

A high-performance stable flow detection algorithm framework implementing multiple sketch-based algorithms for network traffic analysis.

## Overview

PlacidSketch is a comprehensive framework for detecting stable network flows using advanced sketch data structures. It integrates multiple cutting-edge algorithms to provide accurate, memory-efficient stable flow detection.

## Algorithms

This framework implements the following algorithms:

### 1. PlacidSketch
A three-stage stable flow detection algorithm consisting of:
- **Stage1**: Fast candidate filter using multi-row hash tables
- **Stage2**: Fingerprint-based monitor for stable subflow detection
- **Stage3**: Stable subflow merger for complete stable flow reporting

### 2. SteadySketch
A variance-based stable flow identification algorithm using:
- RBF (Random Bloom Filter) for flow tracking
- GS (Group Sketch) for frequency monitoring
- USS (Unique Stable Snapshot) for stable flow detection

### 3. XSketch (Optional)
A polynomial fitting-based algorithm for stable flow detection, using:
- Tower-based Stage1 for efficient candidate filtering
- Linear regression for trend analysis

### 4. Ground Truth Baseline
Reference implementation for accuracy comparison.

## Directory Structure

```
PlacidSketch/
├── CMakeLists.txt           # CMake build configuration
├── README.md                # This file
├── LICENSE                  # License file
├── .gitignore               # Git ignore rules
├── include/                 # Public headers
│   ├── para.h               # Global parameters
│   ├── stage1.h             # Stage1 header
│   ├── stage2.h             # Stage2 header
│   ├── stage3.h             # Stage3 header
│   ├── SteadySketch.h       # SteadySketch header
│   ├── ground_truth_baseline.h
│   └── MurmurHash3.h        # Hash functions
├── src/                     # Source files
│   ├── main.cpp             # Main program
│   ├── stage1.cpp           # Stage1 implementation
│   ├── stage2.cpp           # Stage2 implementation
│   ├── stage3.cpp           # Stage3 implementation
│   ├── SteadySketch.cpp     # SteadySketch implementation
│   ├── ground_truth_baseline.cpp
│   └── MurmurHash3.cpp      # Hash implementation
└── XSketch/                 # XSketch algorithm
    ├── XSketch.h            # XSketch header
    ├── Param.h              # XSketch parameters
    ├── CorrectDetector.h    # Regression functions
    └── Common/
        └── hash.h           # Hash utilities
```

## Building

### Requirements

- CMake 3.15 or higher
- C++17 compatible compiler (GCC, Clang, MSVC)

### Build Steps

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
cmake --build . --config Release
```

### Build Options

```bash
# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build with optimizations
cmake -DCMAKE_BUILD_TYPE=Release ..
```

## Usage

### Input Format

The program expects CSV files with the following format:

```
five_tuple,flow_id
192.168.1.1:80-192.168.1.2:50000-6,3199
192.168.1.2:80-192.168.1.3:50001-6,3199b26a
...
```

Each CSV file represents one time window, and all CSV files should be placed in a single directory.

### Running the Program

After building, run:

```bash
./bin/placidsketch
```

**Note**: You may need to modify the data path in `main.cpp` to point to your dataset location.

### Configuration

Key parameters can be modified in `include/para.h`:

```cpp
// Memory configuration
inline size_t STAGE1_STAGE2_TOTAL_MEMORY_BYTES = 600 * 1024;  // Total memory

// Stage ratio (Stage1 memory / Total memory)
inline double r = 0.2;

// Algorithm parameters
constexpr int SUBFLOW_WINDOWS = 5;        // Sliding window size
constexpr float STABLE_THRESHOLD = 5.0f;  // Variance threshold
```

## Algorithm Comparison Results

The framework automatically compares multiple algorithms across different memory configurations:

| Memory (KB) | Algorithm | Precision | Recall | F1 Score |
|-------------|-----------|-----------|--------|----------|
| 200         | PlacidSketch | - | - | - |
| 200         | SteadySketch | - | - | - |
| 400         | PlacidSketch | - | - | - |
| 400         | SteadySketch | - | - | - |
| 600         | PlacidSketch | - | - | - |
| 600         | SteadySketch | - | - | - |

## Performance Characteristics

### Memory Efficiency
- All algorithms are designed for memory-constrained environments
- Configurable memory allocation for different stages
- Memory sharing between related components

### Time Complexity
- **Stage1/Stage2**: O(d) where d is the number of hash rows
- **Stage3**: O(b) where b is the number of cells per bucket
- **Overall**: Near-linear time complexity with constant memory

### Accuracy
- PlacidSketch: High precision with configurable thresholds
- SteadySketch: Excellent recall with variance-based detection
- XSketch: Alternative approach using polynomial fitting

## Contributing

Contributions are welcome. Please feel free to submit pull requests or create issues for bugs and feature requests.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## References

If you use this software in your research, please cite:

```
PlacidSketch: A High-Performance Stable Flow Detection Framework
```

## Contact

For questions or suggestions, please open an issue on GitHub.