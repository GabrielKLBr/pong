#ifndef WIN_WRAPPER_H
#define WIN_WRAPPER_H
#include <stdbool.h>

typedef void* WinHandle;

WinHandle Win32_CreateWindow(int x, int y, int width, int height, WinHandle owner);
void Win32_MakeRound(WinHandle handle, int width, int height);
void Win32_SetWindowPos(WinHandle handle, int x, int y, int width, int height);
void Win32_SetTopMost(WinHandle handle, bool enable);
void Win32_SetVisibleMode(WinHandle handle, bool visible);
void Win32_SetForegroundWindow(WinHandle handle);
void Win32_UseDarkMode(bool useDarkMode);

void Win32_ApplyEmbeddedIcon(void);

void Win32_ProcessMessages(void);
void Win32_DestroyWindow(WinHandle handle);

#endif