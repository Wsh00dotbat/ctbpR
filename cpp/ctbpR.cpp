#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <random>
#include <nlohmann/json.hpp> // You'll need to include this JSON library

using json = nlohmann::json;
namespace fs = std::filesystem;

class CursorResetError : public std::exception {
private:
    std::string message;
public:
    explicit CursorResetError(const std::string& msg) : message(msg) {}
    const char* what() const noexcept override { return message.c_str(); }
};

// Generate random hex string
std::string generateRandomHex(size_t length) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < length; ++i) {
        ss << std::setw(2) << dis(gen);
    }
    return ss.str();
}

// Generate UUID (simplified version)
std::string generateUUID() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 8; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 4; i++) ss << dis(gen);
    ss << "-4";  // Version 4 UUID
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    ss << std::uniform_int_distribution<>(8, 11)(gen); // Variant
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 12; i++) ss << dis(gen);
    return ss.str();
}

fs::path getStorageFile() {
    std::string system = 
#ifdef _WIN32
        "Windows";
#elif __APPLE__
        "Darwin";
#elif __linux__
        "Linux";
#else
        throw CursorResetError("Unsupported operating system");
#endif

    fs::path base_path;
    if (system == "Windows") {
        base_path = fs::path(getenv("APPDATA")) / "Cursor";
    } else if (system == "Darwin") {
        base_path = fs::path(getenv("HOME")) / "Library" / "Application Support" / "Cursor";
    } else if (system == "Linux") {
        base_path = fs::path(getenv("HOME")) / ".config" / "Cursor";
    }
    return base_path / "User" / "globalStorage" / "storage.json";
}

fs::path backupFile(const fs::path& file_path) {
    if (!fs::exists(file_path)) return "";
    
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream timestamp;
    timestamp << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S");
    
    fs::path backup_path = file_path.string() + ".backup_" + timestamp.str();
    fs::copy_file(file_path, backup_path);
    std::cout << "Created backup at: " << backup_path << std::endl;
    return backup_path;
}

json generateNewIds() {
    json new_ids;
    new_ids["machineId"] = generateRandomHex(32);
    new_ids["macMachineId"] = generateRandomHex(32);
    new_ids["devDeviceId"] = generateUUID();
    return new_ids;
}

void resetCursorId() {
    try {
        fs::path storage_file = getStorageFile();
        std::cout << "Targeting storage file: " << storage_file << std::endl;

        fs::create_directories(storage_file.parent_path());
        
        fs::path backup_path = backupFile(storage_file);
        
        json data;
        if (fs::exists(storage_file)) {
            std::ifstream in_file(storage_file);
            if (in_file.is_open()) {
                in_file >> data;
                in_file.close();
            }
        }

        json new_ids = generateNewIds();
        if (!data.contains("telemetry")) {
            data["telemetry"] = json::object();
        }
        data["telemetry"]["machineId"] = new_ids["machineId"];
        data["telemetry"]["macMachineId"] = new_ids["macMachineId"];
        data["telemetry"]["devDeviceId"] = new_ids["devDeviceId"];

        std::ofstream out_file(storage_file);
        if (!out_file.is_open()) {
            if (!backup_path.empty()) {
                fs::copy_file(backup_path, storage_file);
            }
            throw CursorResetError("Failed to write storage file");
        }
        out_file << std::setw(2) << data << std::endl;
        out_file.close();

        std::cout << "🎉 Device IDs have been successfully reset!" << std::endl;
        std::cout << "New device IDs:" << std::endl;
        std::cout << std::setw(2) << new_ids << std::endl;
    }
    catch (const CursorResetError& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        throw;
    }
    catch (const std::exception& e) {
        std::cerr << "❌ Unexpected error occurred: " << e.what() << std::endl;
        throw;
    }
}

int main() {
    std::cout << "Cursor Trial Reset Tool v2.0" << std::endl;
    std::cout << "---------------------------" << std::endl;
    
    try {
        resetCursorId();
        return 0;
    }
    catch (...) {
        std::cerr << "\nFor more details, check program output" << std::endl;
        return 1;
    }
}