#include <ntddk.h>
#include <wdf.h>
#include <initguid.h>

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD QueueEvtDeviceAdd;

// predefs
NTSTATUS QueueQueueInitialize(WDFDEVICE Device);
VOID QueueEvtIoRead(WDFQUEUE Queue, WDFREQUEST Request, size_t Length);

// defines
DEFINE_GUID(GUID_DEVINTERFACE_QUEUE, 0x73335581, 0xd1c5, 0x4e33, 0xb0, 0x6c, 0x78, 0x13, 0x1a, 0xf4, 0xbe, 0x95);
#define DOS_DEV_NAME L"\\DosDevices\\kmdf_queue"

typedef struct _REQUEST_CONTEXT {

    ULONG PrivateDeviceData;

} REQUEST_CONTEXT, * PREQUEST_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(REQUEST_CONTEXT, RequestGetContext);

typedef struct _QUEUE_CONTEXT {
    PVOID       Buffer;
    ULONG       Length;

    WDFREQUEST  CurrentRequest;
    NTSTATUS    CurrentStatus;

} QUEUE_CONTEXT, *PQUEUE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(QUEUE_CONTEXT, QueueGetContext)

typedef struct _DEVICE_CONTEXT
{
    PVOID Dummy;

} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE(DEVICE_CONTEXT)


NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;

    WDF_DRIVER_CONFIG_INIT(&config, QueueEvtDeviceAdd);

    status = WdfDriverCreate(DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &config,
        WDF_NO_HANDLE);

    if (!NT_SUCCESS(status)) {
        KdPrint(("queue_kmdf: WdfDriverCreate failed: 0x%x\n", status));
    }

    return status;
}

NTSTATUS
DeviceCreate(_In_ PWDFDEVICE_INIT DeviceInit)
{
    WDF_OBJECT_ATTRIBUTES attributes;
    PDEVICE_CONTEXT deviceContext;
    WDFDEVICE device;
    NTSTATUS status;

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "queue_kmdf: DeviceCreate\n"));

    DECLARE_CONST_UNICODE_STRING(dosDeviceName, DOS_DEV_NAME);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, REQUEST_CONTEXT);
    WdfDeviceInitSetRequestAttributes(DeviceInit, &attributes);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DEVICE_CONTEXT);
    status = WdfDeviceCreate(&DeviceInit, &attributes, &device);

    if (NT_SUCCESS(status)) {

        status = WdfDeviceCreateSymbolicLink(device, &dosDeviceName);

        status = WdfDeviceCreateDeviceInterface(
            device,
            &GUID_DEVINTERFACE_QUEUE,
            NULL // ReferenceString
        );

        if (NT_SUCCESS(status)) {
            status = QueueQueueInitialize(device);
        }
        deviceContext = WdfObjectGet_DEVICE_CONTEXT(device);
    }

    return status;
}

NTSTATUS
QueueEvtDeviceAdd(_In_ WDFDRIVER Driver, _In_ PWDFDEVICE_INIT DeviceInit)
{
    UNREFERENCED_PARAMETER(Driver);
    UNREFERENCED_PARAMETER(DeviceInit);

    NTSTATUS status = 0;

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "queue_kmdf: QueueEvtDeviceAdd.\n"));

    status = DeviceCreate(DeviceInit);

    return status;
}

VOID
QueueEvtIoQueueContextDestroy(_In_ WDFOBJECT Object)
{
    PQUEUE_CONTEXT queueContext = QueueGetContext(Object);

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "queue_kmdf: QueueEvtIoQueueContextDestroy\n"));

    if (queueContext->Buffer != NULL) {
        ExFreePool(queueContext->Buffer);
        queueContext->Buffer = NULL;
    }

    return;
}

NTSTATUS
QueueQueueInitialize(_In_ WDFDEVICE Device)
{
    WDFQUEUE queue;
    NTSTATUS status;
    PQUEUE_CONTEXT queueContext;
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDF_OBJECT_ATTRIBUTES attributes;

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "queue_kmdf: QueueQueueInitialize\n"));

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
        &queueConfig,
        WdfIoQueueDispatchSequential
    );

    queueConfig.EvtIoRead = QueueEvtIoRead;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, QUEUE_CONTEXT);

    attributes.EvtDestroyCallback = QueueEvtIoQueueContextDestroy;

    status = WdfIoQueueCreate(
        Device,
        &queueConfig,
        &attributes,
        &queue
    );

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "queue_kmdf: WdfIoQueueCreate failed: 0x%x\n", status));
        return status;
    }

    // Get our Driver Context memory from the returned Queue handle
    queueContext = QueueGetContext(queue);

    queueContext->Buffer = NULL;

    queueContext->CurrentRequest = NULL;
    queueContext->CurrentStatus = STATUS_INVALID_DEVICE_REQUEST;

    return status;
}

VOID
QueueSetCurrentRequest(_In_ WDFREQUEST Request, _In_ WDFQUEUE Queue)
{
    PQUEUE_CONTEXT queueContext;

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "queue_kmdf: QueueSetCurrentRequest\n"));

    queueContext = QueueGetContext(Queue);

    queueContext->CurrentRequest = Request;
    queueContext->CurrentStatus = STATUS_SUCCESS;
}

VOID
QueueEvtIoRead(_In_ WDFQUEUE Queue, _In_ WDFREQUEST Request, _In_ size_t Length)
{
    NTSTATUS Status;
    WDFMEMORY memory;
    PUCHAR randBuffer = NULL;

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "queue_kmdf: QueueEvtIoRead\n"));

    _Analysis_assume_(Length > 0);

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "queue_kmdf: QueueEvtIoRead Called! Queue 0x%p, Request 0x%p Length %Iu\n", Queue, Request, Length));

    Status = WdfRequestRetrieveOutputMemory(Request, &memory);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("queue_kmdf:  QueueEvtIoRead failed: 0x%x\n", Status));
        WdfVerifierDbgBreakPoint();
        WdfRequestCompleteWithInformation(Request, Status, 0L);
        return;
    }

    randBuffer = WdfMemoryGetBuffer(memory, NULL);

    _Analysis_assume_(randBuffer != NULL);

    //DevRandomFillBufferRand(randBuffer, Length);

    WdfRequestSetInformation(Request, (ULONG_PTR)Length);

    WdfRequestComplete(Request, Status);

    return;
}