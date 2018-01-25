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
	0xED};

const typename AsyncProgressQueueWorker<Device>::ExecutionProgress *globalProgress;

void processData(const typename AsyncProgressQueueWorker<Device>::ExecutionProgress &progress)
{
	globalProgress = &progress;

	PopulateAvailableUSBDeviceList();

	std::thread worker(SpyingThread);

#ifdef _DEBUG
	worker.join();
#else
	worker.detach();
#endif
}

DWORD WINAPI SpyingThread()
{
	char className[MAX_THREAD_WINDOW_NAME];
	_snprintf_s(className, MAX_THREAD_WINDOW_NAME, "ListnerThreadUsbDetection_%d", GetCurrentThreadId());

	WNDCLASSA wincl = {0};
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

	DEV_BROADCAST_DEVICEINTERFACE_A notifyFilter = {0};
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

DWORD GetUSBDriveDetails(UINT drive_number IN, Device *device OUT)
{
	DWORD return_value = NO_ERROR;

	// Format physical drive path (may be '\\.\PhysicalDrive0', '\\.\PhysicalDrive1' and so on).
	TCHAR drive_letter[260];
	//sprintf(szBuf, "\\\\?\\%c:", 'A' + drive);
	//strDrivePath.Format(_T("\\\\.\\%c:"), 'A' + drive_number);

	_stprintf(drive_letter, _T("\\\\.\\%c:"), 'A' + drive_number);

	// Get a handle to physical drive
	HANDLE drive_handle = ::CreateFile(drive_letter, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
									   NULL, OPEN_EXISTING, 0, NULL);

	if (INVALID_HANDLE_VALUE == drive_handle)
		return ::GetLastError();

	// Set the input data structure
	STORAGE_PROPERTY_QUERY query;
	ZeroMemory(&query, sizeof(STORAGE_PROPERTY_QUERY));
	query.PropertyId = StorageDeviceProperty;
	query.QueryType = PropertyStandardQuery;

	// Get the necessary output buffer size
	STORAGE_DESCRIPTOR_HEADER header = {0};
	DWORD bytes_returned = 0;
	if (!::DeviceIoControl(drive_handle, IOCTL_STORAGE_QUERY_PROPERTY,
						   &query, sizeof(STORAGE_PROPERTY_QUERY),
						   &header, sizeof(STORAGE_DESCRIPTOR_HEADER),
						   &bytes_returned, NULL))
	{
		return_value = ::GetLastError();
		::CloseHandle(drive_handle);
		return return_value;
	}

	// Alloc the output buffer
	const DWORD buffer_size = header.Size;
	BYTE *buffer = new BYTE[buffer_size];
	ZeroMemory(buffer, buffer_size);

	// Get the storage device descriptor
	if (!::DeviceIoControl(drive_handle, IOCTL_STORAGE_QUERY_PROPERTY,
						   &query, sizeof(STORAGE_PROPERTY_QUERY),
						   buffer, buffer_size,
						   &bytes_returned, NULL))
	{
		return_value = ::GetLastError();
		delete[] buffer;
		::CloseHandle(drive_handle);
		return return_value;
	}

	// Now, the output buffer points to a STORAGE_DEVICE_DESCRIPTOR structure
	// followed by additional info like vendor ID, product ID, serial number, and so on.
	STORAGE_DEVICE_DESCRIPTOR *device_desc = (STORAGE_DEVICE_DESCRIPTOR *)buffer;

	char value[260] = "";

	if (device_desc->ProductIdOffset != 0)
	{
		// Finally, get the serial number
		//device->product_id = CString(buffer + device_desc->ProductIdOffset);
		value[0] = '\0';
		sprintf(value, "%s", buffer + device_desc->ProductIdOffset);
		device->product_id = std::string(value);
	}

	if (device_desc->VendorIdOffset != 0)
	{
		// Finally, get the serial number
		//device->vendor_id = CString(buffer + device_desc->VendorIdOffset);
		value[0] = '\0';
		sprintf(value, "%s", buffer + device_desc->VendorIdOffset);
		device->vendor_id = std::string(value);
	}

	if (device_desc->SerialNumberOffset != 0)
	{
		// Finally, get the serial number
		//device->serial_number = CString(buffer + device_desc->serial_numberOffset);
		value[0] = '\0';
		sprintf(value, "%s", buffer + device_desc->SerialNumberOffset);
		device->serial_number = std::string(value);
	}

	STORAGE_DEVICE_NUMBER sdn;
	if (!DeviceIoControl(drive_handle,
						 IOCTL_STORAGE_GET_DEVICE_NUMBER, &query, sizeof(STORAGE_PROPERTY_QUERY),
						 &sdn, sizeof(sdn),
						 &bytes_returned, NULL))
	{
		return_value = ::GetLastError();
		delete[] buffer;
		::CloseHandle(drive_handle);
		return return_value;
	}

	device->device_number = (int)sdn.DeviceNumber;

	// Do cleanup and return
	delete[] buffer;
	::CloseHandle(drive_handle);
	return return_value;
}

void PopulateAvailableUSBDeviceList()
{
	// Get available drives we can monitor
	DWORD drives_bitmask = GetLogicalDrives();
	DWORD drive;
	TCHAR drive_letter[33];

	std::list<std::string>
		keys;

	Device *device = NULL;

	for (drive = 0; drive < 32; ++drive)
	{
		if (drives_bitmask & (1 << drive))
		{
			_stprintf(drive_letter, _T("%c:\\"), 'A' + drive);
			if (GetDriveType(drive_letter) == DRIVE_REMOVABLE)
			{
				device = new Device;
				GetUSBDriveDetails(drive, device);

				std::string key = "";
				key = key.append(device->vendor_id).append(device->product_id).append(device->serial_number);

				device->SetKey(key);
				device->device_letter = drive_letter;
				device->device_status = (int)Connect;
				AddDevice(device);
				std::cout << "Device " << device->device_letter << " (" << device->product_id << ")"
						  << " is been added!" << std::endl;
			}
		}
	}
}

Device *GetUSBDeviceDetails(bool adjustDeviceList)
{
	// Get available drives we can monitor
	DWORD drives_bitmask = GetLogicalDrives();
	DWORD drive;
	TCHAR drive_letter[33];

	std::list<std::string>
		keys;
	Device *device = NULL;

	for (drive = 0; drive < 32; ++drive)
	{
		if (drives_bitmask & (1 << drive))
		{
			_stprintf(drive_letter, _T("%c:\\"), 'A' + drive);
			device = GetUSBStorageDeviceByPropertyName("device_letter", drive_letter);

			if (GetDriveType(drive_letter) == DRIVE_REMOVABLE)
			{
				if (device && device->device_letter == drive_letter)
				{
					keys.push_back(device->GetKey());
					std::cout << "Device " << device->vendor_id << " (" << device->product_id << ")"
							  << " is already there in the list!" << std::endl;
					delete device; // check wether it is needed or not
					device = NULL;
				}
				else
				{
					device = new Device;
					GetUSBDriveDetails(drive, device);

					std::string key = "";
					key = key.append(device->vendor_id).append(device->product_id).append(device->serial_number);

					device->SetKey(key);
					device->device_letter = drive_letter;
					device->device_status = (int)Connect;
					AddDevice(device);
					std::cout << "Device " << device->device_letter << " (" << device->product_id << ")"
							  << " is been added!" << std::endl;
					break; // if new device is found, exit.
				}
			}
		}
	}

	if (adjustDeviceList)
	{
		Device *deviceToBeRemoved = NULL;

		deviceToBeRemoved = GetDeviceToBeRemoved(keys);

		if (deviceToBeRemoved)
		{
			device = new Device;
			MapDeviceProps(device, deviceToBeRemoved);
			RemoveDevice(deviceToBeRemoved);
			delete deviceToBeRemoved;
			device->device_status = (int)Disconnect;

			std::cout << "Device " << device->device_letter << " (" << device->product_id << ")"
					  << " is been removed!" << std::endl;
		}
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
				Device *device = NULL;

				std::this_thread::sleep_for(std::chrono::seconds(3));

				device = GetUSBDeviceDetails(DBT_DEVICEARRIVAL != wParam);

				if (device)
				{
					globalProgress->Send(device, 1);

					if (DBT_DEVICEARRIVAL != wParam)
					{
						delete device;
					}
				}
			}
		}
	}

	return 1;
}