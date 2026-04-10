/**
 * @file Matrix.h
 * @brief Matrix computation functions
 * 
 * Provides determinant calculation and matrix inversion for linear regression
 */

#ifndef _MATRIX_H_
#define _MATRIX_H_

#include <bits/stdc++.h>
#include <cassert>

/**
 * @brief Calculate the determinant of an n×n matrix (recursive method)
 * @param n Matrix order
 * @param aa Matrix data pointer (row-major order)
 * @return Determinant value
 */
double det(int n, double *aa) {
    // Base case: 1×1 matrix
	if (n == 1)
		return aa[0];
    
    // Allocate temporary sub-matrix memory
	double *bb = new double[(n - 1) * (n - 1)];
	int mov = 0;
	double sum = 0.0;
    
    // Expand along rows to compute determinant
	for (int arow = 0; arow < n; arow++) {
		for (int brow = 0; brow < n - 1; brow++) {
            // Calculate offset
			mov = arow > brow ? 0 : 1;
            // Fill sub-matrix elements
			for (int j = 0; j < n - 1; j++) {
				bb[brow * (n - 1) + j] = aa[(brow + mov) * n + j + 1];
			}
		}
        // Calculate cofactor sign
		int flag = (arow % 2 == 0 ? 1: -1);
        // Recursively compute sub-matrix determinant
		sum += flag * aa[arow*n] * det(n - 1, bb);
	}
	delete[] bb;
	return sum;
}

/**
 * @brief Calculate matrix inverse using adjugate matrix method
 * @param n Matrix order
 * @param aa Input matrix data, result overwrites original array
 */
void inverse(int n, double* aa) {
    // Base case: 1×1 matrix, just take reciprocal
	if (n == 1) {
		aa[0] = 1 / aa[0];
		return;
	}
    
    // Calculate determinant of original matrix
	double det_aa = det(n, aa);
	assert(det_aa != 0);
    
    // Allocate adjugate matrix and temporary matrix memory
	double *adjoint = new double[n * n];
	double *bb = new double[(n - 1) * (n - 1)];
	int pi, pj, q;
    
    // Compute cofactor for each element to build adjugate matrix
	for (int ai = 0; ai < n; ai++) {
		for (int aj = 0; aj < n; aj++) {
            // Build sub-matrix by removing row ai and column aj
			for (int bi = 0; bi < n - 1; bi++) {
				for (int bj = 0; bj < n - 1; bj++) {
                    // Calculate mapping position for sub-matrix
					if (ai > bi)
						pi = 0;
					else
						pi = 1; 
					if (aj > bj)
						pj = 0;
					else
						pj = 1;
                    // Fill sub-matrix elements
					bb[bi * (n - 1) + bj] = aa[(bi + pi) * n + bj + pj];
				}
			}
            // Calculate cofactor sign
			if ((ai + aj) % 2 == 0) 
				q = 1;
			else 
				q = -1;
            // Store adjugate matrix elements (transposed cofactors)
			adjoint[ai * n + aj] = q * det(n - 1, bb);
		}
	}
    
    // Transpose adjugate matrix to get the adjugate
    for(int i = 0; i < n; i++) {
        for(int j = 0; j < i; j++) {
            double tem = adjoint[i * n + j];
            adjoint[i * n + j] =  adjoint[j * n + i];
            adjoint[j * n + i] =  tem;
        }
    }
    
    // Divide adjugate by determinant to get inverse matrix
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			aa[i * n + j] = adjoint[i * n + j] / det_aa;
		}
	}

	delete[] adjoint;
	delete[] bb;
}

#endif