#ifndef _USBS_H
#define _USBS_H

#include "usbspy.h"

void AddDevice(const char *key, Device *item);
void RemoveDevice(Device *item);
Device *GetDevice(const char *key);
void MapDeviceProps(Device *destiDevice, Device *sourceDevice);
bool HasDevice(const char *key);
Device *GetDeviceToBeRemoved(std::vector<const char *> keys);
#endif