# PlacidSketch

PlacidSketch is a three-stage streaming algorithm for detecting stable flows in network traffic data. It uses a multi-stage filtering and monitoring approach to efficiently identify flows with stable frequency patterns.

## Overview

PlacidSketch consists of three main stages:

- **Stage 1 (Filter)**: Filters candidate stable flows using continuity-based detection
- **Stage 2 (Monitor)**: Monitors flows for stability using frequency variance analysis
- **Stage 3 (Merger)**: Merges stable subflows using incremental multi-segment merging technique

## Requirements

- C++17 or later
- CMake 3.26 or later
- A C++ compiler with C++17 support

## Building

```bash
mkdir build
cd build
cmake ..
make
```

Or use CMake directly:

```bash
cmake -B build
cmake --build build
```

## Usage

1. Prepare your input data as CSV files in a directory, where each CSV file represents a time window
2. Modify the `folderPath` variable in `main.cpp` to point to your data directory
3. Run the compiled executable

```bash
./main
```

## Configuration

Parameters can be modified in `parm.h`:

- `STAGE1_MEMORY_BYTES`: Memory allocation for Stage 1
- `STAGE2_MEMORY_BYTES`: Memory allocation for Stage 2
- `STAGE3_MEMORY_BYTES`: Memory allocation for Stage 3
- `SUBFLOW_WINDOWS`: Number of windows for stability detection
- `STABLE_THRESHOLD`: Variance threshold for stability

## Baselines

This project includes two baseline algorithms in the `baselines` directory for comparison:

### SteadySketch

**Location**: `baselines/SteadySketch/`

SteadySketch is a two-stage data structure for detecting stable flows in network traffic. It consists of:
- **Filter**: A filter array for preliminary candidate selection
- **Sketch**: A counting sketch for frequency estimation

**Building**:
```bash
cd baselines/SteadySketch
mkdir -p build && cd build
cmake ..
make
```

**Usage**:
1. Prepare the data file (binary format with flow ID and timestamp)
2. Modify `Data.open("", ios::binary)` in `main.cpp` to point to your data file
3. Adjust parameters in `parm.h` if needed
4. Run the compiled executable

### XSketch

**Location**: `baselines/XSKetch/`

XSketch is a two-stage Sketch data structure for network traffic anomaly detection:
- **Stage1 (TowerSketch)**: Quickly records frequency information of each flow using a multi-level bit-encoded counter array
- **Stage2 (Hash Bucket)**: Provides precise detection using linear regression to identify stable flows

**Building**:
```bash
cd baselines/XSKetch
mkdir -p build && cd build
cmake ..
make
```

**Usage**:
```bash
./XSketch <data_file_path> <memory_size(MB)> <run_length>
```

Parameters can be configured in `Param.h`:
- `var_thres_p`: Variance threshold for stability detection
- `error_thres_p`: Error threshold for linear regression residual
- `stage_ratio_p`: Memory ratio for Stage1
- `bucket_size_p`: Number of slots per hash bucket
- `S_p`: Number of windows for Stage1
- `potential_thres_p`: Potential threshold for flow replacement
