DWORD WINAPI SpyingThread();
LRESULT CALLBACK SpyCallback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

Device *PopulateAvailableUSBDeviceList(bool adjustDeviceList);
DWORD GetUSBDriveDetails(UINT nDriveNumber IN, Device *device OUT);

void StartSpying();