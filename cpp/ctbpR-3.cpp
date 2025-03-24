#include <iostream>
#include <fstream>
#include <filesystem>
#include <random>
#include <nlohmann/json.hpp>
#include <chrono>
#include <sstream>

namespace fs = std::filesystem;
using json = nlohmann::json;

std::string generateRandomHex(size_t length) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, 15);
    std::stringstream ss;
    for (size_t i = 0; i < length; ++i) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

std::string generateUUID() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, 15);
    std::stringstream ss;
    const int uuidFormat[] = {8, 4, 4, 4, 12};
    int i = 0;
    for (const int& group : uuidFormat) {
        if (i++ > 0) ss << "-";
        for (int j = 0; j < group; ++j) {
            ss << std::hex << dis(gen);
        }
    }
    return ss.str();
}

fs::path getStorageFile() {
    #ifdef _WIN32
        char* appdata = std::getenv("APPDATA");
        if (!appdata) throw std::runtime_error("Failed to get APPDATA path.");
        return fs::path(appdata) / "Cursor" / "User" / "globalStorage" / "storage.json";
    #elif __APPLE__
        return fs::path(std::getenv("HOME")) / "Library" / "Application Support" / "Cursor" / "User" / "globalStorage" / "storage.json";
    #elif __linux__
        return fs::path(std::getenv("HOME")) / ".config" / "Cursor" / "User" / "globalStorage" / "storage.json";
    #else
        throw std::runtime_error("Unsupported operating system");
    #endif
}

void backupFile(const fs::path& filePath) {
    if (fs::exists(filePath)) {
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&timestamp), "%Y%m%d_%H%M%S");
        fs::copy(filePath, filePath.parent_path() / ("storage.json.backup_" + ss.str()));
    }
}

void resetCursorID() {
    try {
        fs::path storageFile = getStorageFile();
        fs::create_directories(storageFile.parent_path());
        backupFile(storageFile);
        
        json data;
        if (fs::exists(storageFile)) {
            std::ifstream inFile(storageFile);
            if (inFile) {
                try {
                    inFile >> data;
                } catch (json::parse_error&) {
                    std::cerr << "⚠️ Warning: The storage file is corrupted. A new one will be created.\n";
                    data = json::object();
                }
            }
        }
        
        data["telemetry.machineId"] = generateRandomHex(32);
        data["telemetry.macMachineId"] = generateRandomHex(32);
        data["telemetry.devDeviceId"] = generateUUID();
        
        std::ofstream outFile(storageFile);
        outFile << data.dump(2);
        
        std::cout << "🎉 Device IDs have been successfully reset. The new device IDs are:\n";
        std::cout << data.dump(2) << "\n";
    } catch (const std::exception& e) {
        std::cerr << "❌ An error occurred: " << e.what() << "\n";
    }
}

int main() {
    resetCursorID();
    return 0;
}
