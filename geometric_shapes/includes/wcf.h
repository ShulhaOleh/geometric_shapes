#pragma once

namespace wcf {
	bool get_console_size(int& width, int& height);
	void set_window_resolution(int width, int height);
	void hide_cursor();
	void show_cursor();
}
