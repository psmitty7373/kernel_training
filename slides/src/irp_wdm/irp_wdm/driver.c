#include <wdm.h>

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath);
VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject);
DRIVER_DISPATCH DispatchCreateClose;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(PAGE,DispatchCreateClose)
#pragma alloc_text(PAGE,DriverUnload)
#endif

// !!!!**** (0) some defines useful for setting up our driver's device and symbolic link (done for you) ****!!!!
// define our driver id and names
#define DEVICE_TYPE_CUSTOM 0x6162
#define DEVICE_NAME L"\\Device\\irp_wdm"
#define DEVICE_LINK L"\\DosDevices\\irp_wdm"
// !!!!**** (0) ****!!!!

NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    NTSTATUS Status;
    UNICODE_STRING DeviceName;
    UNICODE_STRING DeviceLink;
    PDEVICE_OBJECT DeviceObject = NULL;

    DbgPrint("irp_wdm: DriverEntry\n");

    DriverObject->DriverUnload = DriverUnload;

    // !!!!**** (0) setup dispatch routines... make sure to setup both CREATE and CLOSE routines! ****!!!!
    // code goes here
    // ****!!!! (1) !!!!****

    // ****!!!! (2) initialize the unicode strings above with the device name and device link using the correct function !!!!****
    // code goes here
    // ****!!!! (2) !!!!****

    // ****!!!! (3) create a device object that has no extension, no characteristcs, is non-exclusive using the correct function !!!!****
    // code goes here
    // ****!!!! (3) !!!!****

    // ****!!!! (4) create a symbolic link for the driver using the correct function !!!!****
    // code goes here
    // ****!!!! (4) !!!!****

    // all good
    return STATUS_SUCCESS;
}

NTSTATUS
DispatchCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    DbgPrint("irp_wdm: DispatchCreateClose Irp=%p\n", Irp);

    // ****!!!! (5) mark the IRP as complete !!!!****
    // code goes here
    // ****!!!! (5) !!!!****

    return STATUS_SUCCESS;
}

VOID
DriverUnload(_In_ PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNICODE_STRING DeviceLink;

    DbgPrint("irp_wdm: DriverUnload\n");

    // ****!!!! (6) properly cleanup the symbolic link and the device !!!!****
    // code goes here
    // ****!!!! (6) !!!!****
}
