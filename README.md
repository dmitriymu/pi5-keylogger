# Raspberry Pi 5 Keylogger

A hardware-level keylogger for Raspberry Pi 5 that captures keystrokes directly from the input device with proper handling of modifier keys.

## Features

* 🔍 Captures all keyboard input events from a specified device
* ⌨️ Correctly handles uppercase letters and special characters (Shift key combinations)
* 💾 Logs keystrokes to a file with clean formatting
* 🚀 Uses optimized ARM64 assembly for system calls
* 🔒 Requests exclusive access to the keyboard device
* 🔧 Implements direct hardware state queries for reliable modifier key detection

## Requirements

* Raspberry Pi 5 running Linux
* GCC/G++ compiler
* Root privileges (sudo)
* Linux input device for your keyboard

## Installation

1. Clone the repository:
   ```
   git clone https://github.com/dmitriymu/pi5-keylogger.git
   cd pi5-keylogger
   ```
2. Compile the program:
   ```
   g++ -o keylogger keylogger.cpp
   ```

## Usage

1. Identify your keyboard's event file:
   ```
   ls -l /dev/input/by-path/ | grep -i kbd
   ```
2. Update the `DEVICE` macro in the code to match your keyboard's event number.
3. Run the keylogger with sudo:
   ```
   sudo ./keylogger
   ```
4. Keystrokes will be logged to `keystrokes.log` in the current directory.
5. To stop, press Ctrl+C in the terminal running the keylogger.

## How It Works

The program works by:

1. Opening the specified input device file
2. Using `ioctl()` to request exclusive access to the device
3. Reading raw input events using an optimized ARM assembly system call
4. Directly querying the hardware state of modifier keys
5. Translating key codes to characters based on the current shift state
6. Writing captured keystrokes to a log file

## Customization

* **Keymap** : Modify the `keymap` in the source code to change key mappings or add additional keys
* **Log file** : Change the log file path in the `main()` function
* **Input device** : Update the `DEVICE` macro to match your keyboard's event file

## Technical Details

* Uses Linux `input_event` structures to read keyboard events
* Implements direct hardware state queries via `EVIOCGKEY` ioctl
* Optimizes read operations with inline ARM64 assembly
* Maps Linux key codes to human-readable characters

## Limitations

* Requires root privileges to access input devices
* Must be run on Raspberry Pi 5 or compatible ARM64 device
* Only captures from a single input device
* Does not capture key combinations with modifiers other than Shift

## Legal Considerations

This tool is intended for educational purposes, security testing on your own devices, and legitimate system monitoring. Unauthorized keylogging may be illegal and unethical. Always:

* Get explicit consent before monitoring anyone's keystrokes
* Inform users if a system is being monitored
* Follow applicable laws regarding computer monitoring and privacy

## License

MIT License

## Author

Dmitriy M

---

 **Disclaimer** : This software is provided for educational and research purposes only. The author is not responsible for any misuse of this software.
