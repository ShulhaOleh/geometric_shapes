#include "input_handler.h"
#include <atomic>
#include <mutex>
#include <sstream>
#include <thread>
#include <chrono>
#include <conio.h>

extern std::atomic<bool> running;
extern std::atomic<int> current_shape;
extern std::atomic<float> shape_size;
extern std::atomic<float> shape_width;
extern std::atomic<float> shape_height;
extern std::atomic<float> rotation_speed;
extern std::mutex shape_mutex;
extern std::mutex input_mutex;
extern std::string input_buffer;

// Splits a string into parts by a separator
std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

// Safely parses a float from string with range validation
bool try_parse_float(const std::string& str, float& result, float min_val, float max_val) {
    try {
        float value = std::stof(str);
        if (value >= min_val && value <= max_val) {
            result = value;
            return true;
        }
    }
    catch (...) {
        return false;
    }
    return false;
}

// Command handler: switches to square shape
void handle_square_command(const std::vector<std::string>& parts) {
    current_shape = 0;

    if (parts.size() > 1) {
        float size;
        if (try_parse_float(parts[1], size, 0.0f, 2.0f)) {
            shape_size = size;
        }
    }
}

// Command handler: switches to circle shape
void handle_circle_command(const std::vector<std::string>& parts) {
    current_shape = 1;

    if (parts.size() > 1) {
        float radius;
        if (try_parse_float(parts[1], radius, 0.0f, 2.0f)) {
            shape_size = radius;
        }
    }
}

// Command handler: switches to rectangle shape
void handle_rectangle_command(const std::vector<std::string>& parts) {
    current_shape = 2;
    float width;
    float height;

    if (parts.size() > 1 && try_parse_float(parts[1], width, 0.0f, 2.0f)) 
        shape_width = width;

    if (parts.size() > 2 && try_parse_float(parts[2], height, 0.0f, 2.0f)) 
        shape_height = height;
}

// Command handler: adjusts rotation speed
void handle_speed_command(const std::vector<std::string>& parts) {
    float speed;
    if (parts.size() > 1 && try_parse_float(parts[1], speed, 0.0f, 0.1f))
        rotation_speed = speed;
}

// Main command processor: parses and routes commands to appropriate handlers
void process_command(const std::string& command) {
    auto parts = split(command, ' ');
    if (parts.empty()) return;

    std::string cmd = parts[0];

    std::lock_guard<std::mutex> shape_lock(shape_mutex);

    if (cmd == "square" || cmd == "s") {
        handle_square_command(parts);
    }
    else if (cmd == "circle" || cmd == "c") {
        handle_circle_command(parts);
    }
    else if (cmd == "rectangle" || cmd == "r") {
        handle_rectangle_command(parts);
    }
    else if (cmd == "speed" || cmd == "sp") {
        handle_speed_command(parts);
    }
    else if (cmd == "quit" || cmd == "q") {
        running = false;
    }
}

void handle_keypress(char ch) {
    std::lock_guard<std::mutex> lock(input_mutex);

    if (ch == '\r' || ch == '\n') {
        std::string command = input_buffer;
        input_buffer.clear();
        process_command(command);
    }
    else if ((ch == '\b' || ch == 127) && !input_buffer.empty())
        input_buffer.pop_back();
    else if (ch >= 32 && ch <= 126)
        input_buffer += ch;
}


// Input stream
void input_thread() {
    while (running) {
        if (_kbhit()) {
            char ch = _getch();
            handle_keypress(ch);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}