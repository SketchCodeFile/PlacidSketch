/**
 * @file Mmap.h
 * @brief Memory-mapped file operations for loading datasets
 */

#ifndef MMAP_H
#define MMAP_H

#include <fcntl.h>
#include <stdint.h>

#include <iostream>

/**
 * @brief CAIDA dataset tuple structure
 * 
 * Contains timestamp and flow ID
 */
struct CAIDA_Tuple {
    uint64_t timestamp;  // Timestamp
    uint64_t id;         // Flow ID
};

/**
 * @brief Load result structure
 * 
 * Contains pointer to loaded data and its length
 */
struct LoadResult{
    void* start;    // Pointer to data start
    uint64_t length; // Data length in bytes
};

// Function declarations
LoadResult Load(const char* PATH);
void UnLoad(LoadResult result);


#include <sys/stat.h>
#include <sys/mman.h>

/**
 * @brief Load file into memory using mmap
 * @param PATH File path
 * @return LoadResult containing pointer and length
 */
LoadResult Load(const char* PATH){
    LoadResult ret;

    // Open file
    int32_t fd = open(PATH, O_RDONLY);
    if(fd == -1) {
        std::cerr << "Cannot open " << PATH << std::endl;
        throw;
    }

    // Get file size
    struct stat sb;
    if(fstat(fd, &sb) == -1){
        std::cerr << "Fstat Error" << std::endl;
        throw;
    }

    ret.length = sb.st_size;
    // Map file to memory
    ret.start = mmap(nullptr, ret.length, PROT_READ, MAP_PRIVATE, fd, 0u);

    if (ret.start == MAP_FAILED) {
        std::cerr << "Cannot mmap " << PATH << " of length " << ret.length << std::endl;
        throw;
    }

    return ret;
}

/**
 * @brief Unload memory-mapped file
 * @param result LoadResult to unload
 */
void UnLoad(LoadResult result){
    munmap(result.start, result.length);
}

#endif