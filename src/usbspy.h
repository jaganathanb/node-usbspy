#ifndef USBSPY_H
#define USBSPY_H

#include <nan.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <dbt.h>
#include <tchar.h>
#include <algorithm>

#include "usbs.h"

#define MAX_THREAD_WINDOW_NAME 64

DWORD WINAPI SpyingThread();
LRESULT CALLBACK SpyCallback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

Device *PopulateAvailableUSBDeviceList(bool adjustDeviceList);
DWORD GetUSBDriveDetails(UINT nDriveNumber IN, Device *device OUT);

void StartSpying();

#endif