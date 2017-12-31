/*********************************************************************
 * NAN - Native Abstractions for Node.js
 *
 * Copyright (c) 2017 NAN contributors
 *
 * MIT License <https://github.com/nodejs/nan/blob/master/LICENSE.md>
 ********************************************************************/

#include <nan.h>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <dbt.h>
#include <tchar.h>
#include <atlstr.h>

#include "cond_var.h"

using namespace Nan; // NOLINT(build/namespaces)

std::mutex m;
std::condition_variable cv;
bool ready = false;

DWORD WINAPI SpyingThread();
LRESULT CALLBACK SpyCallback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#define MAX_THREAD_WINDOW_NAME 64

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

struct data_t {
	int deviceNumber;
	std::string serialNumber;
	std::string productId;
	std::string vendorId;
	std::string driveLetter;
};

const typename AsyncProgressQueueWorker<data_t>::ExecutionProgress *globalProgress;

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
			New<v8::Integer>(data->deviceNumber));
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
		progress->Call(1, argv);
	}

private:
	Callback * progress;
};

NAN_METHOD(SpyOn)
{
//#ifdef DEBUG
	/*Callback *progress = new Callback();
	Callback *callback = new Callback();*/
//#else 
	Callback *progress = new Callback(To<v8::Function>(info[0]).ToLocalChecked());
	Callback *callback = new Callback(To<v8::Function>(info[1]).ToLocalChecked());
//#endif // DEBUG

	AsyncQueueWorker(new ProgressQueueWorker<data_t>(callback, progress));
}

void StartSpying()
{
	{
		std::lock_guard<std::mutex> lk(m);
		ready = true;
		std::cout << "main() signals data ready for processing\n" << std::endl;
	}
	cv.notify_one();

	//New<v8::FunctionTemplate>(SpyOn)->GetFunction()->CallAsConstructor(0, {});
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

void processData(const typename AsyncProgressQueueWorker<data_t>::ExecutionProgress& progress) {
	globalProgress = &progress;

	std::thread worker(SpyingThread);
	worker.detach();
	//worker.join();
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
				data_t d;

				int len = 0;

				CString szDevId = pDevInf->dbcc_name + 4;
				len = szDevId.GetLength();
				int idx = szDevId.ReverseFind(_T('#'));
				szDevId.Truncate(idx);
				len = szDevId.GetLength() - 1;

				CString serialNumber;
				idx = szDevId.ReverseFind(_T('#'));
				serialNumber = szDevId.Right(len - idx);
				szDevId.Truncate(idx);
				len = szDevId.GetLength() - 1;
				d.serialNumber = serialNumber;

				CString productId;
				idx = szDevId.ReverseFind(_T('&'));
				productId = szDevId.Right(len - idx);
				int idx1 = productId.ReverseFind(_T('_'));
				len = productId.GetLength() - 1;
				productId = productId.Right(len - idx1);
				szDevId.Truncate(idx);
				len = szDevId.GetLength() - 1;
				d.productId = productId;

				CString vendorId;
				idx = szDevId.ReverseFind(_T('#'));
				vendorId = szDevId.Right(len - idx);
				idx1 = vendorId.ReverseFind(_T('_'));
				len = vendorId.GetLength() - 1;
				vendorId = vendorId.Right(len - idx1);
				d.vendorId = vendorId;

				_sleep(3000);

				// Get available drives we can monitor
				DWORD drives_bitmask = GetLogicalDrives();
				int i = 1;
				int deviceNumber = 1;
				while (drives_bitmask)
				{
					if (drives_bitmask & 1) {
						TCHAR drive[] = { TEXT('A') + i, TEXT(':'), TEXT('\\'), TEXT('\0') };
						if (GetDriveType(drive) == DRIVE_REMOVABLE)
						{
							d.driveLetter = drive;
							d.deviceNumber = deviceNumber;
							deviceNumber++;
						}
					}
					drives_bitmask >>= 1;
					i++;
				}

				globalProgress->Send(&d, 1);
			}

		}
	}

	return 1;
}

NODE_MODULE(asyncprogressqueueworker, Init)