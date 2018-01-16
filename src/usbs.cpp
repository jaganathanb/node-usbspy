#include "usbs.h"

std::map<std::string, Device*> deviceMap;

void AddDevice(Device *item)
{
	deviceMap.insert(std::pair<std::string, Device*>(item->GetKey(), item));
}

void RemoveDevice(Device *item)
{
	deviceMap.erase(item->GetKey());
}

Device *GetDevice(std::string key)
{
	std::map<std::string, Device*>::iterator it;

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
	destiDevice->drive_letter = sourceDevice->drive_letter;
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
	std::map<std::string, Device*>::iterator it;
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

std::list<Device *> GetUSBDevices() {
	std::list<Device *> deviceList = {};
	Device *device;
	std::map<std::string, Device*>::iterator it;
	for (it = deviceMap.begin(); it != deviceMap.end(); ++it)
	{
		Device *item = it->second;
		device = new Device;
		MapDeviceProps(device, item);
		deviceList.push_back(device);
	}
	return deviceList;
}

Device *GetUSBDeviceByLetter(std::string device_letter) {
	Device *device = NULL;
	std::map<std::string, Device*>::iterator it;
	for (it = deviceMap.begin(); it != deviceMap.end(); ++it)
	{
		Device *item = it->second;

		if (item->drive_letter == device_letter)
		{
			device = new Device;
			MapDeviceProps(device, item);
			break;
		}
	}
	return device;
}

void ClearUSBDeviceList() {
	Device *device;
	std::map<std::string, Device*>::iterator it;
	for (it = deviceMap.begin(); it != deviceMap.end(); ++it)
	{
		deviceMap.erase(it->first);
		delete it->second;
	}
}