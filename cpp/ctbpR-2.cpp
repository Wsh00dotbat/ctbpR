#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <uuid/uuid.h>
#include <boost/filesystem.hpp>
#include <nlohmann/json.hpp>

namespace fs = boost::filesystem;
using json = nlohmann::json;

// Function to create a backup of the file
std::string backup_file(const fs::path& file_path) {
    if (fs::exists(file_path)) {
        std::time_t now = std::time(nullptr);
        char timestamp[20];
        std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", std::localtime(&now));
        fs::path backup_path = file_path.string() + ".backup_" + timestamp;
        fs::copy_file(file_path, backup_path, fs::copy_option::overwrite_if_exists);
        std::cout << "Backup created at: " << backup_path << std::endl;
        return backup_path.string();
    }
    return "";
}

// Function to determine the storage file location based on the operating system
fs::path get_storage_file() {
    std::string system = 
#ifdef _WIN32
    "Windows";
#elif __APPLE__
    "Darwin";
#elif __linux__
    "Linux";
#else
    "Unknown";
#endif

    if (system == "Windows") {
        char* appdata = std::getenv("APPDATA");
        if (appdata) {
            return fs::path(appdata) / "Cursor" / "User" / "globalStorage" / "storage.json";
        }
    } else if (system == "Darwin") {
        return fs::path(std::getenv("HOME")) / "Library" / "Application Support" / "Cursor" / "User" / "globalStorage" / "storage.json";
    } else if (system == "Linux") {
        return fs::path(std::getenv("HOME")) / ".config" / "Cursor" / "User" / "globalStorage" / "storage.json";
    } else {
        throw std::runtime_error("Unsupported operating system: " + system);
    }
    return fs::path();
}

// Function to generate a random hex string
std::string generate_random_hex(size_t length) {
    std::stringstream ss;
    for (size_t i = 0; i < length; ++i) {
        ss << std::hex << (rand() % 256);
    }
    return ss.str();
}

// Function to generate a UUID
std::string generate_uuid() {
    uuid_t uuid;
    uuid_generate_random(uuid);
    char uuid_str[37];
    uuid_unparse(uuid, uuid_str);
    return std::string(uuid_str);
}

// Function to reset Cursor device IDs
void reset_cursor_id() {
    try {
        fs::path storage_file = get_storage_file();
        fs::create_directories(storage_file.parent_path());

        // Create backup before modifications
        backup_file(storage_file);

        json data;
        if (fs::exists(storage_file)) {
            std::ifstream file(storage_file);
            file >> data;
            std::cout << "Loading configuration from " << storage_file << std::endl;
        } else {
            std::cout << "Storage file not found at " << storage_file << ". Creating new configuration." << std::endl;
        }

        // Generate new random IDs
        std::string machine_id = generate_random_hex(32);
        std::string mac_machine_id = generate_random_hex(32);
        std::string dev_device_id = generate_uuid();

        // Store the original values for logging
        std::map<std::string, std::string> original_ids = {
            {"machineId", data.value("telemetry.machineId", "Not set")},
            {"macMachineId", data.value("telemetry.macMachineId", "Not set")},
            {"devDeviceId", data.value("telemetry.devDeviceId", "Not set")}
        };

        // Update the device IDs
        data["telemetry.machineId"] = machine_id;
        data["telemetry.macMachineId"] = mac_machine_id;
        data["telemetry.devDeviceId"] = dev_device_id;

        // For Cursor v0.47.8, also reset these additional values
        data.erase("cursor.trialStartDate");
        data.erase("cursor.trialReminderShown");
        data.erase("cursor.trialExpired");

        // Write the updated configuration back to the file
        std::ofstream out_file(storage_file);
        out_file << data.dump(2);

        std::cout << "Device IDs successfully reset" << std::endl;

        std::cout << "\n🎉 Cursor v0.47.8 trial successfully reset!\n" << std::endl;
        std::cout << "Original device IDs were:" << std::endl;
        std::cout << json(original_ids).dump(2) << std::endl;
        std::cout << "\nNew device IDs are:" << std::endl;
        std::cout << json({
            {"machineId", machine_id},
            {"macMachineId", mac_machine_id},
            {"devDeviceId", dev_device_id}
        }).dump(2) << std::endl;
        std::cout << "\n✅ Additional trial parameters have been cleared" << std::endl;
        std::cout << "✅ Restart Cursor to apply changes" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error resetting Cursor IDs: " << e.what() << std::endl;
        std::cerr << "\n❌ Error: " << e.what() << std::endl;
        std::cerr << "Please check the script and try again." << std::endl;
    }
}

int main() {
    reset_cursor_id();
    return 0;
}