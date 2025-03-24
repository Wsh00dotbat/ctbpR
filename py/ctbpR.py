#!/usr/bin/env python3
'''
Cursor Trial Reset Tool v2.0

This script resets the device IDs in Cursor's configuration file to generate new random IDs.
Optimized for Cursor v0.47.8

Repository: https://github.com/ultrasev/cursor-reset
Author: @ultrasev
Created: 10/Dec/2024
Updated: 23/Mar/2025
'''

import json
import os
import shutil
import uuid
import logging
from datetime import datetime
from pathlib import Path
import platform
from typing import Dict, Optional

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('cursor_reset.log'),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger(__name__)

class CursorResetError(Exception):
    """Custom exception for Cursor reset operations"""
    pass

def backup_file(file_path: Path) -> Optional[Path]:
    """Create a timestamped backup of the given file."""
    try:
        if file_path.exists():
            timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
            backup_path = file_path.with_suffix(f'.backup_{timestamp}')
            shutil.copy2(file_path, backup_path)
            logger.info(f"Created backup at: {backup_path}")
            return backup_path
        return None
    except Exception as e:
        logger.error(f"Failed to create backup: {str(e)}")
        raise CursorResetError(f"Backup creation failed: {str(e)}")

def get_storage_file() -> Path:
    """Determine the storage file location based on the operating system."""
    system = platform.system()
    base_path = {
        "Windows": Path(os.getenv("APPDATA", "")) / "Cursor",
        "Darwin": Path(os.path.expanduser("~")) / "Library" / "Application Support" / "Cursor",
        "Linux": Path(os.path.expanduser("~")) / ".config" / "Cursor"
    }.get(system)

    if not base_path:
        raise CursorResetError(f"Unsupported operating system: {system}")

    return base_path / "User" / "globalStorage" / "storage.json"

def generate_new_ids() -> Dict[str, str]:
    """Generate new device IDs."""
    return {
        "machineId": os.urandom(32).hex(),
        "macMachineId": os.urandom(32).hex(),
        "devDeviceId": str(uuid.uuid4())
    }

def reset_cursor_id() -> None:
    """Reset Cursor device IDs."""
    try:
        storage_file = get_storage_file()
        logger.info(f"Targeting storage file: {storage_file}")

        # Ensure directory exists
        storage_file.parent.mkdir(parents=True, exist_ok=True)
        
        # Create backup
        backup_path = backup_file(storage_file)

        # Load existing data or initialize new
        data = {}
        if storage_file.exists():
            try:
                with open(storage_file, 'r', encoding='utf-8') as f:
                    data = json.load(f)
            except json.JSONDecodeError:
                logger.warning("Invalid JSON in storage file, starting fresh")
                data = {}

        # Generate and apply new IDs
        new_ids = generate_new_ids()
        data["telemetry"] = data.get("telemetry", {})
        data["telemetry"].update({
            "machineId": new_ids["machineId"],
            "macMachineId": new_ids["macMachineId"],
            "devDeviceId": new_ids["devDeviceId"]
        })

        # Write updated data
        try:
            with open(storage_file, 'w', encoding='utf-8') as f:
                json.dump(data, f, indent=2, ensure_ascii=False)
            logger.info("Successfully updated storage file")
        except Exception as e:
            logger.error(f"Failed to write storage file: {str(e)}")
            # Restore backup if it exists
            if backup_path and backup_path.exists():
                shutil.copy2(backup_path, storage_file)
                logger.info("Restored backup due to write failure")
            raise

        # Print success message
        print("🎉 Device IDs have been successfully reset!")
        print("New device IDs:")
        print(json.dumps(new_ids, indent=2))

    except CursorResetError as e:
        logger.error(f"Reset failed: {str(e)}")
        print(f"❌ Error: {str(e)}")
        raise
    except Exception as e:
        logger.error(f"Unexpected error: {str(e)}")
        print(f"❌ Unexpected error occurred: {str(e)}")
        raise

def main():
    """Main entry point with basic argument handling."""
    try:
        print("Cursor Trial Reset Tool v2.0")
        print("---------------------------")
        reset_cursor_id()
    except Exception as e:
        logger.critical(f"Program terminated: {str(e)}")
        print("\nFor more details, check cursor_reset.log")
        exit(1)

if __name__ == "__main__":
    main()