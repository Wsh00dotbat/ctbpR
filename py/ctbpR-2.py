#!/usr/bin/env python3
'''
Cursor Trial Reset Tool
This script resets the device IDs in Cursor's configuration file to generate a new random device ID.
Repository: https://github.com/ultrasev/cursor-reset
Author: @ultrasev
Updated: 23/Mar/2025
Original: 10/Dec/2024
Version: Compatible with Cursor v0.47.8
'''
import json
import os
import shutil
import uuid
import logging
from datetime import datetime
from pathlib import Path
import platform

# Set up logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    datefmt='%Y-%m-%d %H:%M:%S'
)
logger = logging.getLogger(__name__)

def backup_file(file_path: Path):
    """Create a timestamped backup of the given file."""
    if file_path.exists():
        backup_path = f"{file_path}.backup_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        shutil.copy2(file_path, backup_path)
        logger.info(f"Backup created at: {backup_path}")
        return backup_path
    return None

def get_storage_file():
    """Determine the storage file location based on the operating system."""
    system = platform.system()
    
    if system == "Windows":
        return Path(os.getenv("APPDATA")) / "Cursor" / "User" / "globalStorage" / "storage.json"
    elif system == "Darwin":  # macOS
        return Path(os.path.expanduser("~")) / "Library" / "Application Support" / "Cursor" / "User" / "globalStorage" / "storage.json"
    elif system == "Linux":
        return Path(os.path.expanduser("~")) / ".config" / "Cursor" / "User" / "globalStorage" / "storage.json"
    else:
        raise OSError(f"Unsupported operating system: {system}")

def reset_cursor_id():
    """Reset Cursor device IDs to extend trial period for v0.47.8."""
    try:
        storage_file = get_storage_file()
        storage_file.parent.mkdir(parents=True, exist_ok=True)
        
        # Create backup before modifications
        backup_file(storage_file)
        
        # Load existing data or create new if file doesn't exist
        if not storage_file.exists():
            logger.info(f"Storage file not found at {storage_file}. Creating new configuration.")
            data = {}
        else:
            logger.info(f"Loading configuration from {storage_file}")
            with open(storage_file, 'r', encoding='utf-8') as f:
                data = json.load(f)
        
        # Generate new random IDs
        machine_id = os.urandom(32).hex()
        mac_machine_id = os.urandom(32).hex()
        dev_device_id = str(uuid.uuid4())
        
        # Store the original values for logging
        original_ids = {
            "machineId": data.get("telemetry.machineId", "Not set"),
            "macMachineId": data.get("telemetry.macMachineId", "Not set"),
            "devDeviceId": data.get("telemetry.devDeviceId", "Not set")
        }
        
        # Update the device IDs
        data["telemetry.machineId"] = machine_id
        data["telemetry.macMachineId"] = mac_machine_id
        data["telemetry.devDeviceId"] = dev_device_id
        
        # For Cursor v0.47.8, also reset these additional values
        data.pop("cursor.trialStartDate", None)
        data.pop("cursor.trialReminderShown", None)
        data.pop("cursor.trialExpired", None)
        
        # Write the updated configuration back to the file
        with open(storage_file, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2)
        
        logger.info("Device IDs successfully reset")
        
        print("\n🎉 Cursor v0.47.8 trial successfully reset!\n")
        print("Original device IDs were:")
        print(json.dumps(original_ids, indent=2))
        print("\nNew device IDs are:")
        print(json.dumps(
            {
                "machineId": machine_id,
                "macMachineId": mac_machine_id,
                "devDeviceId": dev_device_id,
            },
            indent=2
        ))
        print("\n✅ Additional trial parameters have been cleared")
        print("✅ Restart Cursor to apply changes")
        
    except Exception as e:
        logger.error(f"Error resetting Cursor IDs: {str(e)}")
        print(f"\n❌ Error: {str(e)}")
        print("Please check the script and try again.")

if __name__ == "__main__":
    reset_cursor_id()