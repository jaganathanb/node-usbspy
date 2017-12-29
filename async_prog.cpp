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

#include "cond_var.h"

using namespace Nan; // NOLINT(build/namespaces)

std::mutex m;
std::condition_variable cv;
std::string data;
bool ready = true;
bool process = true;

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
	int index;
	int data;
	int dd;
};

const typename AsyncProgressQueueWorker<data_t>::ExecutionProgress *globalProgress;

template<typename T>
class ProgressQueueWorker : public AsyncProgressQueueWorker<T> {
public:
	ProgressQueueWorker(
		Callback *callback
		, Callback *progress
		, int iters)
		: AsyncProgressQueueWorker<T>(callback), progress(progress)
		, iters(iters) {}

	~ProgressQueueWorker() {
		delete progress;
	}

	void Execute(const typename AsyncProgressQueueWorker<T>::ExecutionProgress& progress) {
		std::unique_lock<std::mutex> lk(m);

		processData(progress);

		while (ready) {
			cv.wait(lk);
		}
	}

	void HandleProgressCallback(const T *data, size_t count) {
		HandleScope scope;
		v8::Local<v8::Object> obj = Nan::New<v8::Object>();
		Nan::Set(
			obj,
			Nan::New("index").ToLocalChecked(),
			New<v8::Integer>(data->index));
		Nan::Set(
			obj,
			Nan::New("data").ToLocalChecked(),
			New<v8::Integer>(data->data));
		Nan::Set(
			obj,
			Nan::New("dd").ToLocalChecked(),
			New<v8::Integer>(data->dd));

		v8::Local<v8::Value> argv[] = { obj };
		progress->Call(1, argv);
	}

private:
	Callback * progress;
	int iters;
};

NAN_METHOD(DoProgress)
{
	Callback *progress = new Callback(To<v8::Function>(info[1]).ToLocalChecked());
	Callback *callback = new Callback(To<v8::Function>(info[2]).ToLocalChecked());
	AsyncQueueWorker(new ProgressQueueWorker<data_t>(callback, progress, To<uint32_t>(info[0]).FromJust()));
}

NAN_METHOD(On)
{
	{
		std::lock_guard<std::mutex> lk(m);
		ready = true;
		std::cout << "main() signals data ready for processing\n" << std::endl;
	}
	cv.notify_one();
}

NAN_METHOD(Off)
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
	Set(target, New<v8::String>("doProgress").ToLocalChecked(), New<v8::FunctionTemplate>(DoProgress)->GetFunction());
	Set(target, New<v8::String>("spyOn").ToLocalChecked(), New<v8::FunctionTemplate>(On)->GetFunction());
	Set(target, New<v8::String>("spyOff").ToLocalChecked(), New<v8::FunctionTemplate>(Off)->GetFunction());
	//(New<v8::FunctionTemplate>(On)->GetFunction())->CallAsConstructor(0, NULL);
}

void worker_thread()
{
	// Wait until main() sends data
	std::unique_lock<std::mutex> lk(m);
	cv.wait(lk, [] { return ready; });

	// after the wait, we own the lock.
	std::cout << "Worker thread is processing data\n";
	data += " after processing";

	std::cout << "Worker thread signals data processing completed\n";

	// Manual unlocking is done before notifying, to avoid waking up
	// the waiting thread only to block again (see notify_one for details)
	lk.unlock();
	cv.notify_one();
}

int main()
{
	std::thread worker(worker_thread);

	data = "Example data";
	// send data to the worker thread
	{
		std::lock_guard<std::mutex> lk(m);
		ready = true;
		std::cout << "main() signals data ready for processing\n";
	}
	cv.notify_one();

	std::cout << "Back in main(), data = " << data << '\n';

	worker.join();
}

void processData(const typename AsyncProgressQueueWorker<data_t>::ExecutionProgress& progress) {
	data_t *dt = new data_t();
	dt->data = 10;
	dt->index = 0;
	progress.Send(dt, sizeof(data_t));
	globalProgress = &progress;

	std::thread worker(SpyingThread);
	worker.detach();
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
			std::cout << "Done! gets message \n" << std::endl;
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
				data_t *d = new data_t();
				d->data = 100;
				d->dd = pDevInf->dbcc_devicetype;
				globalProgress->Send(d, sizeof(d));
			}
		}
	}

	return 1;
}

NODE_MODULE(asyncprogressqueueworker, Init)