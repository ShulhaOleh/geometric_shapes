#include <iostream>
#include <cmath>
#include <thread>
#include <string>
#include <atomic>
#include <mutex>
#include "shapes.h"
#include <windows.h>
#include <conio.h>

std::atomic<bool> running(true);
std::atomic<int> current_shape(0);
std::mutex shape_mutex;
std::string input_buffer = "";
std::mutex input_mutex;

void input_thread() {
    while (running) {
        char ch;
        if (_kbhit()) {
            ch = _getch();
            std::lock_guard<std::mutex> lock(input_mutex);

            if (ch == '\r' || ch == '\n') {
                std::string command = input_buffer;
                input_buffer.clear();

                std::lock_guard<std::mutex> shape_lock(shape_mutex);
                if (command == "square" || command == "s") {
                    current_shape = 0;
                }
                else if (command == "circle" || command == "c") {
                    current_shape = 1;
                }
                else if (command == "rectangle" || command == "r") {
                    current_shape = 2;
                }
                else if (command == "quit" || command == "q") {
                    running = false;
                }
            }
            else if ((ch == '\b' || ch == 127) && !input_buffer.empty()) {
                input_buffer.pop_back();
            }
            else if (ch >= 32 && ch <= 126) {
                input_buffer += ch;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
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

    // For console's input line
    height -= 3;

    float aspect = (float)width / (float)height;
    float pixel_aspect = 11.0f / 24.0f;

    float speed = 0.1f;
    int fps = 120;

    float const PI = 3.14159265359f;

    float r = 0.5;
    float angle = PI / 4;
    float rotation_speed = 0.005f;

    char* screen = new char[width * height + 1];
    screen[width * height] = '\0';

    std::thread input(input_thread);
    input.detach();

    std::cout << "\x1b[2J";

    auto frame_duration = std::chrono::milliseconds(1000 / fps);

    for (long t = 0; running && t < 1000000; t++) {
        angle += rotation_speed * aspect;

        for (int i = 0; i < width; i++) {
            for (int j = 0; j < height; j++) {
                // Coordinates will be from -1 to 1
                float x = (float)i / width * 2.0f - 1.0f;
                float y = (float)j / height * 2.0f - 1.0f;

                x *= aspect * pixel_aspect;

                float xr = x * cos(angle) - y * sin(angle);
                float yr = x * sin(angle) + y * cos(angle);

                char pixel = ' ';

                {
                    std::lock_guard<std::mutex> lock(shape_mutex);
                    if (current_shape == 0) {
                        pixel = square(xr, yr, r) ? '*' : ' ';
                    }
                    else if (current_shape == 1) {
                        pixel = circle(xr, yr, r) ? 'O' : ' ';
                    }
                    else if (current_shape == 2) {
                        pixel = rectangle(xr, yr, r * 1.5f, r * 0.75f) ? '#' : ' ';
                    }
                }

                // If nothing else, draw an empty pixel
                screen[i + j * width] = pixel;
            }
        }

        // Display Frame
        std::cout << "\x1b[H";
        std::cout << screen;

        std::lock_guard<std::mutex> lock(input_mutex);
        std::cout << "> " << input_buffer;
        for (int i = input_buffer.length(); i < width - 9; i++) std::cout << " ";

        std::cout.flush();
        std::this_thread::sleep_for(frame_duration);
    }

    delete[] screen;

    std::cout << "\x1b[2J\x1b[H";
    std::cout << "Program terminated.\n";
    return 0;
}
