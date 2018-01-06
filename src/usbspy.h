#ifndef _USBSPY_H
#define _USBSPY_H

#include <mutex>
#include <iostream>
#include <atlstr.h>
#include <comdef.h>
#include <map>
#include <Dbt.h>

#include <nan.h>

#ifndef UNICODE
typedef std::string String;
#else
typedef std::wstring String;
#endif

#define MAX_THREAD_WINDOW_NAME 64

typedef struct Device_t
{
	int deviceNumber;
	int deviceStatus;
	String serialNumber;
	String productId;
	String vendorId;
	String driveLetter;

	Device_t()
	{
		key = NULL;
	}

	~Device_t()
	{
		if (this->key != NULL)
		{
			delete this->key;
		}
	}

	void SetKey(const char *key)
	{
		if (this->key != NULL)
		{
			delete this->key;
		}
		this->key = new char[strlen(key) + 1];
		memcpy(this->key, key, strlen(key) + 1);
	}

	char *GetKey()
	{
		return this->key;
	}

  private:
	char *key;

} Device;

typedef enum DeviceStatus_t {
	Disconnect = 0,
	Connect
} DeviceStatus;

Device *PopulateAvailableUSBDeviceList(bool adjustDeviceList);
DWORD GetUSBDriveDetails(UINT nDriveNumber IN, Device *device OUT);

DWORD WINAPI SpyingThread();
LRESULT CALLBACK SpyCallback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif
