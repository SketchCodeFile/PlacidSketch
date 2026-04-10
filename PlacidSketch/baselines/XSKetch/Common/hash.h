/**
 * @file hash.h
 * @brief Hash function wrapper
 */

#ifndef HASH_H
#define HASH_H

#include <random>
#include <iostream>
#include <limits.h>
#include <stdint.h>
#include "BOBHash.h"

template<typename T>
inline uint32_t hash(const T& data, uint32_t seed = 0);
inline uint32_t randomGenerator();

// Random number generator
static std::random_device rd;
static std::mt19937 rng(rd());
static std::uniform_real_distribution<double> dis(0, 1);

// Generate random number
inline uint32_t randomGenerator() {
    return rng();
}

/**
 * @brief Hash function template
 * 
 * Uses BOBHash for hash computation
 * @param data Input data
 * @param seed Hash seed
 * @return Hash value
 */
template<typename T>
inline uint32_t hash(const T& data, uint32_t seed) {
    return BOBHash::BOBHash32((uint8_t*)&data, sizeof(T), seed);
}

#endif