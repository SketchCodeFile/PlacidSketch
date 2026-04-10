
#include "parm.h"
#include "stage1.h"
#include "stage2.h"
#include "stage3.h"
#include "SteadySketch.h"
#include "ground_truth_baseline.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

using namespace std;

// ============== 数据加载器 ==============

class PacketProcessor {
private:
    vector<Packet> packets;
    size_t packetCount = 0;
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
                if (!loadSingleCSVFile(csvFiles[windowNumber], windowNumber)) {
                    return false;
                }
            }

            packetCount = packets.size();
            return true;

        } catch (const filesystem::filesystem_error& e) {
            return false;
        }
    }

    bool loadSingleCSVFile(const string& filename, uint32_t windowNumber) {
        ifstream dataFile(filename);
        string headerLine;
        getline(dataFile, headerLine);

        while (getline(dataFile, headerLine)) {
            if (headerLine.empty()) continue;

            size_t firstComma = headerLine.find(',');
            if (firstComma == string::npos) continue;

            string quintuple = headerLine.substr(0, firstComma);
            quintuple.erase(0, quintuple.find_first_not_of(" \t"));
            quintuple.erase(quintuple.find_last_not_of(" \t") + 1);

            size_t secondComma = headerLine.find(',', firstComma + 1);
            string fingerprint;
            if (secondComma != string::npos) {
                fingerprint = headerLine.substr(firstComma + 1, secondComma - firstComma - 1);
            } else {
                fingerprint = headerLine.substr(firstComma + 1);
            }
            fingerprint.erase(0, fingerprint.find_first_not_of(" \t"));
            fingerprint.erase(fingerprint.find_last_not_of(" \t") + 1);

            if (quintuple.empty() || fingerprint.empty()) continue;

            Packet packet(fingerprint.c_str(), quintuple.c_str(), windowNumber);
            packets.push_back(packet);
        }

        dataFile.close();
        return true;
    }

    const vector<Packet>& getPackets() const { return packets; }
    const vector<string>& getCSVFiles() const { return csvFiles; }
    size_t getWindowCount() const { return csvFiles.size(); }
};

// ============== PlacidSketch 封装 ==============

class PlacidSketchWrapper {
private:
    Stage3Merger stage3;
    Stage1Filter stage1;
    Stage2Monitor stage2;
    uint32_t currentWindow = 0;
    unordered_map<string, string> fingerprintToQuintuple;

public:
    PlacidSketchWrapper() : stage3(), stage1(), stage2(stage3) {}

    void processPacket(const Packet& packet) {
        uint32_t windowSeq = packet.windowNumber;
        if (windowSeq != currentWindow) {
            currentWindow = windowSeq;
        }
        stage2.processPotentialFlow(packet.flowID, windowSeq);
    }

    void buildFingerprintMapping(const vector<Packet>& packets) {
        for (const auto& packet : packets) {
            if (packet.quintuple[0] != '\0' && packet.flowID[0] != '\0') {
                fingerprintToQuintuple.try_emplace(packet.flowID, packet.quintuple);
            }
        }
    }

    void finalizeProcessing() {
        stage3.finalize();
    }

    size_t getStableFlowCount() const {
        return stage3.getStableFlowCount();
    }

    Stage3Merger& getStage3() { return stage3; }
    
    set<string> convertFingerprintsToQuintuples(const set<string>& fingerprints) const {
        set<string> quintuples;
        for (const auto& fingerprint : fingerprints) {
            auto it = fingerprintToQuintuple.find(fingerprint);
            if (it != fingerprintToQuintuple.end() && !it->second.empty()) {
                quintuples.insert(it->second);
            } else {
                quintuples.insert(fingerprint);
            }
        }
        return quintuples;
    }

    string getQuintupleForFingerprint(const string& fingerprint) const {
        auto it = fingerprintToQuintuple.find(fingerprint);
        if (it != fingerprintToQuintuple.end() && !it->second.empty()) {
            return it->second;
        }
        return fingerprint;
    }
    
