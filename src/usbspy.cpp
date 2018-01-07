#include <nan.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <dbt.h>
#include <tchar.h>
#include <algorithm>

#include "usbs.h"
#include "usbspy.h"

using namespace Nan; // NOLINT(build/namespaces)

std::mutex m;
std::condition_variable cv;
bool ready = false;

#define MAX_THREAD_WINDOW_NAME 64

#define __TEST_MODE__ 1

GUID GUID_DEVINTERFACE_USB_DEVICE = {
	0xA5DCBF10L,
	0x6530,
	0x11D2,
	0x90,
	0x1F,
	0x00,
	0xC0,
	0x4F,
	0xB9,
	0x51,
	0xED };

const typename AsyncProgressQueueWorker<Device>::ExecutionProgress *globalProgress;

template<typename T>
class ProgressQueueWorker : public AsyncProgressQueueWorker<T> {
public:
	ProgressQueueWorker(Callback *callback, Callback *progress) : AsyncProgressQueueWorker<T>(callback), progress(progress) {

	}

	~ProgressQueueWorker() {
		delete progress;
	}

	void Execute(const typename AsyncProgressQueueWorker<T>::ExecutionProgress& progress) {
		std::unique_lock<std::mutex> lk(m);

		processData(progress);

		while (ready) {
			cv.wait(lk);
		}

		lk.unlock();
		cv.notify_one();
	}

	void HandleProgressCallback(const T *data, size_t count) {
		HandleScope scope;
		v8::Local<v8::Object> obj = Nan::New<v8::Object>();


		Nan::Set(
			obj,
			Nan::New("deviceNumber").ToLocalChecked(),
			New<v8::Number>(data->deviceNumber));
		Nan::Set(
			obj,
			Nan::New("deviceStatus").ToLocalChecked(),
			New<v8::Number>(data->deviceStatus));
		Nan::Set(
			obj,
			Nan::New("vendorId").ToLocalChecked(),
			New<v8::String>(data->vendorId.c_str()).ToLocalChecked());
		Nan::Set(
			obj,
			Nan::New("serialNumber").ToLocalChecked(),
			New<v8::String>(data->serialNumber.c_str()).ToLocalChecked());
		Nan::Set(
			obj,
			Nan::New("productId").ToLocalChecked(),
			New<v8::String>(data->productId.c_str()).ToLocalChecked());
		Nan::Set(
			obj,
			Nan::New("driveLetter").ToLocalChecked(),
			New<v8::String>(data->driveLetter.c_str()).ToLocalChecked());

		v8::Local<v8::Value> argv[] = { obj };

#ifndef __TEST_MODE__
		progress->Call(1, argv);
#endif // __TEST_MODE__

	}

private:
	Callback * progress;
};

NAN_METHOD(SpyOn)
{
#ifdef __TEST_MODE__
	Callback *progress = new Callback();
	Callback *callback = new Callback();
#else 
	Callback *progress = new Callback(To<v8::Function>(info[0]).ToLocalChecked());
	Callback *callback = new Callback(To<v8::Function>(info[1]).ToLocalChecked());
#endif // __TEST_MODE__

	AsyncQueueWorker(new ProgressQueueWorker<Device>(callback, progress));
}

void StartSpying()
{
	{
		std::lock_guard<std::mutex> lk(m);
		ready = true;
		std::cout << "main() signals data ready for processing\n" << std::endl;
	}
	cv.notify_one();

#ifdef __TEST_MODE__
	New<v8::FunctionTemplate>(SpyOn)->GetFunction()->CallAsConstructor(0, {});
#endif // !__TEST_MODE__

}

NAN_METHOD(SpyOff)
{
	{
		std::lock_guard<std::mutex> lk(m);
		ready = false;
		std::cout << "main() signals data  not ready for processing\n" << std::endl;
	}
	cv.notify_one();
}

NAN_MODULE_INIT(Init)
{
	Set(target, New<v8::String>("spyOn").ToLocalChecked(), New<v8::FunctionTemplate>(SpyOn)->GetFunction());
	Set(target, New<v8::String>("spyOff").ToLocalChecked(), New<v8::FunctionTemplate>(SpyOff)->GetFunction());
	StartSpying();
}

