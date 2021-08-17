#include <Windows.h>
#include <stdio.h>

// define driver link and id
#define DEVICE_TYPE_CUSTOM 0x6162
#define DEVICE_LINK L"\\\\.\\ioctl_wdm"
// !!!!**** (0) Define our IOCTL control code using the appropriate macro here.  The function code is up to you! ****!!!!
#define IOCTL_DOSTUFF CTL_CODE(DEVICE_TYPE_CUSTOM, 0x0001, METHOD_BUFFERED, FILE_ALL_ACCESS)
// ****!!!! (0) !!!!****

int main() {
	int ret = 0;
	HANDLE driver = INVALID_HANDLE_VALUE;
	DWORD BytesTransferred;

	// open our driver
	driver = CreateFile(
		DEVICE_LINK,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);


	// make sure the driver opened
	if (INVALID_HANDLE_VALUE == driver) {
		printf("Error, unable to open driver.\n");
		return 1;
	}

	// !!!!**** (1) use the appropriate function to send your defined IOCTL to the driver ****!!!!
	if (0 == DeviceIoControl(
		driver,					// driver handle
		IOCTL_DOSTUFF,			// io control code
		NULL,					// in buffer
		0,						// in buffer size
		NULL,					// out buffer
		0,						// out buffer size
		&BytesTransferred,		// bytes returned
		NULL))					// overlapped
	{
		printf("Error, unable to send IOCTL\n");
		ret = 1;
	}
	// ****!!!! (1) !!!!****

	if (INVALID_HANDLE_VALUE != driver) {
		CloseHandle(driver);
	}

	return ret;
}