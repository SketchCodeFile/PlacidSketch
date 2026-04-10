/**
 * @file Param.h
 * @brief XSketch algorithm parameter definitions and helper functions
 * 
 * Contains matrix initialization and regression functions for linear regression calculation
 */

#ifndef _PARAM_H_
#define _PARAM_H_

#include <bits/stdc++.h>
#include "Common/Matrix.h"

// Window size (timestamp units)
const uint32_t window_size = 10000;

// Memory size (MB)
const int memory_p = 300;

// Memory ratio allocated to Stage1
const double stage_ratio_p = 0.8;

// Minimum threshold for a_K (used for detection)
const double var_thres_p = 0.1;

// Number of consecutive windows (used for linear regression)
const int P = 7;

// Polynomial degree
const int K = 0;

// Number of windows recorded in Stage1
const int S = 4;

// Stage1 window count constant
const int S_p = S;

// Mean squared error threshold
const double error_thres_p = 0.5;

// Number of slots per bucket
const int bucket_size_p = 4;

// Potential threshold
const double potential_thres_p = 0.25;

// Matrix for linear regression calculation (P windows, K+1 coefficients)
double calcu_matrix[K + 1][P] = {};

// Matrix for linear regression calculation (S windows, K+1 coefficients)
double calcu_matrix_try[K + 1][S_p] = {};

/**
 * @brief Report_Slot class: stores reported flow information
 * 
 * @tparam ID_TYPE Flow ID type
 */
template<typename ID_TYPE>
class Report_Slot {
public:
    ID_TYPE id;              // Flow ID
    uint32_t start_window;   // Start window
    uint32_t end_window;     // End window

    Report_Slot() {}

    Report_Slot(ID_TYPE _id, uint32_t st, uint32_t et): 
        id(_id), start_window(st), end_window(et) {}

    ~Report_Slot() {}

    // Comparison operator: sort by flow duration
    bool operator < (const Report_Slot &r) {
        if (end_window - start_window == r.end_window - r.start_window) {
            if (start_window == r.start_window)
                return id < r.id;
            return start_window < r.start_window;
        }
        return end_window - start_window < r.end_window - r.start_window;
    }
};

/**
 * @brief Initialize matrix for linear regression calculation
 * 
 * Computes coefficient matrix for least squares solution via normal equations
 */
void init_matrix() {
    // X matrix: [P x (K+1)]
    double X[P][K + 1] = {};
    double XX[K + 1][K + 1] = {};
    double temp[(K + 1) * (K + 1)];

    // Build X matrix: X[i][j] = i^j
    for (int i = 0; i <= P - 1; ++i) {
        for (int j = 0; j <= K; ++j) {
            X[i][j] = pow(i, j);
        }
    }

    // Compute X'X
    for (int i = 0; i <= K; ++i) {
        for (int j = 0; j <= K; ++j) {
            for (int k = 0; k <= P - 1; ++k) {
                XX[i][j] += X[k][i] * X[k][j];
            }
        }
    }

    // Copy to temp array
    for (int i = 0; i <= K; ++i) {
        for (int j = 0; j <= K; ++j) {
            temp[i * (K + 1) + j] = XX[i][j];
        }
    }

    // Compute matrix inverse
    inverse(K + 1, temp);

    // Update XX to inverse matrix
    for (int i = 0; i <= K; ++i) {
        for (int j = 0; j <= K; ++j) {
            XX[i][j] = temp[i * (K + 1) + j];
        }
    }

    // Compute (X'X)^{-1}X', store in calcu_matrix
    for (int i = 0; i <= K; ++i) {
        for (int j = 0; j <= P - 1; ++j) {
            for (int k = 0; k <= K; ++k) {
                calcu_matrix[i][j] += XX[i][k] * X[j][k];
            }
        }
    }

    // ========== Part 2: Matrix computation for Stage1 ==========

    double Z[S_p][K + 1] = {};

    // Build Z matrix: Z[i][j] = i^j (using S windows)
    for (int i = 0; i <= S_p - 1; ++i) {
        for (int j = 0; j <= K; ++j) {
            X[i][j] = pow(i, j);
        }
    }

    // Reset XX and recompute
    memset(XX, 0, sizeof(XX));
    for (int i = 0; i <= K; ++i) {
        for (int j = 0; j <= K; ++j) {
            for (int k = 0; k <= S_p - 1; ++k) {
                XX[i][j] += X[k][i] * X[k][j];
            }
        }
    }

    for (int i = 0; i <= K; ++i) {
        for (int j = 0; j <= K; ++j) {
            temp[i * (K + 1) + j] = XX[i][j];
        }
    }

    // Compute matrix inverse
    inverse(K + 1, temp);

    // Update XX to inverse matrix
    for (int i = 0; i <= K; ++i) {
        for (int j = 0; j <= K; ++j) {
            XX[i][j] = temp[i * (K + 1) + j];
        }
    }

    // Compute calcu_matrix_try
    for (int i = 0; i <= K; ++i) {
        for (int j = 0; j <= S_p - 1; ++j) {
            for (int k = 0; k <= K; ++k) {
                calcu_matrix_try[i][j] += XX[i][k] * Z[j][k];
            }
        }
    }
}

/**
 * @brief Linear regression function (used in Stage2)
 * 
 * Fast linear regression using precomputed matrix
 * @param y Input P observed values
 * @param b Output K+1 regression coefficients
 */
void linear_regressing(double* y, double* b) {
    memset(b, 0, (K + 1) * sizeof(double));
    for (int i = 0; i <= K; ++i) {
        for (int j = 0; j <= P - 1; ++j) {
            b[i] += calcu_matrix[i][j] * y[j];
        }
    }
}

/**
 * @brief Linear regression function (used in push operation)
 * 
 * Fast linear regression for S windows using precomputed matrix
 * @param y Input S observed values
 * @param b Output K+1 regression coefficients
 */
void linear_regressing_try(double* y, double* b) {
    memset(b, 0, (K + 1) * sizeof(double));
    for (int i = 0; i <= K; ++i) {
        for (int j = 0; j <= S_p - 1; ++j) {
            b[i] += calcu_matrix[i][j] * y[j];
        }
    }
}

#endif