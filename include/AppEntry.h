#pragma once

int AppEntry(char* cmd_line);

struct HINSTANCE__ { int unused; };
typedef struct HINSTANCE__* HINSTANCE;

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* lpCmdLine, int nShowCmd)
{
	return AppEntry(lpCmdLine);
}
