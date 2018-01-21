#ifndef USBSPY_H
#define USBSPY_H

#include <nan.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>

#include "usbs.h"

#define DBT_DEVICEARRIVAL	0x8000
#define DBT_DEVICEREMOVECOMPLETE	0x8004
#define DBT_DEVTYP_DEVICEINTERFACE 5
#define MAX_THREAD_WINDOW_NAME 64

typedef struct _DEV_BROADCAST_DEVICEINTERFACE_W {
	DWORD dbcc_size;
	DWORD dbcc_devicetype;
	DWORD dbcc_reserved;
	GUID dbcc_classguid;
	wchar_t dbcc_name[1];
} *PDEV_BROADCAST_DEVICEINTERFACE;

typedef struct _DEV_BROADCAST_HDR {
	DWORD dbch_size;
	DWORD dbch_devicetype;
	DWORD dbch_reserved;
} DEV_BROADCAST_HDR, *PDEV_BROADCAST_HDR;

typedef struct _DEV_BROADCAST_DEVICEINTERFACE_A {
	DWORD dbcc_size;
	DWORD dbcc_devicetype;
	DWORD dbcc_reserved;
	GUID dbcc_classguid;
	char dbcc_name[1];
} DEV_BROADCAST_DEVICEINTERFACE_A, *PDEV_BROADCAST_DEVICEINTERFACE_A;

#define _TEST_NODE_

DWORD WINAPI SpyingThread();
LRESULT CALLBACK SpyCallback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void PopulateAvailableUSBDeviceList();
DWORD GetUSBDriveDetails(UINT drive_number IN, Device *device OUT);

void StartSpying();

#endif
