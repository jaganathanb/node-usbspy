#include <nan.h>
#include <iostream>
#include <string>
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

typedef struct Device_t {
	int deviceNumber;
	int deviceStatus;
	std::string serialNumber;
	std::string productId;
	std::string vendorId;
	std::string driveLetter;

	Device_t() {
		key = NULL;
	}

	~Device_t() {
		if (this->key != NULL) {
			delete this->key;
		}
	}

	void SetKey(const char* key) {
		if (this->key != NULL) {
			delete this->key;
		}
		this->key = new char[strlen(key) + 1];
		memcpy(this->key, key, strlen(key) + 1);
	}

	char* GetKey() {
		return this->key;
	}

private:
	char* key;

} Device;

typedef enum  DeviceStatus_t {
	Disconnect = 0,
	Connect
} DeviceStatus;

Device *PopulateAvailableUSBDeviceList(bool adjustDeviceList);
DWORD GetUSBDriveDetails(UINT nDriveNumber IN, Device *device OUT);

DWORD WINAPI SpyingThread();

void StartSpying();

bool HasDevice(const char *key);
void MapDeviceProps(Device *destiDevice, Device *sourceDevice);
Device *GetDevice(const char *key);
void RemoveDevice(Device *item);
void AddDevice(const char *key, Device *item);