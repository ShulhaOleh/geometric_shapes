#include <iostream>
#include <cmath>
#include <thread>
#include <string>
#include <atomic>
#include <mutex>
#include <conio.h>
#include <sstream>
#include <vector>

#include "shapes.h"
#include "input_handler.h"
#include "wcf.h"

std::atomic<bool> running(true);
std::atomic<int> current_shape(0);
std::mutex shape_mutex;
std::string input_buffer = "";
std::mutex input_mutex;

std::atomic<float> shape_size(0.5f);
std::atomic<float> shape_width(0.75f);
std::atomic<float> shape_height(0.375f);
std::atomic<float> rotation_speed(0.005f);
std::atomic<char> draw_char('*');


int main()
{
    int width = 120;
    int height = 30;

    // If the code is running on Windows, data from 
    // the console window is taken and set as the resolution
    void set_window_resolution(int width, int height);

    // For console's input line
    height -= 4;

    float aspect = (float)width / (float)height;
    float pixel_aspect = 11.0f / 24.0f;

    // 0 == unlimited fps
    // Set lower number for better performance
    int max_fps = 0;
    // fps count
    float actual_fps = 0.0f;
    float fps_update_timer = 0.0f;
    int frame_count = 0;

    float speed = 0.05f;
    float const PI = 3.14159265359f;
    float angle = PI / 4;

    // Initialize screen buffer with initial console size
    char* screen = nullptr;
    int screen_size = width * height;
    screen = new char[screen_size + 1];
    screen[screen_size] = '\0';

    std::thread input(input_thread);
    input.detach();

    std::cout << "\x1b[2J";

    auto last_time = std::chrono::high_resolution_clock::now();
    float time_accumulator = 0.0f;

    for (long t = 0; running; t++) {
        auto current_time = std::chrono::high_resolution_clock::now();
        float delta_time = std::chrono::duration<float>(current_time - last_time).count();
        last_time = current_time;
        time_accumulator += delta_time;

        frame_count++;
        fps_update_timer += delta_time;
        if (fps_update_timer >= 0.1f) {
            actual_fps = frame_count / fps_update_timer;
            frame_count = 0;
            fps_update_timer = 0.0f;
        }

        int new_width = width;
        int new_height = height;

        // Check for console size changes
        if (!wcf::get_console_size(new_width, new_height)) {
            // Failed to get size, use current dimensions
            new_width = width;
            new_height = height;
        }

        new_height -= 4; // Reserve space for UI elements

        // Reallocate screen buffer if size changed
        if (new_width != width || new_height != height) {
            width = new_width;
            height = new_height;

            delete[] screen;
            screen_size = width * height;
            screen = new char[screen_size + 1];
            screen[screen_size] = '\0';

            std::cout << "\x1b[2J";
        }

        angle += rotation_speed.load() * aspect * delta_time * 50.0f;

        for (int i = 0; i < width; i++) {
            for (int j = 0; j < height; j++) {
                // Coordinates will be from -1 to 1
                float x = (float)i / width * 2.0f - 1.0f;
                float y = (float)j / height * 2.0f - 1.0f;

                x *= aspect * pixel_aspect;

                x += sin(time_accumulator * speed * 50.0f);

                float xr = x * cos(angle) - y * sin(angle);
                float yr = x * sin(angle) + y * cos(angle);

                char pixel = ' ';

                {
                    std::lock_guard<std::mutex> lock(shape_mutex);
                    char ch = draw_char.load();
                    if (current_shape == 0) pixel = square(xr, yr, shape_size.load()) ? ch : ' ';
                    else if (current_shape == 1) pixel = circle(xr, yr, shape_size.load()) ? ch : ' ';
                    else if (current_shape == 2) pixel = rectangle(xr, yr, shape_width.load(), shape_height.load()) ? ch : ' ';
                }

                // If nothing else, draw an empty pixel
                screen[i + j * width] = pixel;
            }
        }

        std::ostringstream frame_buffer;

        // Display Frame
        frame_buffer << "\x1b[H";
        frame_buffer << screen;

        frame_buffer << "\x1b[" << (height + 1) << ";1H";

        std::string status = "Status: ";
        int shape = current_shape.load();
        if (shape == 0)
            status += "Square (size: " + std::to_string(shape_size.load()).substr(0, 4) + ")";
        else if (shape == 1)
            status += "Circle (radius: " + std::to_string(shape_size.load()).substr(0, 4) + ")";
        else if (shape == 2)
            status += "Rectangle (w: " + std::to_string(shape_width.load()).substr(0, 4) +
            ", h: " + std::to_string(shape_height.load()).substr(0, 4) + ")";

        status += " | Speed: " + std::to_string(rotation_speed.load()).substr(0, 5);
        status += " | Symbol: '" + std::string(1, draw_char.load()) + "'";
        status += " | FPS: " + std::to_string((int)actual_fps);
        frame_buffer << status;
        for (int i = status.length(); i < width; i++) frame_buffer << " ";
        frame_buffer << "\n";

        frame_buffer << "Shapes: square s [size] | circle c [radius] | rectangle r [w] [h]";
        for (int i = 68; i < width; i++) frame_buffer << " ";
        frame_buffer << "\n";

        std::string actions = "Actions: ";
        if (shape == 0)
            actions += "size sz [val]";
        else if (shape == 1)
            actions += "radius rad [val]";
        else if (shape == 2)
            actions += "dim d [w] [h]";
        actions += " | speed sp [val] | char ch [c] | quit q";

        frame_buffer << actions;
        for (int i = actions.length(); i < width; i++) frame_buffer << " ";
        frame_buffer << "\n";

        int input_length = 0;
        {
            std::lock_guard<std::mutex> lock(input_mutex);
            frame_buffer << "> " << input_buffer;
            input_length = input_buffer.length();
            for (int i = input_length; i < width - 2; i++) frame_buffer << " ";
        }

        frame_buffer << "\x1b[" << (height + 4) << ";" << (3 + input_length) << "H";

        wcf::hide_cursor();
        std::cout << frame_buffer.str() << std::flush;
        wcf::show_cursor();

        if (max_fps > 0) {
            auto frame_duration = std::chrono::milliseconds(1000 / max_fps);
            auto frame_end_time = last_time + frame_duration;

            auto now = std::chrono::high_resolution_clock::now();
            if (now < frame_end_time) {
                std::this_thread::sleep_for(frame_end_time - now);
            }
        }
    }

    delete[] screen;

    std::cout << "\x1b[2J\x1b[H";
    std::cout << "Program terminated.\n";
    return 0;
}
