#include <wdm.h>

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath);
VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject);

// global to hold a pointer to our g_thread_object
PVOID g_thread_object = NULL;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#endif

// a dumb thread that doesn't do much
NTSTATUS
DumbThread(PVOID context)
{
    UNREFERENCED_PARAMETER(context);

	// just print something
    DbgPrint("async_wdm: **** I'm a dumb thread and I do nothing useful, but I'm a thread in the kernel! ****\n");

    return STATUS_SUCCESS;
}


// function to clean up threads
VOID
DestroyThread(PVOID thread_object) {
    if (thread_object) {
        // wait for thread to finish
        KeWaitForSingleObject(thread_object, Executive, KernelMode, FALSE, NULL);

        // dereference thread object
        ObDereferenceObject(thread_object);
    }
}

NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    NTSTATUS status;
    HANDLE thread_handle;

    DbgPrint("async_wdm: DriverEntry\n");

	// set up our unload function
    DriverObject->DriverUnload = DriverUnload;

    // create a dumb thread that doesn't do much
    // https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-pscreatesystemthread
    status = PsCreateSystemThread(
        &thread_handle,         // thread handle
        THREAD_ALL_ACCESS,      // desired access
        NULL,                   // object attributes
        NULL,                   // process handle
        NULL,                   // client id
        DumbThread,             // start routine
        NULL);                  // start context

    // something went wrong
    if (!NT_SUCCESS(status)) {
        DbgPrint("async_wdm: PsCreateSystemThread failed!\n");
        goto error;
    }

    // create a reference to the thread object and store it in a global variable
    // https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-obreferenceobjectbyhandle
    status = ObReferenceObjectByHandle(
        thread_handle,          // handle
        THREAD_ALL_ACCESS,      // desired access
        NULL,                   // object type
        KernelMode,             // access mode
        &g_thread_object,       // object
        NULL);                  // handle information

    // something went wrong
    if (!NT_SUCCESS(status)) {
        DbgPrint("async_wdm: ObReferenceObjectByHandle failed!\n");
        goto error;
    }

    // close the local thread handle
    ZwClose(thread_handle);

    return STATUS_SUCCESS;

// something went wrong
error:
    DestroyThread(g_thread_object);
    return status;
}

VOID
DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);

    // destroy our thread
    DestroyThread(g_thread_object);

    DbgPrint("async_wdm: DriverUnload\n");
}
