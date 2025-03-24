/**
 * Cursor Trial Reset Tool
 * This program resets the device IDs in Cursor's configuration file to generate a new random device ID.
 * Repository: https://github.com/ultrasev/cursor-reset
 * Author: @ultrasev (Python version)
 * C++ port: [Your name]
 * Created: 24/Mar/2025
 * Updated: 24/Mar/2025 (for Cursor v0.47.8)
 */

#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <filesystem>
#include <random>
#include <iomanip>
#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

using json = nlohmann::json;
namespace fs = std::filesystem;

// Generate a random hex string of specified length
std::string generate_random_hex(size_t length) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    for (size_t i = 0; i < length; i++) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

// Generate a UUID v4
std::string generate_uuid() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::uniform_int_distribution<> dis2(8, 11);

    std::stringstream ss;
    ss << std::hex;

    for (int i = 0; i < 8; i++) {
        ss << dis(gen);
    }
    ss << "-";

    for (int i = 0; i < 4; i++) {
        ss << dis(gen);
    }
    ss << "-4";

    for (int i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";
    
    ss << dis2(gen);
    for (int i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";

    for (int i = 0; i < 12; i++) {
        ss << dis(gen);
    }

    return ss.str();
}

// Create a timestamped backup of the given file
void backup_file(const fs::path& file_path) {
    if (fs::exists(file_path)) {
        std::time_t now = std::time(nullptr);
        std::tm tm;
        
        #ifdef _WIN32
        localtime_s(&tm, &now);
        #else
        localtime_r(&now, &tm);
        #endif
        
        char buffer[20];
        std::strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", &tm);
        
        fs::path backup_path = file_path.string() + ".backup_" + buffer;
        fs::copy_file(file_path, backup_path, fs::copy_options::overwrite_existing);
    }
}

// Determine the storage file location based on the operating system
fs::path get_storage_file() {
    fs::path storage_path;
    
    #ifdef _WIN32
    char* appdata = nullptr;
    size_t sz = 0;
    _dupenv_s(&appdata, &sz, "APPDATA");
    if (appdata) {
        storage_path = fs::path(appdata) / "Cursor" / "User" / "globalStorage" / "storage.json";
        free(appdata);
    }
    #elif defined(__APPLE__)
    char* home = getenv("HOME");
    if (home) {
        storage_path = fs::path(home) / "Library" / "Application Support" / "Cursor" / "User" / "globalStorage" / "storage.json";
    }
    #elif defined(__linux__)
    char* home = getenv("HOME");
    if (home) {
        storage_path = fs::path(home) / ".config" / "Cursor" / "User" / "globalStorage" / "storage.json";
    }
    #else
    throw std::runtime_error("Unsupported operating system");
    #endif
    
    return storage_path;
}

void reset_cursor_id() {
    fs::path storage_file = get_storage_file();
    
    // Create parent directories if they don't exist
    fs::create_directories(storage_file.parent_path());
    
    // Backup the file if it exists
    backup_file(storage_file);
    
    // Load or create storage data
    json data;
    if (fs::exists(storage_file)) {
        std::ifstream file(storage_file);
        if (file.is_open()) {
            file >> data;
            file.close();
        }
    }
    
    // Generate new IDs
    std::string machine_id = generate_random_hex(64);
    std::string mac_machine_id = generate_random_hex(64);
    std::string dev_device_id = generate_uuid();
    
    // Update JSON data
    data["telemetry.machineId"] = machine_id;
    data["telemetry.macMachineId"] = mac_machine_id;
    data["telemetry.devDeviceId"] = dev_device_id;
    
    // Write updated data back to file
    std::ofstream out_file(storage_file);
    if (out_file.is_open()) {
        out_file << std::setw(2) << data << std::endl;
        out_file.close();
    } else {
        throw std::runtime_error("Failed to open storage file for writing");
    }
    
    // Print success message
    std::cout << "🎉 Device IDs have been successfully reset. The new device IDs are: \n" << std::endl;
    
    json result;
    result["machineId"] = machine_id;
    result["macMachineId"] = mac_machine_id;
    result["devDeviceId"] = dev_device_id;
    
    std::cout << std::setw(2) << result << std::endl;
}

int main() {
    try {
        reset_cursor_id();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}