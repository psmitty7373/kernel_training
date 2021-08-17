#include <wdm.h>

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath);
VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject);
DRIVER_DISPATCH DispatchCreateClose;
DRIVER_DISPATCH DispatchIoctl;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(PAGE,DispatchCreateClose)
#pragma alloc_text(PAGE,DispatchIoctl)
#pragma alloc_text(PAGE,DriverUnload)
#endif

// define our driver id and names
#define DEVICE_TYPE_CUSTOM 0x6162
#define DEVICE_NAME L"\\Device\\ioctl_wdm"
#define DEVICE_LINK L"\\DosDevices\\ioctl_wdm"

// !!!!**** (0) Define our IOCTL control code using the appropriate macro here.  The function code is up to you! ****!!!!
// code goes here
#define IOCTL_DOSTUFF CTL_CODE(DEVICE_TYPE_CUSTOM, 0x0001, METHOD_BUFFERED, FILE_ALL_ACCESS)
// ****!!!! (0) !!!!****

NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    NTSTATUS Status;
    UNICODE_STRING DeviceName;
    UNICODE_STRING DeviceLink;
    PDEVICE_OBJECT DeviceObject = NULL;

    DbgPrint("ioctl_wdm: DriverEntry\n");

    DriverObject->DriverUnload = DriverUnload;

    // register dispatch routines
    // IRP_MJ_CREATE - request to open a handle to the device
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreateClose;
    // IRP_MJ_CLOSE - request when last handle to a device is closed
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchCreateClose;

    // !!!!**** (1) Assign the appropriate major function for handling IOCTLs to point to your Dispatch Routine ****!!!!
    // code goes here
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoctl;
    // ****!!!! (1) !!!!****
    
    // initialize our device name
    RtlInitUnicodeString(&DeviceName, DEVICE_NAME);

    // initialize our device symbolic link
    RtlInitUnicodeString(&DeviceLink, DEVICE_LINK);

    // attempt to create a device object for our device
    // https://docs.microsoft.com/en-us/windows-hardware/drivers/ifs/creating-the-control-device-object
    // https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-iocreatedevice
    Status = IoCreateDevice(
        DriverObject,       // driver object
        0,                  // device extension size
        &DeviceName,        // device name
        DEVICE_TYPE_CUSTOM, // device type
        0,                  // device characteristics
        FALSE,              // exclusive
        &DeviceObject);     // *DeviceObject

    // make sure IoCreateDevice succeeded
    if (!NT_SUCCESS(Status)) {
        DbgPrint("ioctl_wdm: DriverEntry IoCreateDevice failed!\n");
        goto exit;
    }

    // create a symbolic link for our device object that is accessible from user space
    // https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-iocreatesymboliclink
    Status = IoCreateSymbolicLink(&DeviceLink, &DeviceName);

    // make sure IoCreateSymbolicLink succeeded
    if (!NT_SUCCESS(Status)) {
        DbgPrint("ioctl_wdm: DriverEntry IoCreateSymbolicLink failed!\n");
        goto exit;
    }

    // all good
    return STATUS_SUCCESS;

exit:
    if (DeviceObject) {
        IoDeleteDevice(DeviceObject);
    }
    return Status;
}

// !!!!**** (2) Routine for handling IOCTLs defined here  ****!!!!
NTSTATUS
DispatchIoctl(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp)
{
    NTSTATUS Status = STATUS_NOT_SUPPORTED;
    PIO_STACK_LOCATION IoStackLocation;

    UNREFERENCED_PARAMETER(DeviceObject);

    DbgPrint("ioctl_wdm: DispatchIoctl\n");

    // !!!!**** (3) Get our current stack location and store it in IoStackLocation ****!!!!
    // code goes here
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    // ****!!!! (3) !!!! ****

    // !!!!**** (4) switch on the appropriate parameter from the IoStackLocation ****!!!!
    switch (IoStackLocation->Parameters.DeviceIoControl.IoControlCode) {
    // ****!!!! (4) !!!!****

        // !!!!**** (5) create a case statement for your created IOCTL function code ****!!!!
        // code goes here
        // just make it do something like printing to the debug log
        // make sure to set Status to something succesful
        case IOCTL_DOSTUFF: {
            Status = STATUS_SUCCESS;
            DbgPrint("ioctl_wdm: DispatchIoctl got an IOCTL!\n");
            break;
        }
        // ****!!!! (5) !!!!****

        default: {
            DbgPrint("ioctl_wdm: DispatchIoctl Unknown IOCTL\n");
            break;
        }
    };

    // !!!!**** (6) mark the IRP as with the appropriate status ****!!!!
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    // ****!!!! (6) !!!!****

    return Status;
}
// ****!!!! (2) !!!!****

NTSTATUS
DispatchCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    DbgPrint("ioctl_wdm: DispatchCreateClose Irp=%p\n", Irp);

    // mark the irp as successful
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    // tell the kernel we have processed this irp
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

VOID
DriverUnload(_In_ PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNICODE_STRING DeviceLink;

    DbgPrint("ioctl_wdm: DriverUnload\n");

    // initialize our device symbolic link
    RtlInitUnicodeString(&DeviceLink, DEVICE_LINK);

    // delete the symbolic link
    IoDeleteSymbolicLink(&DeviceLink);

    // delete the device object
    IoDeleteDevice(DriverObject->DeviceObject);
}
