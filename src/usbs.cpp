#include "usbs.h"

std::map<std::string, Device *> deviceMap;

void AddDevice(const char *key, Device *item)
{
	item->SetKey(key);
	deviceMap.insert(std::pair<std::string, Device *>(item->GetKey(), item));
}

void RemoveDevice(Device *item)
{
	deviceMap.erase(item->GetKey());
}

Device *GetDevice(const char *key)
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
	destiDevice->deviceStatus = sourceDevice->deviceStatus;
	destiDevice->vendorId = sourceDevice->vendorId;
	destiDevice->productId = sourceDevice->productId;
	destiDevice->serialNumber = sourceDevice->serialNumber;
	destiDevice->deviceNumber = sourceDevice->deviceNumber;
	destiDevice->driveLetter = sourceDevice->driveLetter;
}

bool HasDevice(const char *key)
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

Device *GetDeviceToBeRemoved(std::vector<const char *> keys)
{
	Device *deviceToBeRemoved;
	std::map<std::string, Device *>::iterator it;
	for (it = deviceMap.begin(); it != deviceMap.end(); ++it)
	{
		Device *item = it->second;
        char key [260]= "";
		sprintf(key, "%s", item->GetKey());
		if (std::find(keys.begin(), keys.end(), key) == keys.end())
		{
			deviceToBeRemoved = item;
			break;
		}
	}
	return deviceToBeRemoved;
}