# PlacidSketch

A high-performance sketch algorithm for detecting stable flows in network traffic using a three-stage architecture.

## Overview

PlacidSketch is designed to identify stable network flows (flows that appear consistently across multiple time windows) with minimal memory usage. It uses a three-stage pipeline:

1. **Stage1 (Stage1Filter)**: Quick candidate selection based on flow continuity
2. **Stage2 (Stage2Monitor)**: Detailed stability monitoring using counter-based analysis
3. **Stage3 (Stage3Merger)**: Merging consecutive stable subflows

## Building

```bash
mkdir build && cd build
cmake ..
make
```

## Usage

```bash
./placidsketch [data_folder_path]
```

## Input Data Format

The program expects CSV files in the specified folder:
- Each file represents one time window
- Files are sorted by name (e.g., `window_0000.csv`, `window_0001.csv`, ...)
- Each CSV file should have a header row
- The **second column** contains the flow fingerprint (16-character hex string, e.g., `6da9ea4534f1ef71`)

## Output

The program outputs:
- Total number of time windows and packets processed
- Number of detected stable flows
- Memory configuration for each stage

## Parameters

Key parameters (defined in `parm.h`):

| Parameter | Default | Description |
|-----------|---------|-------------|
| `STAGE1_MEMORY_BYTES` | 25 KB | Memory for Stage1 |
| `STAGE2_MEMORY_BYTES` | 275 KB | Memory for Stage2 |
| `STAGE3_MEMORY_BYTES` | 100 KB | Memory for Stage3 |
| `SUBFLOW_WINDOWS` | 5 | Windows per subflow |
| `STABLE_THRESHOLD` | 3.0 | Variance threshold for stability |

## Architecture

```
Packet → Stage1Filter → Stage2Monitor → Stage3Merger → Stable Flow Output
         (Continuity)   (Stability)    (Merging)
```

## License

MIT License
