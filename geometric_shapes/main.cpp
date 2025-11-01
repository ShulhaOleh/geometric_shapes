#include <iostream>
#include <cmath>
#include <thread>
#include <string>
#include <atomic>
#include <mutex>
#include <windows.h>
#include <conio.h>
#include <sstream>
#include <vector>

#include "shapes.h"
#include "input_handler.h"

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

// Gets current console dimensions
// Returns true if successful, updates width and height parameters
bool get_console_size(int& width, int& height) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        return true;
    }

    return false;
}

int main()
{
    int width = 120;
    int height = 30;

    // If the code is running on Windows, data from 
    // the console window is taken and set as the resolution
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }

    // Hide cursor
    std::cout << "\x1b[?25l";

    // For console's input line
    height -= 4;

    float aspect = (float)width / (float)height;
    float pixel_aspect = 11.0f / 24.0f;
    int fps = 120; // Set lower number for better performance
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

    auto frame_duration = std::chrono::milliseconds(1000 / fps);

    for (long t = 0; running && t < 1000000; t++) {
        int new_width = width;
        int new_height = height;

        // Check for console size changes
        if (!get_console_size(new_width, new_height)) {
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

        angle += rotation_speed.load() * aspect;

        for (int i = 0; i < width; i++) {
            for (int j = 0; j < height; j++) {
                // Coordinates will be from -1 to 1
                float x = (float)i / width * 2.0f - 1.0f;
                float y = (float)j / height * 2.0f - 1.0f;

                x *= aspect * pixel_aspect;

                x += sin(t * speed);

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

        // Display Frame
        std::cout << "\x1b[H";
        std::cout << screen;

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
        std::cout << status;
        for (int i = status.length(); i < width; i++) std::cout << " ";
        std::cout << "\n";

        std::cout << "Shapes: square s [size] | circle c [radius] | rectangle r [w] [h]";
        for (int i = 68; i < width; i++) std::cout << " ";
        std::cout << "\n";

        std::string actions = "Actions: ";
        if (shape == 0)
            actions += "size sz [val]";
        else if (shape == 1)
            actions += "radius rad [val]";
        else if (shape == 2)
            actions += "dim d [w] [h]";
        actions += " | speed sp [val] | char ch [c] | quit q";

        std::cout << actions;
        for (int i = actions.length(); i < width; i++) std::cout << " ";
        std::cout << "\n";

        std::lock_guard<std::mutex> lock(input_mutex);
        std::cout << "> " << input_buffer;
        for (int i = input_buffer.length(); i < width - 2; i++) std::cout << " ";

        std::cout.flush();
        std::this_thread::sleep_for(frame_duration);
    }

    delete[] screen;

    std::cout << "\x1b[2J\x1b[H";
    std::cout << "\x1b[?25h";
    std::cout << "Program terminated.\n";
    return 0;
}
