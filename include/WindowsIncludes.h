#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#ifndef STRICT
#	define STRICT
#endif

#include <Windows.h>

#undef CreateFont
#undef CreateDirectory
#undef DialogBox
#undef DrawText
#undef MoveFile
#undef far
#undef near
#undef small
#undef IN
#undef OUT
#undef ERROR
