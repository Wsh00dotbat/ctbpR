#!/usr/bin/env python3
'''
Cursor Trial Reset Tool (Updated for v0.47.8)

This script resets the device IDs in Cursor's configuration file to generate a new random device ID.

Repository: https://github.com/ultrasev/cursor-reset
Author: @ultrasev
Updated: 23/Mar/2025
'''

import json
import os
import shutil
import uuid
from datetime import datetime
from pathlib import Path
import platform

def backup_file(file_path: Path):
    """Create a timestamped backup of the given file."""
    if file_path.exists():
        backup_path = file_path.with_suffix(f".backup_{datetime.now().strftime('%Y%m%d_%H%M%S')}")
        shutil.copy2(file_path, backup_path)

def get_storage_file() -> Path:
    """Determine the storage file location based on the operating system."""
    system = platform.system()
    base_path = {
        "Windows": Path(os.getenv("APPDATA", "")) / "Cursor" / "User" / "globalStorage",
        "Darwin": Path.home() / "Library" / "Application Support" / "Cursor" / "User" / "globalStorage",
        "Linux": Path.home() / ".config" / "Cursor" / "User" / "globalStorage",
    }.get(system)
    
    if not base_path:
        raise OSError(f"Unsupported operating system: {system}")
    
    return base_path / "storage.json"

def reset_cursor_id():
    try:
        storage_file = get_storage_file()
        storage_file.parent.mkdir(parents=True, exist_ok=True)
        backup_file(storage_file)
        
        data = {}
        if storage_file.exists():
            with open(storage_file, 'r', encoding='utf-8') as f:
                try:
                    data = json.load(f)
                except json.JSONDecodeError:
                    print("⚠️ Warning: The storage file is corrupted. A new one will be created.")
        
        data.update({
            "telemetry.machineId": os.urandom(32).hex(),
            "telemetry.macMachineId": os.urandom(32).hex(),
            "telemetry.devDeviceId": str(uuid.uuid4()),
        })
        
        with open(storage_file, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2)
        
        print("🎉 Device IDs have been successfully reset. The new device IDs are: \n")
        print(json.dumps({
            "machineId": data["telemetry.machineId"],
            "macMachineId": data["telemetry.macMachineId"],
            "devDeviceId": data["telemetry.devDeviceId"],
        }, indent=2))
    except Exception as e:
        print(f"❌ An error occurred: {e}")
        
if __name__ == "__main__":
    reset_cursor_id()
