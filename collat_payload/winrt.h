#ifndef _WINRT_H
#define _WINRT_H

#include <Windows.h>

int show_toast();
int show_message_dialog(PCWSTR dialogTitle, PCWSTR dialogContent);

#endif