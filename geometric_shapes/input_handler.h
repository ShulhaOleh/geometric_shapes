#pragma once

#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <string>
#include <vector>

// Safely parses a float from string with range validation
bool try_parse_float(const std::string& str, float& result, float min_val, float max_val);

// Command handler: switches to square shape
void handle_square_command(const std::vector<std::string>& parts);
// Command handler: switches to circle shape
void handle_circle_command(const std::vector<std::string>& parts);
// Command handler: switches to rectangle shape
void handle_rectangle_command(const std::vector<std::string>& parts);
// Command handler: adjusts rotation speed
void handle_speed_command(const std::vector<std::string>& parts);
// Command handler: changes drawing symbol
void handle_char_command(const std::vector<std::string>& parts);
// Main command processor: parses and routes commands to appropriate handlers
void process_command(const std::string& command);

void handle_keypress(char ch);
void input_thread();

#endif
