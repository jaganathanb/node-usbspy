#ifndef USBSPY_H
#define USBSPY_H

#include <nan.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <tchar.h>

#include "usbs.h"

#define DBT_DEVICEARRIVAL	0x8000
#define DBT_DEVICEREMOVECOMPLETE	0x8004
#define DBT_DEVTYP_DEVICEINTERFACE 5
#define MAX_THREAD_WINDOW_NAME 64

#define _TEST_NODE_

DWORD WINAPI SpyingThread();
LRESULT CALLBACK SpyCallback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void PopulateAvailableUSBDeviceList();
DWORD GetUSBDriveDetails(UINT drive_number IN, Device *device OUT);

void StartSpying();

#endif
