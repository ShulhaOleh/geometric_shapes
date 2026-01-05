#include <atomic>
#include <mutex>
#include <sstream>
#include <thread>
#include <chrono>
#include <conio.h>
#include <unordered_map>
#include <functional>

#include "input_handler.h"

extern std::atomic<bool> running;
extern std::atomic<int> current_shape;
extern std::atomic<float> shape_size;
extern std::atomic<float> shape_width;
extern std::atomic<float> shape_height;
extern std::atomic<float> rotation_speed;
extern std::mutex shape_mutex;
extern std::mutex input_mutex;
extern std::string input_buffer;
extern std::atomic<char> draw_char;
extern std::atomic<int> max_fps;

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
    std::lock_guard<std::mutex> shape_lock(shape_mutex);
    current_shape = 0;

    if (parts.size() <= 1) return;

    float size;
    
    if (try_parse_float(parts[1], size, 0.0f, 2.0f))
        shape_size = size;
}

// Command handler: switches to circle shape
void handle_circle_command(const std::vector<std::string>& parts) {
    std::lock_guard<std::mutex> shape_lock(shape_mutex);
    current_shape = 1;

    if (parts.size() <= 1) return;
    
    float radius;

    if (try_parse_float(parts[1], radius, 0.0f, 2.0f))
        shape_size = radius;
}

// Command handler: switches to rectangle shape
void handle_rectangle_command(const std::vector<std::string>& parts) {
    std::lock_guard<std::mutex> shape_lock(shape_mutex);
    current_shape = 2;

    float width;
    float height;

    if (parts.size() > 1 && try_parse_float(parts[1], width, 0.0f, 2.0f))
        shape_width = width;

    if (parts.size() > 2 && try_parse_float(parts[2], height, 0.0f, 2.0f))
        shape_height = height;
}

// Command handler: switches to oval shape
void handle_oval_command(const std::vector<std::string>& parts) {
    std::lock_guard<std::mutex> shape_lock(shape_mutex);
    current_shape = 3;

    float width;
    float height;

    if (parts.size() > 1 && try_parse_float(parts[1], width, 0.0f, 2.0f))
        shape_width = width;
    if (parts.size() > 2 && try_parse_float(parts[2], height, 0.0f, 2.0f))
        shape_height = height;
}

// Command handler: adjusts rotation speed (NO shape change)
void handle_speed_command(const std::vector<std::string>& parts) {
    float speed;
    if (parts.size() > 1 && try_parse_float(parts[1], speed, 0.0f, 0.1f))
        rotation_speed = speed;
}

// Command handler: changes drawing symbol (NO shape change)
void handle_char_command(const std::vector<std::string>& parts) {
    if (parts.size() > 1 && !parts[1].empty())
        draw_char = parts[1][0];
}

// Command handler: updates square size ONLY (does NOT change shape)
void handle_size_command(const std::vector<std::string>& parts) {
    if (parts.size() < 2) return;

    // Thread-safe check: only modify if current shape is square
    std::lock_guard<std::mutex> shape_lock(shape_mutex);
    if (current_shape.load() != 0) return;  // Not a square, ignore

    float size;
    if (try_parse_float(parts[1], size, 0.0f, 2.0f)) {
        shape_size = size;
    }
}

// Command handler: updates circle radius ONLY (does NOT change shape)
void handle_radius_command(const std::vector<std::string>& parts) {
    if (parts.size() < 2) return;

    // Thread-safe check: only modify if current shape is circle
    std::lock_guard<std::mutex> shape_lock(shape_mutex);
    if (current_shape.load() != 1) return;  // Not a circle, ignore

    float radius;
    if (try_parse_float(parts[1], radius, 0.0f, 2.0f)) {
        shape_size = radius;
    }
}

// Command handler: updates rectangle dimensions ONLY
void handle_dimensions_command(const std::vector<std::string>& parts) {
    if (parts.size() < 2) return;

    // Thread-safe check: only modify if current shape is rectangle
    std::lock_guard<std::mutex> shape_lock(shape_mutex);
    if (current_shape.load() != 2 && current_shape.load() != 3) return; // Not a rectangle or oval, ignore

    float width, height;
    if (parts.size() > 1 && try_parse_float(parts[1], width, 0.0f, 2.0f))
        shape_width = width;
    if (parts.size() > 2 && try_parse_float(parts[2], height, 0.0f, 2.0f))
        shape_height = height;
}

// Command handler: change maximum FPS
void handle_max_fps_command(const std::vector<std::string>& parts) {
    if (parts.size() < 2) return;

    try {
        int fps = std::stoi(parts[1]);

        if (fps >= 0) max_fps = fps;
    }
    catch (...) { }
}


// Main command processor: parses and routes commands to appropriate handlers
void process_command(const std::string& command) {
    auto parts = split(command, ' ');
    if (parts.empty()) return;

    std::string cmd = parts[0];

    // Command lookup table (initialized once, thread-safe)
    static const auto command_table = [] {
        std::unordered_map<std::string, std::function<void(const std::vector<std::string>&)>> map;

        auto register_command = [&](auto func, std::initializer_list<const char*> names) {
            for (auto name : names)
                map[name] = func;
            };

        // Shape-changing commands (handle their own locking)
        register_command(handle_square_command, { "square", "s" });
		register_command(handle_oval_command, { "oval", "o" });
        register_command(handle_circle_command, { "circle", "c" });
        register_command(handle_rectangle_command, { "rectangle", "r" });

        // Shape-specific parameter commands (REQUIRE correct shape, handle own locking)
        register_command(handle_size_command, { "size", "sz" });      // Only for square
        register_command(handle_radius_command, { "radius", "rad" }); // Only for circle
        register_command(handle_dimensions_command, { "dim", "d" });  // Only for rectangle

        // Global parameter commands (NO shape dependency, NO locking needed)
        register_command(handle_speed_command, { "speed", "sp" });
        register_command(handle_char_command, { "char", "ch" });

        // Utility commands
        register_command(handle_max_fps_command, { "maxfps", "fps", "mf" });
        register_command([](const auto&) { running = false; }, { "quit", "q" });

        return map;
    }();

    auto it = command_table.find(cmd);
    if (it != command_table.end())
        it->second(parts);
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