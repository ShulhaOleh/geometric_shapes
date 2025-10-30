#include <iostream>
#include <cmath>
#include <thread>

#include "shapes.h"

#ifdef _WIN32
#include <windows.h>
#endif

int main()
{
    // Screen resolution
    int width = 120;
    int height = 30;

	// If the code is running on Windows, data from 
	// the console window is taken and set as the resolution
#ifdef _WIN32
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
		width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
		height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	}
#endif

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

	auto frame_duration = std::chrono::milliseconds(1000 / fps);

	for (long t = 0; t < 1000000; t++) {
		angle += rotation_speed * aspect;

		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
				// Coordinates will be from -1 to 1
				float x = (float)i / width * 2.0f - 1.0f;
				float y = (float)j / height * 2.0f - 1.0f;

				x *= aspect * pixel_aspect;

				float xr = x * cos(angle) - y * sin(angle);
				float yr = x * sin(angle) + y * cos(angle);

				char pixel = square(xr, yr, r) ? '*' : ' ';

				// If nothing else, draw an empty pixel
				screen[i + j * width] = pixel;
			}
		}

		// Display Frame
		std::cout << "\x1b[H";
		std::cout << screen;
		std::cout.flush();
		std::this_thread::sleep_for(std::chrono::milliseconds(frame_duration));
	}

	delete[] screen;
	return 0;
}
