#include <iostream>
#include <atlstr.h>
#include <comdef.h>
#include <map>

#include "usbs.h"
#include "usbspy.h"

Device *PopulateAvailableUSBDeviceList(bool adjustDeviceList)
{
	DWORD drives_bitmask = GetLogicalDrives();
	DWORD drive;

	std::vector<const char *> keys;
	Device *device;

	for (drive = 0; drive < 32; ++drive)
	{
		if (drives_bitmask & (1 << drive))
		{
			TCHAR driveLetter[] = { TEXT('A') + drive, TEXT(':'), TEXT('\\'), TEXT('\0') };
			if (GetDriveType(driveLetter) == DRIVE_REMOVABLE)
			{
				device = new Device();
				GetUSBDriveDetails(drive, device);

				const char *key = "";
				_bstr_t b(device->vendorId.append(device->productId).append(device->serialNumber).c_str());
				key = b;

				if (HasDevice(key))
				{
					keys.push_back(key);
					std::cout << "Device " << device->vendorId << " (" << device->productId << ")"
						<< " is already there in the list!" << std::endl;
				}
				else
				{
					device->driveLetter = String(driveLetter);
					device->deviceStatus = (int)Connect;
					AddDevice(key, device);
					std::cout << "Device " << device->driveLetter << " (" << device->productId << ")"
						<< " is been added!" << std::endl;
				}
			}
		}
	}

	if (adjustDeviceList)
	{
		Device *deviceToBeRemoved;
		device = new Device();

		deviceToBeRemoved = GetDeviceToBeRemoved(keys);

		if (deviceToBeRemoved)
		{
			MapDeviceProps(device, deviceToBeRemoved);
			RemoveDevice(deviceToBeRemoved);
			device->deviceStatus = (int)Disconnect;

			std::cout << "Device " << deviceToBeRemoved->driveLetter << " (" << deviceToBeRemoved->productId << ")"
				<< " is been removed!" << std::endl;

			//delete deviceToBeRemoved;
		}
	}

	return device;
}

DWORD GetUSBDriveDetails(UINT nDriveNumber IN, Device *device OUT)
{
	DWORD dwRet = NO_ERROR;

	// Format physical drive path (may be '\\.\PhysicalDrive0', '\\.\PhysicalDrive1' and so on).
	CString strDrivePath;
	//sprintf(szBuf, "\\\\?\\%c:", 'A' + drive);
	strDrivePath.Format(_T("\\\\.\\%c:"), 'A' + nDriveNumber);

	// Get a handle to physical drive
	HANDLE hDevice = ::CreateFile(strDrivePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
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
	BYTE *pOutBuffer = new BYTE[dwOutBufferSize];
	ZeroMemory(pOutBuffer, dwOutBufferSize);

	// Get the storage device descriptor
	if (!::DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
		&storagePropertyQuery, sizeof(STORAGE_PROPERTY_QUERY),
		pOutBuffer, dwOutBufferSize,
		&dwBytesReturned, NULL))
	{
		dwRet = ::GetLastError();
		delete[] pOutBuffer;
		::CloseHandle(hDevice);
		return dwRet;
	}

	// Now, the output buffer points to a STORAGE_DEVICE_DESCRIPTOR structure
	// followed by additional info like vendor ID, product ID, serial number, and so on.
	STORAGE_DEVICE_DESCRIPTOR *pDeviceDescriptor = (STORAGE_DEVICE_DESCRIPTOR *)pOutBuffer;
	const DWORD dwSerialNumberOffset = pDeviceDescriptor->SerialNumberOffset;

	if (pDeviceDescriptor->ProductIdOffset != 0)
	{
		// Finally, get the serial number
		device->productId = CString(pOutBuffer + pDeviceDescriptor->ProductIdOffset);
	}

	if (pDeviceDescriptor->VendorIdOffset != 0)
	{
		// Finally, get the serial number
		device->vendorId = CString(pOutBuffer + pDeviceDescriptor->VendorIdOffset);
	}

	if (pDeviceDescriptor->SerialNumberOffset != 0)
	{
		// Finally, get the serial number
		device->serialNumber = CString(pOutBuffer + pDeviceDescriptor->SerialNumberOffset);
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
	delete[] pOutBuffer;
	::CloseHandle(hDevice);
	return dwRet;
}