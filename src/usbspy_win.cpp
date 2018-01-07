#include "usbspy.h"

using namespace Nan;

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
	0xED 
};

const typename AsyncProgressQueueWorker<Device>::ExecutionProgress *globalProgress;

void processData(const typename AsyncProgressQueueWorker<Device>::ExecutionProgress& progress) {
	globalProgress = &progress;

	PopulateAvailableUSBDeviceList(false);

	std::thread worker(SpyingThread);

#ifndef __TEST_MODE__
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

	STORAGE_DEVICE_NUMBER sdn;
	if (!DeviceIoControl(hDevice,
		IOCTL_STORAGE_GET_DEVICE_NUMBER, &storagePropertyQuery, sizeof(STORAGE_PROPERTY_QUERY),
		&sdn, sizeof(sdn),
		&dwBytesReturned, NULL))
	{
		dwRet = ::GetLastError();
		delete[] pOutBuffer;
		::CloseHandle(hDevice);
		return dwRet;
	}

	device->deviceNumber = (int)sdn.DeviceNumber;

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

				std::this_thread::sleep_for(std::chrono::seconds(3));

				device = PopulateAvailableUSBDeviceList(DBT_DEVICEARRIVAL != wParam);

#ifndef __TEST_MODE__
					globalProgress->Send(device, 1);
#endif // !__TEST_MODE__
			}

		}
	}

	return 1;
}