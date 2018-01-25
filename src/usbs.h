#ifndef USBS_H
#define USBS_H

#include <map>
#include <list>
#include <string>

typedef struct Device_t
{
	int device_number;
	int device_status;
	std::string serial_number;
	std::string product_id;
	std::string vendor_id;
	std::string device_letter;

	void SetKey(std::string key)
	{
		_key = key;
	}

	std::string GetKey()
	{
		return _key;
	}

  private:
	std::string _key;
} Device;

typedef enum device_status_t {
	Disconnect = 0,
	Connect
} device_status;

enum USBProperties {
	InvaildProperty = -1,
	DeviceStatus,
	DeviceNumber,
	DeviceLetter,
	SerialNumber,
	ProductId,
	VendorId
};

bool HasDevice(std::string key);
void MapDeviceProps(Device *destiDevice, Device *sourceDevice);
Device *GetDevice(std::string key);
void RemoveDevice(Device *item);
void AddDevice(Device *item);
Device *GetDeviceToBeRemoved(std::list<std::string> keys);
Device *GetUSBStorageDeviceByPropertyName(std::string property_name, std::string value);
std::list<Device *> GetUSBStorageDevices();
void ClearUSBDeviceList();

#endif