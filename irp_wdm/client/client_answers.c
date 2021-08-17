#include <Windows.h>
#include <stdio.h>

// !!!!**** (0) define a name for your driver (done for you) ****!!!!
#define DEVICE_LINK L"\\\\.\\irp_wdm"
// !!!!**** (0) ****!!!!

int main() {
	HANDLE driver = INVALID_HANDLE_VALUE;

	// !!!!**** (1) create a handle to your driver using an applicable function ****!!!!
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
	// !!!!**** (1) ****!!!!

	// sleep for 30 seconds
	Sleep(30 * 1000);

	// !!!!**** (2) properly close the handle to your driver ****!!!!
	if (INVALID_HANDLE_VALUE != driver) {
		CloseHandle(driver);
	}
	// !!!!**** (2) ****!!!!

	return 0;
}