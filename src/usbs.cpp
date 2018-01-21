#include "usbs.h"

std::map<std::string, Device *> deviceMap;

void AddDevice(Device *item)
{
	deviceMap.insert(std::pair<std::string, Device *>(item->GetKey(), item));
}

void RemoveDevice(Device *item)
{
	deviceMap.erase(item->GetKey());
}

Device *GetDevice(std::string key)
{
	std::map<std::string, Device *>::iterator it;

	it = deviceMap.find(key);
	if (it == deviceMap.end())
	{
		return NULL;
	}
	else
	{
		return it->second;
	}
}

void MapDeviceProps(Device *destiDevice, Device *sourceDevice)
{
	destiDevice->device_status = sourceDevice->device_status;
	destiDevice->vendor_id = sourceDevice->vendor_id;
	destiDevice->product_id = sourceDevice->product_id;
	destiDevice->serial_number = sourceDevice->serial_number;
	destiDevice->device_number = sourceDevice->device_number;
	destiDevice->device_letter = sourceDevice->device_letter;
	destiDevice->SetKey(sourceDevice->GetKey());
}

bool HasDevice(std::string key)
{
	std::map<std::string, Device *>::iterator it;

	it = deviceMap.find(key);
	if (it == deviceMap.end())
	{
		return false;
	}
	else
	{
		return true;
	}

	return true;
}

Device *GetDeviceToBeRemoved(std::list<std::string> keys)
{
	Device *deviceToBeRemoved = NULL;
	std::map<std::string, Device *>::iterator it;
	for (it = deviceMap.begin(); it != deviceMap.end(); ++it)
	{
		Device *item = it->second;
		if (std::find(keys.begin(), keys.end(), item->GetKey()) == keys.end())
		{
			deviceToBeRemoved = new Device;
			MapDeviceProps(deviceToBeRemoved, item);
			break;
		}
	}
	return deviceToBeRemoved;
}

std::list<Device *> GetUSBDevices()
{
	std::list<Device *> deviceList = {};
	Device *device;
	std::map<std::string, Device *>::iterator it;
	for (it = deviceMap.begin(); it != deviceMap.end(); ++it)
	{
		Device *item = it->second;
		device = new Device;
		MapDeviceProps(device, item);
		deviceList.push_back(device);
	}
	return deviceList;
}

USBProperties ResolveUSBProperty(std::string property_name) {
	static const std::map<std::string, USBProperties> usbPropMap{
		{ "device_status", DeviceStatus },
	{ "device_number", DeviceNumber },
	{"device_letter", DeviceLetter},
	{"serial_number", SerialNumber},
	{"product_id", ProductId},
	{"vendor_id", VendorId}
	};

	auto itr = usbPropMap.find(property_name);
	if (itr != usbPropMap.end()) {
		return itr->second;
	}
	return InvaildProperty;
}

Device *GetUSBDeviceByPropertyName(std::string property_name, std::string value)
{
	Device *device = NULL;
	std::map<std::string, Device *>::iterator it;
	for (it = deviceMap.begin(); it != deviceMap.end(); ++it)
	{
		Device *item = it->second;
		bool found = false;

		switch (ResolveUSBProperty(property_name))
		{
		case DeviceLetter:
			found = item->device_letter == value;
			break;
		case DeviceNumber:
			found = item->device_number == atoi(value.c_str());
			break;
		case SerialNumber:
			found = item->serial_number == value;
			break;
		case ProductId:
			found = item->product_id == value;
			break;
		case VendorId:
			found = item->vendor_id == value;
			break;

		default:
			found = item->device_status == 1;
			break;
		}

		if (found)
		{
			device = new Device;
			MapDeviceProps(device, item);
			break;
		}
	}
	return device;
}

void ClearUSBDeviceList()
{
	std::map<std::string, Device *>::iterator it;
	for (it = deviceMap.begin(); it != deviceMap.end(); ++it)
	{
		deviceMap.erase(it->first);
		delete it->second;
	}
}