    size_t getFingerprintMappingSize() const {
        return fingerprintToQuintuple.size();
    }
};

// ============== 测试结果结构 ==============

struct TestResult {
    size_t memory_kb;
    double placidPrecision;
    double placidRecall;
    double placidF1;
    size_t placidStableFlows;
    double steadyPrecision;
    double steadyRecall;
    double steadyF1;
    size_t steadyStableFlows;
};

// ============== 主程序 ==============

int main() {
    cout << "========================================" << endl;
    cout << "  Stable Flow Detection Algorithm Test" << endl;
    cout << "========================================" << endl;
    
    // 加载数据
    PacketProcessor dataLoader;
    string folderPath = "E:/CLionProjects/Merge_Csv/cmake-build-debug/merged_output";
    
    cout << "\nLoading data from: " << folderPath << endl;
    if (!dataLoader.loadDataFromFolder(folderPath)) {
        cerr << "Failed to load data!" << endl;
        return 1;
    }

    vector<Packet> packets = dataLoader.getPackets();
    const size_t totalPackets = packets.size();
    cout << "Loaded " << totalPackets << " packets, " << dataLoader.getWindowCount() << " windows" << endl;

    // Ground Truth 计算
    GroundTruthBaseline groundTruth;
    for (const auto& packet : packets) {
        string quintuple(packet.quintuple);
        if (!quintuple.empty()) {
            groundTruth.insert(quintuple, packet.windowNumber);
        }
    }
    groundTruth.flush();
    
    const string groundTruthOutputPath = "E:/CLionProjects/MiningDataGenerate/cmake-build-debug/mining_five_tuples_data_v2/all_mining_five_tuples.csv";
    groundTruth.exportResults(groundTruthOutputPath);
    set<string> groundTruthFlows = groundTruth.getStableIDs();
    
    // 测试配置
    vector<size_t> memoryConfigs = {200, 300, 400, 500, 600};
    vector<TestResult> allResults;
    
    cout << fixed << setprecision(4);
    cout << "\n========== Ground Truth Information ==========" << endl;
    cout << "Baseline: GroundTruthBaseline" << endl;
    cout << "Ground Truth Flows: " << groundTruthFlows.size() << endl;
    cout << "==========================================\n" << endl;
    
    double placid_r = 0.2;
    double steady_r = 0.8;
    
    for (size_t memory_kb : memoryConfigs) {
        cout << "\n========== Testing Memory Configuration: " << memory_kb << " KB ==========" << endl;
        cout << "PlacidSketch r = " << placid_r << " (Stage1占" << (placid_r * 100) << "%)" << endl;
        cout << "SteadySketch r = " << steady_r << " (steady_filter占" << (steady_r * 100) << "%)" << endl;
        
        STAGE1_STAGE2_TOTAL_MEMORY_BYTES = memory_kb * 1024;
        r = placid_r;
        steady::steady_filter_rolling_sketch_total_memory_kb = memory_kb;
        steady::r = steady_r;
        
        TestResult result;
        result.memory_kb = memory_kb;
        
        // PlacidSketch 处理
        PlacidSketchWrapper sketch;
        for (size_t i = 0; i < totalPackets; ++i) {
            sketch.processPacket(packets[i]);
        }
        sketch.finalizeProcessing();
        sketch.buildFingerprintMapping(packets);
        Stage3Merger& stage3 = sketch.getStage3();
        result.placidStableFlows = sketch.getStableFlowCount();
        
        // SteadySketch 处理
        SteadySketch steadySketch(steady::DefaultHashSeed);
        for (const auto& packet : packets) {
            steadySketch.Insert(packet.flowID, packet.windowNumber, packet.quintuple);
        }
        steadySketch.Flush();
        steadySketch.ReportValue(0.0);
        
        set<string> steadyFingerprints;
        for (const auto& kv : steadySketch.TopKReport) {
            steadyFingerprints.insert(kv.first);
        }
        set<string> steadyFlows = steadySketch.convertFingerprintsToQuintuples(steadyFingerprints);
        result.steadyStableFlows = steadyFlows.size();
        
        // 计算精确率和召回率
        auto placidFingerprints = stage3.getStableFlowIDs();
        auto placidFlows = sketch.convertFingerprintsToQuintuples(placidFingerprints);

        set<string> placidGtIntersection;
        set_intersection(placidFlows.begin(), placidFlows.end(),
                        groundTruthFlows.begin(), groundTruthFlows.end(),
                        inserter(placidGtIntersection, placidGtIntersection.begin()));
        size_t placidGtCommon = placidGtIntersection.size();
        
        set<string> steadyGtIntersection;
        set_intersection(steadyFlows.begin(), steadyFlows.end(),
                        groundTruthFlows.begin(), groundTruthFlows.end(),
                        inserter(steadyGtIntersection, steadyGtIntersection.begin()));
        size_t steadyGtCommon = steadyGtIntersection.size();
        
        result.placidPrecision = placidFlows.empty() ? 0.0 
            : static_cast<double>(placidGtCommon) / static_cast<double>(placidFlows.size());
        result.placidRecall = groundTruthFlows.empty() ? 0.0
            : static_cast<double>(placidGtCommon) / static_cast<double>(groundTruthFlows.size());
        result.placidF1 = (result.placidPrecision + result.placidRecall) > 0.0
            ? (2.0 * result.placidPrecision * result.placidRecall) / (result.placidPrecision + result.placidRecall)
            : 0.0;
        
        result.steadyPrecision = steadyFlows.empty() ? 0.0
            : static_cast<double>(steadyGtCommon) / static_cast<double>(steadyFlows.size());
        result.steadyRecall = groundTruthFlows.empty() ? 0.0
            : static_cast<double>(steadyGtCommon) / static_cast<double>(groundTruthFlows.size());
        result.steadyF1 = (result.steadyPrecision + result.steadyRecall) > 0.0
            ? (2.0 * result.steadyPrecision * result.steadyRecall) / (result.steadyPrecision + result.steadyRecall)
            : 0.0;
        
        cout << "\nPlacidSketch:" << endl;
        cout << "  Detected Flows: " << placidFlows.size() << endl;
        cout << "  True Positives: " << placidGtCommon << endl;
        cout << "  Precision: " << result.placidPrecision << endl;
        cout << "  Recall: " << result.placidRecall << endl;
        cout << "  F1 Score: " << result.placidF1 << endl;
        cout << "\nSteadySketch:" << endl;
        cout << "  Detected Flows: " << steadyFlows.size() << endl;
        cout << "  True Positives: " << steadyGtCommon << endl;
        cout << "  Precision: " << result.steadyPrecision << endl;
        cout << "  Recall: " << result.steadyRecall << endl;
        cout << "  F1 Score: " << result.steadyF1 << endl;
        cout << "==========================================\n" << endl;
        
        allResults.push_back(result);
    }
    
    // 输出汇总表格
    cout << "\n========== Summary Table ==========" << endl;
    cout << left << setw(10) << "Memory(KB)" 
              << setw(20) << "Algorithm" 
              << setw(12) << "Precision" 
              << setw(12) << "Recall" 
              << setw(12) << "F1 Score" 
              << setw(15) << "Detected Flows" << endl;
    cout << string(80, '-') << endl;
    
    for (const auto& result : allResults) {
        cout << left << setw(10) << result.memory_kb
                  << setw(20) << "PlacidSketch"
                  << setw(12) << result.placidPrecision
                  << setw(12) << result.placidRecall
                  << setw(12) << result.placidF1
                  << setw(15) << result.placidStableFlows << endl;
        
        cout << left << setw(10) << ""
                  << setw(20) << "SteadySketch"
                  << setw(12) << result.steadyPrecision
                  << setw(12) << result.steadyRecall
                  << setw(12) << result.steadyF1
                  << setw(15) << result.steadyStableFlows << endl;
    }
    cout << "==========================================\n" << endl;

    return 0;
}
