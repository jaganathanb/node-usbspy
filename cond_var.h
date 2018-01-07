#include <nan.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <dbt.h>
#include <tchar.h>
#include <atlstr.h>
#include <map>
#include <vector>
#include <algorithm>

DWORD WINAPI SpyingThread();
LRESULT CALLBACK SpyCallback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

Device *PopulateAvailableUSBDeviceList(bool adjustDeviceList);
DWORD GetUSBDriveDetails(UINT nDriveNumber IN, Device *device OUT);

DWORD WINAPI SpyingThread();

void StartSpying();