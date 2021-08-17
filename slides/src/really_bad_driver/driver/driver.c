#pragma warning( disable : 4100 ) 

#include <wdm.h>

typedef struct {
	HANDLE hPID;
	ULONGLONG targetAddr;
} Request;

typedef struct {
	UCHAR code;
	UCHAR mem;
	ULONGLONG eprocessAddr;
} Response;

typedef struct _KAPC_STATE {
	LIST_ENTRY  ApcListHead[2];
	PVOID       Process;
	BOOLEAN     KernelApcInProgress;
	BOOLEAN     KernelApcPending;
	BOOLEAN     UserApcPending;
} KAPC_STATE, * PKAPC_STATE;

extern NTSTATUS PsLookupProcessByProcessId(HANDLE ProcessId, PEPROCESS* Process);
extern VOID KeStackAttachProcess(PEPROCESS Process, KAPC_STATE* ApcState);
extern VOID KeUnstackDetachProcess(KAPC_STATE* ApcState);
extern NTKERNELAPI VOID KeAttachProcess(PEPROCESS Process);
extern NTKERNELAPI VOID KeDetachProcess();

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);

VOID DriverUnload(PDRIVER_OBJECT DriverObject);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#endif

#define DEVICE_TYPE_VAULTDRV   0x434d

#define DEVICE_NAME_VAULTDRV  L"\\Device\\bad_drv"
#define SYMLINK_NAME_VAULTDRV L"\\DosDevices\\bad_drv"

#define IOCTL_READPROCMEM CTL_CODE(DEVICE_TYPE_VAULTDRV, 0x0001, METHOD_BUFFERED, FILE_ALL_ACCESS)

NTSTATUS DispatchCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS DispatchIoctl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	NTSTATUS Status;
	UNICODE_STRING DeviceKernel;
	UNICODE_STRING DeviceUser;
	PDEVICE_OBJECT DeviceObject = NULL;

	UNREFERENCED_PARAMETER(RegistryPath);

	DriverObject->DriverUnload = DriverUnload;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoctl;

	RtlInitUnicodeString(&DeviceKernel, DEVICE_NAME_VAULTDRV);

	Status = IoCreateDevice(DriverObject, 0, &DeviceKernel, DEVICE_TYPE_VAULTDRV, 0, FALSE, &DeviceObject);

	if (!NT_SUCCESS(Status)) {
		DbgPrint("bad_drv: IoCreateDevice failed.\n");
		goto Exit;
	}

	RtlInitUnicodeString(&DeviceUser, SYMLINK_NAME_VAULTDRV);
	Status = IoCreateSymbolicLink(&DeviceUser, &DeviceKernel);

	if (!NT_SUCCESS(Status)) {
		DbgPrint("bad_drv: IoCreateSymbolicLink failed.\n");
		goto Exit;
	}

	return STATUS_SUCCESS;

Exit:
	if (DeviceObject) {
		IoDeleteDevice(DeviceObject);
	}

	return Status;
}

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
	UNICODE_STRING DeviceUser;

	RtlInitUnicodeString(&DeviceUser, SYMLINK_NAME_VAULTDRV);
	IoDeleteSymbolicLink(&DeviceUser);
	IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS DispatchCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS DispatchIoctl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	NTSTATUS Status = STATUS_NOT_SUPPORTED;
	ULONG BytesReturned = 0;
	PIO_STACK_LOCATION IoStackLocation;

	IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

	switch (IoStackLocation->Parameters.DeviceIoControl.IoControlCode) {

		// Handle a READPROC mem request
		// Request should be in the following format:
		// HANDLE pid; <-- target pid
		// ULONGLONG addr; <-- target address
		// we will return a single byte
		// although, now that I think about it, you could
		// use this to read memory from... anywhere
		// almost.

	case  IOCTL_READPROCMEM: {
		HANDLE hProc = NULL;
		ULONGLONG addr;
		PEPROCESS ProcessObject = NULL;
		KAPC_STATE state;

		if (IoStackLocation->Parameters.DeviceIoControl.InputBufferLength < (sizeof(HANDLE) + sizeof(DWORD64))) {
			BytesReturned = 0;
			Status = STATUS_INVALID_PARAMETER;
			goto done;
		}

		hProc = ((Request*)Irp->AssociatedIrp.SystemBuffer)->hPID;
		addr = ((Request*)Irp->AssociatedIrp.SystemBuffer)->targetAddr;

		// reading from this address causes bad things to happen
		// (aka bluescreen the box)
		if (addr == 0xffffffffffffffff) {
			break;
		}

		Status = PsLookupProcessByProcessId(hProc, &ProcessObject);
		if (!NT_SUCCESS(Status)) {
			DbgPrint("bad_drv: PsLookupProcessByProcessId failed.\n");
			goto done;
		}

		RtlZeroMemory(Irp->AssociatedIrp.SystemBuffer, IoStackLocation->Parameters.DeviceIoControl.InputBufferLength);

		DbgPrint("Process EPROCESS at: %p\n", ProcessObject);

		__try {
			KeStackAttachProcess(ProcessObject, &state);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			DbgPrint("bad_drv: KeStackAttachProcess failed.\n");
			goto done;
		}

		// return the 1-byte of memory at offset 1
		__try {
			((Response*)Irp->AssociatedIrp.SystemBuffer)->mem = ((PUCHAR)addr)[0];
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			DbgPrint("%s ReadMemory FAIL1: %llx\n ", __FUNCTION__, addr);
			goto detach;
		}

		// return the success code at offset 0
		__try {
			((Response*)Irp->AssociatedIrp.SystemBuffer)->code = 1;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			DbgPrint("%s ReadMemory FAIL1\n", __FUNCTION__);
			goto detach;
		}

		// used for troubleshooting...
		// return the address of the EPROCESS pointer at offset 2
		((Response*)Irp->AssociatedIrp.SystemBuffer)->eprocessAddr = (ULONGLONG)ProcessObject;

		Status = STATUS_SUCCESS;
		BytesReturned = IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
	detach:
		__try {
			KeUnstackDetachProcess(&state);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			DbgPrint("bad_drv: KeUnstackDetachProcess failed.\n");
			goto done;
		}
		break;
	}

	default:
		break;
	}

done:
	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = BytesReturned;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return Status;
}
