/**
 * @file main.cc
 * @brief XSketch core functionality test program
 * 
 * This program runs only the XSketch component without the benchmark framework
 */

#include <bits/stdc++.h>
#include "XSketch.h"
#include "Param.h"
#include "Common/Mmap.h"

using namespace std;

// Define data type
typedef uint64_t ID_TYPE;

/**
 * @brief Simple XSketch test program
 * @param argc Number of command line arguments
 * @param argv Command line arguments
 *        argv[1]: Data file path
 *        argv[2]: Memory size (MB)
 *        argv[3]: Run length (number of insert operations)
 * @return Program exit code
 */
int main(int argc, char** argv) {
    // Initialize matrix for linear regression calculation
    init_matrix();

    // Parse command line arguments
    if (argc < 4) {
        cout << "Usage: " << argv[0] << " <data_file_path> <memory_size(MB)> <run_length>" << endl;
        return 1;
    }

    string PATH = argv[1];
    uint32_t memory = atoi(argv[2]);
    uint32_t run_length = atoi(argv[3]);

    // Load dataset
    LoadResult load_result = Load(PATH.c_str());
    CAIDA_Tuple* dataset = (CAIDA_Tuple*)load_result.start;
    uint64_t length = load_result.length / sizeof(CAIDA_Tuple);

    cout << "Dataset length: " << length << endl;
    cout << "Memory size: " << memory << " MB" << endl;
    cout << "Run length: " << run_length << endl;

    // Create XSketch instance with default parameters from Param.h
    XSketch<ID_TYPE>* x_sketch = new XSketch<ID_TYPE>(
        memory, 
        var_thres_p,    // Variance threshold
        error_thres_p,  // Error threshold
        stage_ratio_p,  // Stage1 memory ratio
        bucket_size_p,  // Number of slots per bucket
        S_p,            // Number of Stage1 windows
        potential_thres_p  // Potential threshold
    );

    // Record start time
    auto start = std::chrono::high_resolution_clock::now();

    // Perform insert operations
    for (uint32_t i = 0; i < run_length; ++i) {
        x_sketch->insert(dataset[i % length].id, i);

        // Output progress every 10000 insertions
        if (i > 0 && i % 10000 == 0) {
            cout << "Processed " << i << " records..." << endl;
        }
    }

    // Record end time
    auto end = std::chrono::high_resolution_clock::now();
    chrono::duration<double, milli> tm = end - start;

    cout << "Insertion completed!" << endl;
    cout << "Time elapsed: " << tm.count() << " ms" << endl;
    cout << "Throughput: " << run_length / (1.0 * tm.count() * 1000) << " M/s" << endl;

    // Query results
    vector<pair<ID_TYPE, uint32_t>> result = x_sketch->query();
    vector<Report_Slot<ID_TYPE>> report = x_sketch->report();

    cout << "Number of detected flows: " << result.size() << endl;
    cout << "Number of reported Top-K flows: " << report.size() << endl;

    // Clean up resources
    delete x_sketch;
    UnLoad(load_result);

    return 0;
}