void processData(const typename AsyncProgressQueueWorker<Device>::ExecutionProgress& progress) {
	globalProgress = &progress;

	PopulateAvailableUSBDeviceList(false);

	std::thread worker(SpyingThread);

#ifdef __TEST_MODE__
	worker.join();
#else
	worker.detach();
#endif // __TEST_MODE__

}

DWORD WINAPI SpyingThread()
{
	char className[MAX_THREAD_WINDOW_NAME];
	_snprintf_s(className, MAX_THREAD_WINDOW_NAME, "ListnerThreadUsbDetection_%d", GetCurrentThreadId());

	WNDCLASSA wincl = { 0 };
	wincl.hInstance = GetModuleHandle(0);
	wincl.lpszClassName = className;
	wincl.lpfnWndProc = SpyCallback;

	if (!RegisterClassA(&wincl))
	{
		DWORD le = GetLastError();
		printf("RegisterClassA() failed [Error: %x]\r\n", le);
		return 1;
	}

	HWND hwnd = CreateWindowExA(WS_EX_TOPMOST, className, className, 0, 0, 0, 0, 0, NULL, 0, 0, 0);
	if (!hwnd)
	{
		DWORD le = GetLastError();
		printf("CreateWindowExA() failed [Error: %x]\r\n", le);
		return 1;
	}

	DEV_BROADCAST_DEVICEINTERFACE_A notifyFilter = { 0 };
	notifyFilter.dbcc_size = sizeof(notifyFilter);
	notifyFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	notifyFilter.dbcc_classguid = GUID_DEVINTERFACE_USB_DEVICE;

	HDEVNOTIFY hDevNotify = RegisterDeviceNotificationA(hwnd, &notifyFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
	if (!hDevNotify)
	{
		DWORD le = GetLastError();
		printf("RegisterDeviceNotificationA() failed [Error: %x]\r\n", le);
		return 1;
	}
	std::cout << "before gets message \n" << std::endl;
	MSG msg;
	while (TRUE)
	{
		BOOL bRet = GetMessage(&msg, hwnd, 0, 0);
		if ((bRet == 0) || (bRet == -1))
		{
			break;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

DWORD GetUSBDriveDetails(UINT nDriveNumber IN, Device *device OUT)
{
	DWORD dwRet = NO_ERROR;

	// Format physical drive path (may be '\\.\PhysicalDrive0', '\\.\PhysicalDrive1' and so on).
	TCHAR szDrvName[260];
	//sprintf(szBuf, "\\\\?\\%c:", 'A' + drive);
	//strDrivePath.Format(_T("\\\\.\\%c:"), 'A' + nDriveNumber);

	_stprintf(szDrvName, _T("\\\\.\\%c:"), 'A' + nDriveNumber);

	// Get a handle to physical drive
	HANDLE hDevice = ::CreateFile(szDrvName, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, 0, NULL);

	if (INVALID_HANDLE_VALUE == hDevice)
		return ::GetLastError();

	// Set the input data structure
	STORAGE_PROPERTY_QUERY storagePropertyQuery;
	ZeroMemory(&storagePropertyQuery, sizeof(STORAGE_PROPERTY_QUERY));
	storagePropertyQuery.PropertyId = StorageDeviceProperty;
	storagePropertyQuery.QueryType = PropertyStandardQuery;

	// Get the necessary output buffer size
	STORAGE_DESCRIPTOR_HEADER storageDescriptorHeader = { 0 };
	DWORD dwBytesReturned = 0;
	if (!::DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
		&storagePropertyQuery, sizeof(STORAGE_PROPERTY_QUERY),
		&storageDescriptorHeader, sizeof(STORAGE_DESCRIPTOR_HEADER),
		&dwBytesReturned, NULL))
	{
		dwRet = ::GetLastError();
		::CloseHandle(hDevice);
		return dwRet;
	}

	// Alloc the output buffer
	const DWORD dwOutBufferSize = storageDescriptorHeader.Size;
	BYTE* pOutBuffer = new BYTE[dwOutBufferSize];
	ZeroMemory(pOutBuffer, dwOutBufferSize);

	// Get the storage device descriptor
	if (!::DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
		&storagePropertyQuery, sizeof(STORAGE_PROPERTY_QUERY),
		pOutBuffer, dwOutBufferSize,
		&dwBytesReturned, NULL))
	{
		dwRet = ::GetLastError();
		delete[]pOutBuffer;
		::CloseHandle(hDevice);
		return dwRet;
	}

	// Now, the output buffer points to a STORAGE_DEVICE_DESCRIPTOR structure
	// followed by additional info like vendor ID, product ID, serial number, and so on.
	STORAGE_DEVICE_DESCRIPTOR* pDeviceDescriptor = (STORAGE_DEVICE_DESCRIPTOR*)pOutBuffer;
	const DWORD dwSerialNumberOffset = pDeviceDescriptor->SerialNumberOffset;

	char value[260] = "";

	if (pDeviceDescriptor->ProductIdOffset != 0)
	{
		// Finally, get the serial number
		//device->productId = CString(pOutBuffer + pDeviceDescriptor->ProductIdOffset);

		sprintf(value, "%s", pOutBuffer + pDeviceDescriptor->ProductIdOffset);
		device->productId = std::string(value);
	}

	if (pDeviceDescriptor->VendorIdOffset != 0)
	{
		// Finally, get the serial number
		//device->vendorId = CString(pOutBuffer + pDeviceDescriptor->VendorIdOffset);
		sprintf(value, "%s", pOutBuffer + pDeviceDescriptor->VendorIdOffset);
		device->vendorId = std::string(value);
	}

	if (pDeviceDescriptor->SerialNumberOffset != 0)
	{
		// Finally, get the serial number
		//device->serialNumber = CString(pOutBuffer + pDeviceDescriptor->SerialNumberOffset);
		sprintf(value, "%s", pOutBuffer + pDeviceDescriptor->SerialNumberOffset);
		device->serialNumber = std::string(value);
	}

	// Do cleanup and return
	delete[]pOutBuffer;
	::CloseHandle(hDevice);
	return dwRet;
}

Device *PopulateAvailableUSBDeviceList(bool adjustDeviceList)
{
	// Get available drives we can monitor
	DWORD drives_bitmask = GetLogicalDrives();
	DWORD drive;
	int deviceNumber = 1;

	std::vector<const char *> keys;
	Device *device;

	for(drive = 0; drive < 32; ++drive)
	{
		if (drives_bitmask & (1 << drive)) {
			TCHAR driveLetter[] = { TEXT('A') + drive, TEXT(':'), TEXT('\\'), TEXT('\0') };
			if (GetDriveType(driveLetter) == DRIVE_REMOVABLE)
			{
				device = new Device();
				GetUSBDriveDetails(drive, device);

				const char *key = "";
				key = device->vendorId.append(device->productId).append(device->serialNumber).c_str();

				if (HasDevice(key))
				{
					keys.push_back(key);
					std::cout << "Device " << device->vendorId << " (" << device->productId << ")" << " is already there in the list!" << std::endl;
				}
				else {
					device->driveLetter = driveLetter;
					device->deviceStatus = (int)Connect;
					AddDevice(key, device);
					std::cout << "Device " << device->driveLetter << " (" << device->productId << ")" << " is been added!" << std::endl;
				}
			}
		}
	}

	if (adjustDeviceList)
	{
		Device *deviceToBeRemoved = new Device();
		device = new Device();

		deviceToBeRemoved = GetDeviceToBeRemoved(keys);

		if (deviceToBeRemoved)
		{
			MapDeviceProps(device, deviceToBeRemoved);
			RemoveDevice(deviceToBeRemoved);
			device->deviceStatus = (int)Disconnect;

			std::cout << "Device " << deviceToBeRemoved->driveLetter << " (" << deviceToBeRemoved->productId << ")" << " is been removed!" << std::endl;
		}
		delete deviceToBeRemoved;
		deviceToBeRemoved = NULL;
	}

	return device;
}

LRESULT CALLBACK SpyCallback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_DEVICECHANGE)
	{
		if (DBT_DEVICEARRIVAL == wParam || DBT_DEVICEREMOVECOMPLETE == wParam)
		{
			PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR)lParam;
			PDEV_BROADCAST_DEVICEINTERFACE pDevInf;

			if (pHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
			{
				pDevInf = (PDEV_BROADCAST_DEVICEINTERFACE)pHdr;
				Device *device;

				Sleep(4000);

				device = PopulateAvailableUSBDeviceList(DBT_DEVICEARRIVAL != wParam);

				if (ready)
				{
#ifndef __TEST_MODE__
					globalProgress->Send(device, 1);
#endif // !__TEST_MODE__


				}
			}

		}
	}

	return 1;
}

NODE_MODULE(usbspy, Init)