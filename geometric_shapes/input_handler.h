#pragma once

#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <string>
#include <vector>

bool try_parse_float(const std::string& str, float& result, float min_val, float max_val);

void handle_square_command(const std::vector<std::string>& parts);
void handle_circle_command(const std::vector<std::string>& parts);
void handle_rectangle_command(const std::vector<std::string>& parts);
void handle_speed_command(const std::vector<std::string>& parts);
void handle_char_command(const std::vector<std::string>& parts);
void process_command(const std::string& command);

void handle_keypress(char ch);
void input_thread();

#endif
