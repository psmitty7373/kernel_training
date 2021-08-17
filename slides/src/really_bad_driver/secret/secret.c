#include <windows.h>
#include <stdio.h>

int main(int argc, char* argv[], char* envp[])
{
	char buf[] = "this is so secret you will never find it!";

	printf("[*] My pid: %d\n", GetCurrentProcessId());
	printf("[*] buf allocated at: %p\n", buf);

	Sleep(1000 * 60 * 60);

	return 0;
}