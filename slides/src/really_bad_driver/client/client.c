#include <windows.h>
#include <stdio.h>

#define DEVICE_TYPE_BADDRV 0x434d
#define	DEVICE_NAME_BADDRV "\\\\.\\bad_drv"
#define IOCTL_READPROCMEM CTL_CODE(DEVICE_TYPE_BADDRV, 0x0001, METHOD_BUFFERED, FILE_ALL_ACCESS)

int main(int argc, char* argv[], char* envp[])
{
	HANDLE driver = INVALID_HANDLE_VALUE;
	HANDLE hPID;
	DWORD64 addr;
	DWORD64 length;
	ULONG bytes_transferred;
	char in_buf[sizeof(HANDLE) + sizeof(DWORD64)];
	char out_buf[64] = {0};
	char* buf = NULL;

	if (argc != 4) {
		printf("Usage: client.exe <pid> <start address> <length>\n");
		goto exit;
	}

	hPID = (HANDLE)atoll(argv[1]);
	addr = atoll(argv[2]);
	length = atoll(argv[3]);

	buf = (char*)calloc(1, length + 1);
	if (NULL == buf) {
		printf("[!] Error allocating buffer!\n");
		goto exit;
	}

	// open the driver
	if ((driver = CreateFileA(
		DEVICE_NAME_BADDRV,
		GENERIC_READ | GENERIC_WRITE,
		0,
		0,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0)) == INVALID_HANDLE_VALUE) {

		printf("[-] CreateFile() failed: 0x%x\n", GetLastError());
		goto exit;
	}

	// attempt to read memory
	int i;
	for (i = 0; i < length; i++) {
		*((HANDLE*)in_buf) = hPID;
		*((DWORD64*)(in_buf + sizeof(HANDLE))) = addr + i;

		if (DeviceIoControl(
			driver,
			IOCTL_READPROCMEM,
			in_buf,
			sizeof(in_buf),
			out_buf,
			sizeof(out_buf),
			&bytes_transferred,
			NULL) == 0) 
		{
			printf("[-] DeviceIoControl(IOCTL_READPROCMEM) FAIL=0x%x\n", GetLastError());
			goto exit;
		}

		buf[i] = out_buf[1];
	}
	buf[i] = '\0';

	printf("[*] buf(hex): ");
	for (int i = 0; i < length; i++)
	{
		printf("%02X", buf[i]);
	}

	printf("\n[*] buf: %s\n", buf);

exit:
	if (driver != INVALID_HANDLE_VALUE) {
		CloseHandle(driver);
	}

	if (buf) {
		free(buf);
	}
	printf("[+] Done\n");
	return 0;
}