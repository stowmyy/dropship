#pragma once

//#define _WIN32_WINNT = 0x0601

#define half_pi 1.57079632679
#define NOMINMAX

#include <print>
using std::println;
#include <format>


#include <functional>
#include <optional>
#include <future>
#include <chrono>

using namespace std::chrono_literals;


// data structures
#include <string>
#include <vector>
#include <set>

// imgui
#define IMGUI_DEFINE_MATH_OPERATORS // https://github.com/ocornut/imgui/issues/2832
#include "imgui-docking/imgui.h"
#include "imgui-docking/imgui_internal.h" // ClosePopupsOverWindow, pushitemflag, popitemflag

// json
#include "json/json.hpp"

// Windows API
# define WIN32_LEAN_AND_MEAN 
#include <Windows.h>
// TODO which one? for ccomptr
#include <atlcomcli.h>
//#include <atlbase.h>


// util
#include "util/timer/timer.h";
