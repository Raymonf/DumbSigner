#pragma once
#include <string>
#include <vector>

std::string make_uuid();
std::string temporary_directory();
std::vector<unsigned char> readFile(const char* filename);
std::string replace_all(const std::string& str, const std::string& find, const std::string& replace);
std::string StringFromWideString(std::wstring wideString);
std::wstring WideStringFromString(std::string string);
