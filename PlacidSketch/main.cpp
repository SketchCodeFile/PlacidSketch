#include "parm.h"
#include "stage1.h"
#include "stage2.h"
#include "stage3.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <string>
#include <vector>

using namespace std;

class PacketProcessor {
    vector<Packet> packets;
    vector<string> csvFiles;

public:
    bool loadDataFromFolder(const string& folderPath) {
        try {
            for (const auto& entry : filesystem::directory_iterator(folderPath)) {
                if (entry.is_regular_file() && entry.path().extension() == ".csv") {
                    csvFiles.push_back(entry.path().string());
                }
            }

            sort(csvFiles.begin(), csvFiles.end());

            for (uint32_t windowNumber = 0; windowNumber < csvFiles.size(); ++windowNumber) {
                if (!loadSingleCsvFile(csvFiles[windowNumber], windowNumber)) {
                    cout << "Failed to load file: " << csvFiles[windowNumber] << endl;
                    return false;
                }
            }

            cout << "Total windows: " << csvFiles.size() 
                 << ", Total packets: " << packets.size() << endl;
            return true;

        } catch (const filesystem::filesystem_error& e) {
            cout << "Error accessing folder: " << e.what() << endl;
            return false;
        }
    }

    bool loadSingleCsvFile(const string& filename, uint32_t windowNumber) {
        ifstream dataFile(filename);
        if (!dataFile.is_open()) {
            cout << "Cannot open file: " << filename << endl;
            return false;
        }

        string line;
        // Skip header line
        if (!getline(dataFile, line)) {
            return true;
        }

        while (getline(dataFile, line)) {
            if (line.empty()) continue;

            // Parse CSV: find second column
            size_t firstComma = line.find(',');
            if (firstComma == string::npos) continue;

            size_t secondComma = line.find(',', firstComma + 1);
            string secondColumn;

            if (secondComma != string::npos) {
                secondColumn = line.substr(firstComma + 1, secondComma - firstComma - 1);
            } else {
                secondColumn = line.substr(firstComma + 1);
            }

            // Trim whitespace
            size_t start = secondColumn.find_first_not_of(" \t");
            size_t end = secondColumn.find_last_not_of(" \t");
            if (start == string::npos) continue;
            secondColumn = secondColumn.substr(start, end - start + 1);

            // Parse hex string (e.g., "6da9ea4534f1ef71") to 64-bit, then take lower 16 bits
            uint64_t fullID = 0;
            for (char c : secondColumn) {
                fullID <<= 4;
                if (c >= '0' && c <= '9') {
                    fullID |= (c - '0');
                } else if (c >= 'a' && c <= 'f') {
                    fullID |= (c - 'a' + 10);
                } else if (c >= 'A' && c <= 'F') {
                    fullID |= (c - 'A' + 10);
                } else {
                    break;
                }
            }

            // Take lower 16 bits as flow ID
            uint16_t flowID = static_cast<uint16_t>(fullID & 0xFFFF);

            packets.emplace_back(flowID, windowNumber);
        }

        return true;
    }

    const vector<Packet>& getPackets() const { return packets; }
};

class PlacidSketch {
private:
    Stage3Merger stage3;
    Stage1Filter stage1;
    Stage2Monitor stage2;
    uint32_t currentWindow = 0;
    vector<uint16_t> detectedStableFlows;

public:
    explicit PlacidSketch(size_t stage1MemoryBytes = STAGE1_MEMORY_BYTES, 
                         size_t stage2MemoryBytes = STAGE2_MEMORY_BYTES)
        : stage1(stage1MemoryBytes), stage2(stage3, stage2MemoryBytes) {
        stage2.setDetectedFlowsCallback(&detectedStableFlows);
    }

    void processPacket(const Packet& packet) {
        uint32_t windowSeq = packet.windowNumber;

        if (windowSeq != currentWindow) {
            stage1.resetBuckets(currentWindow);
            currentWindow = windowSeq;
        }

        if (stage1.processPacket(packet.flowID, windowSeq)) {
            stage2.processPotentialFlow(packet.flowID, windowSeq);
        }
    }

    void finalizeProcessing() {
        stage1.resetBuckets(currentWindow);
        stage3.finalize();
        
        sort(detectedStableFlows.begin(), detectedStableFlows.end());
        detectedStableFlows.erase(unique(detectedStableFlows.begin(), 
                                        detectedStableFlows.end()), 
                                 detectedStableFlows.end());
    }

    const vector<uint16_t>& getDetectedStableFlows() const {
        return detectedStableFlows;
    }
};

int main(int argc, char* argv[]) {
    cout << "========================================" << endl;
    cout << "  PlacidSketch Stable Flow Detection   " << endl;
    cout << "========================================" << endl;

    //data path
    string folderPath = " ";
    
    // Allow custom data path from command line
    if (argc > 1) {
        folderPath = argv[1];
    }

    PacketProcessor dataLoader;
    if (!dataLoader.loadDataFromFolder(folderPath)) {
        cout << "\nFailed to load data from folder: " << folderPath << endl;
        cout << "Usage: " << argv[0] << " [data_folder_path]" << endl;
        return 1;
    }

    const auto& packets = dataLoader.getPackets();

    cout << "\nRunning PlacidSketch..." << endl;
    PlacidSketch sketch;
    
    for (const auto& packet : packets) {
        sketch.processPacket(packet);
    }
    sketch.finalizeProcessing();

    const auto& detectedFlows = sketch.getDetectedStableFlows();

    cout << "\n========== Detection Results ==========" << endl;
    cout << "Detected Stable Flows: " << detectedFlows.size() << endl;
    cout << endl;
    
    cout << "Memory Configuration:" << endl;
    cout << "  Stage1 Memory: " << STAGE1_MEMORY_BYTES / 1024.0 << " KB" << endl;
    cout << "  Stage2 Memory: " << STAGE2_MEMORY_BYTES / 1024.0 << " KB" << endl;
    cout << "  Stage3 Memory: " << STAGE3_MEMORY_BYTES / 1024.0 << " KB" << endl;
    cout << "  Total Memory:  " 
         << (STAGE1_MEMORY_BYTES + STAGE2_MEMORY_BYTES + STAGE3_MEMORY_BYTES) / 1024.0 
         << " KB" << endl;
    cout << "========================================" << endl;

    return 0;
}
