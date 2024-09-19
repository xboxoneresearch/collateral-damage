#ifndef _WINRT_H
#define _WINRT_H

#define _CRT_SECURE_NO_WARNINGS 1

#include <stdio.h>
#include <Windows.h>
#include <initguid.h>
#include <roapi.h>
#include <Windows.ui.notifications.h>
#include <windows.ui.popups.h>
#include <WinSock2.h>

int show_toast();
int show_message_dialog(PCWSTR dialogTitle, PCWSTR dialogContent);

#endif