#include <Windows.h>
#include <stdio.h>

// !!!!**** (0) define a name for your driver (done for you) ****!!!!
#define DEVICE_LINK L"\\\\.\\irp_wdm"
// !!!!**** (0) ****!!!!

int main() {
	HANDLE driver = INVALID_HANDLE_VALUE;

	// !!!!**** (1) create a handle to your driver using an applicable function ****!!!!
	// code goes here
	// !!!!**** (1) ****!!!!

	// sleep for 30 seconds
	Sleep(30 * 1000);

	// !!!!**** (2) properly close the handle to your driver ****!!!!
	// code goes here
	// !!!!**** (2) ****!!!!

	return 0;
}