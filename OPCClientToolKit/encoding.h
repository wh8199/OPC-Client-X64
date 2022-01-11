#ifndef ENCODING_H
#define ENCODING_H

#include <string>
#include <windows.h>

extern std::string U2A(const std::wstring &str);
extern std::wstring A2U(const std::string &str);

#endif