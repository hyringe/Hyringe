#include "nonpnp.h"

#pragma pack(1)


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#pragma alloc_text( PAGE, NonPnpDeviceAdd)
#pragma alloc_text( PAGE, NonPnpEvtDriverUnload)
#pragma alloc_text( PAGE, FileEvtIoWrite)

#pragma alloc_text( PAGE, perform_memory )
#pragma alloc_text( PAGE, perform_memory_log_result )
#pragma alloc_text( PAGE, perform_memory_log_output )
#pragma alloc_text( PAGE, perform_memory_log_result_output )
#pragma alloc_text( PAGE, perform_memory_log_exectime )
#pragma alloc_text( PAGE, perform_memory_log_exectime_result )
#pragma alloc_text( PAGE, perform_memory_log_exectime_output )
#pragma alloc_text( PAGE, perform_memory_log_exectime_result_output )
#pragma alloc_text( PAGE, perform_memory_log_timestamps )
#pragma alloc_text( PAGE, perform_memory_log_timestamps_result )
#pragma alloc_text( PAGE, perform_memory_log_timestamps_output )
#pragma alloc_text( PAGE, perform_memory_log_timestamps_result_output )
#pragma alloc_text( PAGE, perform_file )
#pragma alloc_text( PAGE, perform_file_log_result )
#pragma alloc_text( PAGE, perform_file_log_output )
#pragma alloc_text( PAGE, perform_file_log_result_output )
#pragma alloc_text( PAGE, perform_file_log_exectime)
#pragma alloc_text( PAGE, perform_file_log_exectime_result )
#pragma alloc_text( PAGE, perform_file_log_exectime_output )
#pragma alloc_text( PAGE, perform_file_log_exectime_result_output )
#pragma alloc_text( PAGE, perform_file_log_timestamps )
#pragma alloc_text( PAGE, perform_file_log_timestamps_result )
#pragma alloc_text( PAGE, perform_file_log_timestamps_output )
#pragma alloc_text( PAGE, perform_file_log_timestamps_result_output )
#endif // ALLOC_PRAGMA

PVOID input_page_virtual, output_page_virtual, hypercall_page_virtual;
PHYSICAL_ADDRESS input_page_physical, output_page_physical, hypercall_page_physical;
UINT64(*PHV_PERFORM_HYPERCALL)(UINT64 hypercall_info, PHYSICAL_ADDRESS input, PHYSICAL_ADDRESS output);

PVOID campaign_buffer, campaign_buffer_end, log_buffer;

struct campaign_info {
	UINT32 size;
	UINT32 num_calls;
	UINT32 num_waits;
};

struct campaign_entry {
	UINT8 type;
	union {
		struct {
			UINT32 waiting_time;
			UINT16 reserved;
		} waiting_info;
		
		struct {
			UINT16 callcode;
			UINT16 count;
			UINT16 input_page_size;
		} hypercall_info;
	} info;
};

NTSTATUS
DriverEntry(
    IN OUT PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING      RegistryPath
    )
/*++

Routine Description:
    This routine is called by the Operating System to initialize the driver.

    It creates the device object, fills in the dispatch entry points and
    completes the initialization.

Arguments:
    DriverObject - a pointer to the object that represents this device
    driver.

    RegistryPath - a pointer to our Services key in the registry.

Return Value:
    STATUS_SUCCESS if initialized; an error otherwise.

--*/
{
    NTSTATUS                       status;
    WDF_DRIVER_CONFIG              config;
    WDFDRIVER                      hDriver;
    PWDFDEVICE_INIT                pInit = NULL;

    KdPrint(("Driver Frameworks NONPNP Legacy Driver Example\n"));


    WDF_DRIVER_CONFIG_INIT(
        &config,
        WDF_NO_EVENT_CALLBACK // This is a non-pnp driver.
        );

    //
    // Tell the framework that this is non-pnp driver so that it doesn't
    // set the default AddDevice routine.
    //
    config.DriverInitFlags |= WdfDriverInitNonPnpDriver;

    //
    // NonPnp driver must explicitly register an unload routine for
    // the driver to be unloaded.
    //
    config.EvtDriverUnload = NonPnpEvtDriverUnload;


    //
    // Create a framework driver object to represent our driver.
    //
    status = WdfDriverCreate(DriverObject,
                            RegistryPath,
                            WDF_NO_OBJECT_ATTRIBUTES,
                            &config,
                            &hDriver);
    if (!NT_SUCCESS(status)) {
        KdPrint (("NonPnp: WdfDriverCreate failed with status 0x%x\n", status));
        return status;
    }


    //
    // In order to create a control device, we first need to allocate a
    // WDFDEVICE_INIT structure and set all properties.
    //
    pInit = WdfControlDeviceInitAllocate(
                            hDriver,
                            &SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RW_RES_R
                            );

    if (pInit == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        return status;
    }

    //
    // Call NonPnpDeviceAdd to create a deviceobject to represent our
    // software device.
    //
    status = NonPnpDeviceAdd(hDriver, pInit);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Adding control device failed with status 0x%x\n", status));
		return status;
	}

	PHYSICAL_ADDRESS physMaxAddr = { 0 };
	physMaxAddr.QuadPart = (LONGLONG)-1;

	//
	// Map Hypercallpage into virtual memory
	//
	UINT64 msr = __readmsr(0x40000001);
	//if (!(msr & 0x1) || !(msr >> 12)) {
	//	MmFreeContiguousMemory(input_page_virtual);
	//	MmFreeContiguousMemory(output_page_virtual);
	//}
	hypercall_page_physical.QuadPart = (msr >> 12) << 12;
	hypercall_page_virtual = MmMapIoSpace(hypercall_page_physical, PAGE_SIZE, MmNonCached);
	PHV_PERFORM_HYPERCALL = (UINT64(*)(UINT64, PHYSICAL_ADDRESS, PHYSICAL_ADDRESS)) hypercall_page_virtual;

	//
	// Allocate hypercall input page
	//
	input_page_virtual = MmAllocateContiguousMemory(PAGE_SIZE, physMaxAddr);
	if (input_page_virtual == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		return status;
	}
	RtlZeroMemory(input_page_virtual, PAGE_SIZE);
	input_page_physical = MmGetPhysicalAddress(input_page_virtual);

	//
	// Allocate hypercall output page
	//
	output_page_virtual = MmAllocateContiguousMemory(PAGE_SIZE, physMaxAddr);
	if (output_page_virtual == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		return status;
	}
	RtlZeroMemory(output_page_virtual, PAGE_SIZE);
	output_page_physical = MmGetPhysicalAddress(output_page_virtual);

    return status;
}

NTSTATUS
NonPnpDeviceAdd(
    IN WDFDRIVER Driver,
    IN PWDFDEVICE_INIT DeviceInit
    )
/*++

Routine Description:

    Called by the DriverEntry to create a control-device. This call is
    responsible for freeing the memory for DeviceInit.

Arguments:

    DriverObject - a pointer to the object that represents this device
    driver.

    DeviceInit - Pointer to a driver-allocated WDFDEVICE_INIT structure.

Return Value:

    STATUS_SUCCESS if initialized; an error otherwise.

--*/
{
    NTSTATUS                       status;
    WDF_IO_QUEUE_CONFIG      ioQueueConfig;
    WDF_FILEOBJECT_CONFIG fileConfig;
    WDFQUEUE                            queue;
    WDFDEVICE   controlDevice;
    DECLARE_CONST_UNICODE_STRING(ntDeviceName, NTDEVICE_NAME_STRING) ;
    DECLARE_CONST_UNICODE_STRING(symbolicLinkName, SYMBOLIC_NAME_STRING) ;

    UNREFERENCED_PARAMETER( Driver );

    PAGED_CODE();

    //
    // Set exclusive to TRUE so that no more than one app can talk to the
    // control device at any time.
    //
    WdfDeviceInitSetExclusive(DeviceInit, TRUE);

    WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoBuffered);


    status = WdfDeviceInitAssignName(DeviceInit, &ntDeviceName);

    if (!NT_SUCCESS(status)) {
        goto End;
    }


    //
    // Initialize WDF_FILEOBJECT_CONFIG_INIT struct to tell the
    // framework whether you are interested in handling Create, Close and
    // Cleanup requests that gets generated when an application or another
    // kernel component opens an handle to the device. If you don't register
    // the framework default behaviour would be to complete these requests
    // with STATUS_SUCCESS. A driver might be interested in registering these
    // events if it wants to do security validation and also wants to maintain
    // per handle (fileobject) context.
    //

    WDF_FILEOBJECT_CONFIG_INIT(
                        &fileConfig,
						WDF_NO_EVENT_CALLBACK,
						WDF_NO_EVENT_CALLBACK,
                        WDF_NO_EVENT_CALLBACK // not interested in Cleanup
                        );

    WdfDeviceInitSetFileObjectConfig(DeviceInit,
                                       &fileConfig,
                                       WDF_NO_OBJECT_ATTRIBUTES);


    status = WdfDeviceCreate(&DeviceInit,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &controlDevice);
    if (!NT_SUCCESS(status)) {
        goto End;
    }

    //
    // Create a symbolic link for the control object so that usermode can open
    // the device.
    //


    status = WdfDeviceCreateSymbolicLink(controlDevice,
                                &symbolicLinkName);

    if (!NT_SUCCESS(status)) {
        //
        // Control device will be deleted automatically by the framework.
        //
        goto End;
    }

    //
    // Configure a default queue so that requests that are not
    // configure-fowarded using WdfDeviceConfigureRequestDispatching to goto
    // other queues get dispatched here.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig,
                                    WdfIoQueueDispatchSequential);

    ioQueueConfig.EvtIoWrite = FileEvtIoWrite;

    //
    // By default, Static Driver Verifier (SDV) displays a warning if it 
    // doesn't find the EvtIoStop callback on a power-managed queue. 
    // The 'assume' below causes SDV to suppress this warning. If the driver 
    // has not explicitly set PowerManaged to WdfFalse, the framework creates
    // power-managed queues when the device is not a filter driver.  Normally 
    // the EvtIoStop is required for power-managed queues, but for this driver
    // it is not needed b/c the driver doesn't hold on to the requests or 
    // forward them to other drivers. This driver completes the requests 
    // directly in the queue's handlers. If the EvtIoStop callback is not 
    // implemented, the framework waits for all driver-owned requests to be
    // done before moving in the Dx/sleep states or before removing the 
    // device, which is the correct behavior for this type of driver.
    // If the requests were taking an indeterminate amount of time to complete,
    // or if the driver forwarded the requests to a lower driver/another stack,
    // the queue should have an EvtIoStop/EvtIoResume.
    //
    __analysis_assume(ioQueueConfig.EvtIoStop != 0);
    status = WdfIoQueueCreate(controlDevice,
                              &ioQueueConfig,
							  WDF_NO_OBJECT_ATTRIBUTES,
                              &queue // pointer to default queue
                              );
    __analysis_assume(ioQueueConfig.EvtIoStop == 0);
    if (!NT_SUCCESS(status)) {
        goto End;
    }

    //
    // Control devices must notify WDF when they are done initializing.   I/O is
    // rejected until this call is made.
    //
    WdfControlFinishInitializing(controlDevice);

End:
    //
    // If the device is created successfully, framework would clear the
    // DeviceInit value. Otherwise device create must have failed so we
    // should free the memory ourself.
    //
    if (DeviceInit != NULL) {
        WdfDeviceInitFree(DeviceInit);
    }

    return status;

}



VOID
FileEvtIoWrite(
	IN WDFQUEUE         Queue,
	IN WDFREQUEST       Request,
	IN size_t            Length
)
/*++

Routine Description:

	This event is called when the framework receives IRP_MJ_WRITE requests.

Arguments:

	Queue -  Handle to the framework queue object that is associated with the
			I/O request.
	Request - Handle to a framework request object.

	Length  - number of bytes to be written.
				   Queue is by default configured to fail zero length read & write requests.


Return Value:

   None
--*/
{
	NTSTATUS status = STATUS_SUCCESS;
	PVOID inBuf;
	ULONG_PTR bytesWritten = 0;
	UNICODE_STRING input_filename, output_filename;
	OBJECT_ATTRIBUTES fileAttributes;
	HANDLE input_file, output_file;
	IO_STATUS_BLOCK io_status;
	struct campaign_info info;
	PVOID campaign_buffer_start = 0, log_buffer_start = 0;
	SIZE_T log_buffer_size = 0;

	struct flags {
		UINT32 exectime : 1;
		UINT32 timestamps : 1;
		UINT32 memory : 1;
		UINT32 result : 1;
		UINT32 output : 1;
		UINT32 reserved : 27;
	};

	DECLARE_CONST_UNICODE_STRING(output_file_ending, OUTPUT_FILE_ENDING_STRING);


	UNREFERENCED_PARAMETER(Queue);

	PAGED_CODE();

	status = WdfRequestRetrieveInputBuffer(Request, 0, &inBuf, 0);
	if (!NT_SUCCESS(status)) {
		WdfRequestComplete(Request, status);
		return;
	}

	struct flags *flags_received = (struct flags *)inBuf;

	input_filename.Length = (USHORT) Length - sizeof(UINT32);
	input_filename.MaximumLength = input_filename.Length;
	input_filename.Buffer = (PWCH)((PUINT32) inBuf + 1);

	output_filename.Length = 0;
	output_filename.MaximumLength = (USHORT) (Length + output_file_ending.Length);
	output_filename.Buffer = ExAllocatePool(PagedPool, output_filename.MaximumLength);
	if (output_filename.Buffer == NULL) {
		WdfRequestComplete(Request, STATUS_INSUFFICIENT_RESOURCES);
		return;
	}
	RtlAppendUnicodeStringToString(&output_filename, &input_filename);
	RtlAppendUnicodeStringToString(&output_filename, &output_file_ending);


	InitializeObjectAttributes( &fileAttributes,
								&input_filename,
								OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
								NULL,
								NULL
								);

	status = ZwCreateFile(  &input_file,
							SYNCHRONIZE | GENERIC_READ,
							&fileAttributes,
							&io_status,
							NULL,
							FILE_ATTRIBUTE_NORMAL,
							FILE_SHARE_READ,
							FILE_OPEN,
							FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE,
							NULL,
							0
							);

	if (!NT_SUCCESS(status)) {
		WdfRequestComplete(Request, status);
		return;
	}


	InitializeObjectAttributes(&fileAttributes,
		&output_filename,
		OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
		NULL,
		NULL
	);

	status = ZwCreateFile(&output_file,
		SYNCHRONIZE | GENERIC_WRITE,
		&fileAttributes,
		&io_status,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ,
		FILE_OVERWRITE_IF,
		FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE,
		NULL,
		0
	);

	if (!NT_SUCCESS(status)) {
		WdfRequestComplete(Request, status);
		return;
	}

	status = ZwWriteFile(output_file,
		NULL,
		NULL,
		NULL,
		&io_status,
		flags_received,
		sizeof(struct flags),
		0,
		NULL);

	if (!NT_SUCCESS(status)) {
		WdfRequestComplete(Request, status);
		return;
	}


	status = ZwReadFile(input_file,
		NULL,
		NULL,
		NULL,
		&io_status,
		&info,
		sizeof(info),
		0,
		NULL);

	if (!NT_SUCCESS(status)) {
		WdfRequestComplete(Request, status);
		return;
	}

	if (flags_received->memory) {
		campaign_buffer = ExAllocatePool(PagedPool, info.size);
		if (campaign_buffer == 0) {
			WdfRequestComplete(Request, STATUS_MEMORY_NOT_ALLOCATED);
			return;
		}

		log_buffer_size = ((flags_received->exectime != 0) * 8 + (flags_received->timestamps != 0) * 16 + (flags_received->result != 0) * 8 + (flags_received->output != 0) * 4096) * info.num_calls +
						  ((flags_received->exectime != 0) * 8 + (flags_received->timestamps != 0) * 16) * info.num_waits;
		log_buffer = ExAllocatePool(PagedPool, log_buffer_size);
		if (log_buffer == 0) {
			ExFreePool(campaign_buffer);
			WdfRequestComplete(Request, STATUS_MEMORY_NOT_ALLOCATED);
			return;
		}

		status = ZwReadFile(input_file,
			NULL,
			NULL,
			NULL,
			&io_status,
			campaign_buffer,
			info.size,
			0,
			NULL);

		if (!NT_SUCCESS(status)) {
			WdfRequestComplete(Request, status);
			return;
		}

		campaign_buffer_start = campaign_buffer;
		campaign_buffer_end = (UINT8 *)campaign_buffer + info.size;
		log_buffer_start = log_buffer;
	}

	
	
	if (flags_received->memory && !flags_received->exectime && !flags_received->timestamps && !flags_received->result && !flags_received->output)
		status = perform_memory(input_file, output_file);
	else if (flags_received->memory && !flags_received->exectime && !flags_received->timestamps && flags_received->result && !flags_received->output)
		status = perform_memory_log_result(input_file, output_file);
	else if (flags_received->memory && !flags_received->exectime && !flags_received->timestamps && !flags_received->result && flags_received->output)
		status = perform_memory_log_output(input_file, output_file);
	else if (flags_received->memory && !flags_received->exectime && !flags_received->timestamps && flags_received->result && flags_received->output)
		status = perform_memory_log_result_output(input_file, output_file);
	else if (flags_received->memory && flags_received->exectime && !flags_received->timestamps && !flags_received->result && !flags_received->output)
		status = perform_memory_log_exectime(input_file, output_file);
	else if (flags_received->memory && flags_received->exectime && !flags_received->timestamps && flags_received->result && !flags_received->output)
		status = perform_memory_log_exectime_result(input_file, output_file);
	else if (flags_received->memory && flags_received->exectime && !flags_received->timestamps && !flags_received->result && flags_received->output)
		status = perform_memory_log_exectime_output(input_file, output_file);
	else if (flags_received->memory && flags_received->exectime && !flags_received->timestamps && flags_received->result && flags_received->output)
		status = perform_memory_log_exectime_result_output(input_file, output_file);
	else if (flags_received->memory && !flags_received->exectime && flags_received->timestamps && !flags_received->result && !flags_received->output)
		status = perform_memory_log_timestamps(input_file, output_file);
	else if (flags_received->memory && !flags_received->exectime && flags_received->timestamps && flags_received->result && !flags_received->output)
		status = perform_memory_log_timestamps_result(input_file, output_file);
	else if (flags_received->memory && !flags_received->exectime && flags_received->timestamps && !flags_received->result && flags_received->output)
		status = perform_memory_log_timestamps_output(input_file, output_file);
	else if (flags_received->memory && !flags_received->exectime && flags_received->timestamps && flags_received->result && flags_received->output)
		status = perform_memory_log_timestamps_result_output(input_file, output_file);
	else if (!flags_received->memory && !flags_received->exectime && !flags_received->timestamps && !flags_received->result && !flags_received->output)
		status = perform_file(input_file, output_file);
	else if (!flags_received->memory && !flags_received->exectime && !flags_received->timestamps && flags_received->result && !flags_received->output)
		status = perform_file_log_result(input_file, output_file);
	else if (!flags_received->memory && !flags_received->exectime && !flags_received->timestamps && !flags_received->result && flags_received->output)
		status = perform_file_log_output(input_file, output_file);
	else if (!flags_received->memory && !flags_received->exectime && !flags_received->timestamps && flags_received->result && flags_received->output)
		status = perform_file_log_result_output(input_file, output_file);
	else if (!flags_received->memory && flags_received->exectime && !flags_received->timestamps && !flags_received->result && !flags_received->output)
		status = perform_file_log_exectime(input_file, output_file);
	else if (!flags_received->memory && flags_received->exectime && !flags_received->timestamps && flags_received->result && !flags_received->output)
		status = perform_file_log_exectime_result(input_file, output_file);
	else if (!flags_received->memory && flags_received->exectime && !flags_received->timestamps && !flags_received->result && flags_received->output)
		status = perform_file_log_exectime_output(input_file, output_file);
	else if (!flags_received->memory && flags_received->exectime && !flags_received->timestamps && flags_received->result && flags_received->output)
		status = perform_file_log_exectime_result_output(input_file, output_file);
	else if (!flags_received->memory && !flags_received->exectime && flags_received->timestamps && !flags_received->result && !flags_received->output)
		status = perform_file_log_timestamps(input_file, output_file);
	else if (!flags_received->memory && !flags_received->exectime && flags_received->timestamps && flags_received->result && !flags_received->output)
		status = perform_file_log_timestamps_result(input_file, output_file);
	else if (!flags_received->memory && !flags_received->exectime && flags_received->timestamps && !flags_received->result && flags_received->output)
		status = perform_file_log_timestamps_output(input_file, output_file);
	else if (!flags_received->memory && !flags_received->exectime && flags_received->timestamps && flags_received->result && flags_received->output)
		status = perform_file_log_timestamps_result_output(input_file, output_file);
	else
		status = STATUS_NOINTERFACE;

	if (flags_received->memory) {
		status = ZwWriteFile(output_file,
			NULL,
			NULL,
			NULL,
			&io_status,
			log_buffer_start,
			(ULONG)log_buffer_size,
			0,
			NULL);

		if (!NT_SUCCESS(status)) {
			WdfRequestComplete(Request, status);
			return;
		}

		ExFreePool(campaign_buffer_start);
		ExFreePool(log_buffer_start);
	}

	ZwClose(input_file);
	ZwClose(output_file);


	ExFreePool(output_filename.Buffer);

    WdfRequestCompleteWithInformation(Request, status, bytesWritten);

}




VOID
NonPnpEvtDriverUnload(
    IN WDFDRIVER Driver
    )
/*++
Routine Description:

   Called by the I/O subsystem just before unloading the driver.
   You can free the resources created in the DriverEntry either
   in this routine or in the EvtDriverContextCleanup callback.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

Return Value:

    NTSTATUS

--*/
{
    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

	//
	// Clean up
	//
	MmFreeContiguousMemory(input_page_virtual);
	MmFreeContiguousMemory(output_page_virtual);
	MmUnmapIoSpace(hypercall_page_virtual, PAGE_SIZE);

    return;
}



NTSTATUS perform_memory(HANDLE input, HANDLE output){
	UNREFERENCED_PARAMETER(input);
	UNREFERENCED_PARAMETER(output);

	struct campaign_entry *entry;

	UINT8  **cam_buf_u8 = &(UINT8  *)campaign_buffer;

	while (campaign_buffer < campaign_buffer_end) {
		entry = campaign_buffer;
		*cam_buf_u8 += 7; //sizeof(entry) delivers 8...

		switch (entry->type) {
		case TYPE_WAIT:
			KeStallExecutionProcessor(entry->info.waiting_info.waiting_time);
			// KeDelayExecutionThread for longer waits?

			break;
		case TYPE_CALL:
			memcpy(input_page_virtual, campaign_buffer, entry->info.hypercall_info.input_page_size);
			*cam_buf_u8 += entry->info.hypercall_info.input_page_size;

			for (; entry->info.hypercall_info.count > 0; entry->info.hypercall_info.count--) {
				PHV_PERFORM_HYPERCALL(entry->info.hypercall_info.callcode, input_page_physical, output_page_physical);
			}
		}
	}
	return STATUS_SUCCESS;
}

NTSTATUS perform_memory_log_result(HANDLE input, HANDLE output) {
	UNREFERENCED_PARAMETER(input);
	UNREFERENCED_PARAMETER(output);

	struct campaign_entry *entry;
	UINT64 result_val;

	UINT8  **cam_buf_u8 = &(UINT8  *)campaign_buffer;
	UINT64 **log_buf_u64 = &(UINT64 *)log_buffer;

	while (campaign_buffer < campaign_buffer_end) {
		entry = campaign_buffer;
		*cam_buf_u8 += 7; //sizeof(entry) delivers 8...

		switch (entry->type) {
		case TYPE_WAIT:
			KeStallExecutionProcessor(entry->info.waiting_info.waiting_time);
			// KeDelayExecutionThread for longer waits?

			break;
		case TYPE_CALL:
			memcpy(input_page_virtual, campaign_buffer, entry->info.hypercall_info.input_page_size);
			*cam_buf_u8 += entry->info.hypercall_info.input_page_size;

			for (; entry->info.hypercall_info.count > 0; entry->info.hypercall_info.count--) {
				result_val = PHV_PERFORM_HYPERCALL(entry->info.hypercall_info.callcode, input_page_physical, output_page_physical);

				*(*log_buf_u64)++ = result_val;
			}
		}
	}
	return STATUS_SUCCESS;
}

NTSTATUS perform_memory_log_output(HANDLE input, HANDLE output) {
	UNREFERENCED_PARAMETER(input);
	UNREFERENCED_PARAMETER(output);

	struct campaign_entry *entry;

	UINT8  **cam_buf_u8 = &(UINT8  *)campaign_buffer;
	UINT8  **log_buf_u8 = &(UINT8  *)log_buffer;

	while (campaign_buffer < campaign_buffer_end) {
		entry = campaign_buffer;
		*cam_buf_u8 += 7; //sizeof(entry) delivers 8...

		switch (entry->type) {
		case TYPE_WAIT:
			KeStallExecutionProcessor(entry->info.waiting_info.waiting_time);
			// KeDelayExecutionThread for longer waits?

			break;
		case TYPE_CALL:
			memcpy(input_page_virtual, campaign_buffer, entry->info.hypercall_info.input_page_size);
			*cam_buf_u8 += entry->info.hypercall_info.input_page_size;

			for (; entry->info.hypercall_info.count > 0; entry->info.hypercall_info.count--) {
				PHV_PERFORM_HYPERCALL(entry->info.hypercall_info.callcode, input_page_physical, output_page_physical);

				memcpy(log_buffer, output_page_virtual, PAGE_SIZE);
				*log_buf_u8 += PAGE_SIZE;

				RtlZeroMemory(output_page_virtual, PAGE_SIZE);
			}
		}
	}
	return STATUS_SUCCESS;
}

NTSTATUS perform_memory_log_result_output(HANDLE input, HANDLE output) {
	UNREFERENCED_PARAMETER(input);
	UNREFERENCED_PARAMETER(output);
	
	struct campaign_entry *entry;
	UINT64 result_val;

	UINT8  **cam_buf_u8 = &(UINT8  *)campaign_buffer;
	UINT64 **log_buf_u64 = &(UINT64 *)log_buffer;
	UINT8  **log_buf_u8 = &(UINT8  *)log_buffer;

	while (campaign_buffer < campaign_buffer_end) {
		entry = campaign_buffer;
		*cam_buf_u8 += 7; //sizeof(entry) delivers 8...

		switch (entry->type) {
		case TYPE_WAIT:
			KeStallExecutionProcessor(entry->info.waiting_info.waiting_time);
			// KeDelayExecutionThread for longer waits?

			break;
		case TYPE_CALL:
			memcpy(input_page_virtual, campaign_buffer, entry->info.hypercall_info.input_page_size);
			*cam_buf_u8 += entry->info.hypercall_info.input_page_size;

			for (; entry->info.hypercall_info.count > 0; entry->info.hypercall_info.count--) {
				result_val = PHV_PERFORM_HYPERCALL(entry->info.hypercall_info.callcode, input_page_physical, output_page_physical);

				*(*log_buf_u64)++ = result_val;

				memcpy(log_buffer, output_page_virtual, PAGE_SIZE);
				*log_buf_u8 += PAGE_SIZE;

				RtlZeroMemory(output_page_virtual, PAGE_SIZE);
			}
		}
	}
	return STATUS_SUCCESS;
}

NTSTATUS perform_memory_log_exectime(HANDLE input, HANDLE output){
	UNREFERENCED_PARAMETER(input);
	UNREFERENCED_PARAMETER(output);
	struct campaign_entry *entry;
	LARGE_INTEGER start_time, end_time;
	LONGLONG exec_time;

	UINT8  **cam_buf_u8 = &(UINT8  *)campaign_buffer;
	UINT64 **log_buf_u64 = &(UINT64 *)log_buffer;

	while (campaign_buffer < campaign_buffer_end) {
		entry = campaign_buffer;
		*cam_buf_u8 += 7; //sizeof(entry) delivers 8...

		switch (entry->type) {
		case TYPE_WAIT:
			KeQuerySystemTimePrecise(&start_time);
			KeStallExecutionProcessor(entry->info.waiting_info.waiting_time);
			// KeDelayExecutionThread for longer waits?
			KeQuerySystemTimePrecise(&end_time);
			exec_time = end_time.QuadPart - start_time.QuadPart;

			*(*log_buf_u64)++ = exec_time;

			break;
		case TYPE_CALL:
			memcpy(input_page_virtual, campaign_buffer, entry->info.hypercall_info.input_page_size);
			*cam_buf_u8 += entry->info.hypercall_info.input_page_size;

			for (; entry->info.hypercall_info.count > 0; entry->info.hypercall_info.count--) {
				KeQuerySystemTimePrecise(&start_time);
				PHV_PERFORM_HYPERCALL(entry->info.hypercall_info.callcode, input_page_physical, output_page_physical);
				KeQuerySystemTimePrecise(&end_time);
				exec_time = end_time.QuadPart - start_time.QuadPart;

				*(*log_buf_u64)++ = exec_time;
			}
		}
	}
	return STATUS_SUCCESS;
}

NTSTATUS perform_memory_log_exectime_result(HANDLE input, HANDLE output){
	UNREFERENCED_PARAMETER(input);
	UNREFERENCED_PARAMETER(output);
	
	struct campaign_entry *entry;
	LARGE_INTEGER start_time, end_time;
	LONGLONG exec_time;
	UINT64 result_val;

	UINT8  **cam_buf_u8 = &(UINT8  *)campaign_buffer;
	UINT64 **log_buf_u64 = &(UINT64 *)log_buffer;

	while (campaign_buffer < campaign_buffer_end) {
		entry = campaign_buffer;
		*cam_buf_u8 += 7; //sizeof(entry) delivers 8...

		switch (entry->type) {
		case TYPE_WAIT:
			KeQuerySystemTimePrecise(&start_time);
			KeStallExecutionProcessor(entry->info.waiting_info.waiting_time);
			// KeDelayExecutionThread for longer waits?
			KeQuerySystemTimePrecise(&end_time);
			exec_time = end_time.QuadPart - start_time.QuadPart;

			*(*log_buf_u64)++ = exec_time;

			break;
		case TYPE_CALL:
			memcpy(input_page_virtual, campaign_buffer, entry->info.hypercall_info.input_page_size);
			*cam_buf_u8 += entry->info.hypercall_info.input_page_size;

			for (; entry->info.hypercall_info.count > 0; entry->info.hypercall_info.count--) {
				KeQuerySystemTimePrecise(&start_time);
				result_val = PHV_PERFORM_HYPERCALL(entry->info.hypercall_info.callcode, input_page_physical, output_page_physical);
				KeQuerySystemTimePrecise(&end_time);
				exec_time = end_time.QuadPart - start_time.QuadPart;

				*(*log_buf_u64)++ = exec_time;

				*(*log_buf_u64)++ = result_val;
			}
		}
	}
	return STATUS_SUCCESS;
}

NTSTATUS perform_memory_log_exectime_output(HANDLE input, HANDLE output){
	UNREFERENCED_PARAMETER(input);
	UNREFERENCED_PARAMETER(output);

	struct campaign_entry *entry;
	LARGE_INTEGER start_time, end_time;
	LONGLONG exec_time;

	UINT8  **cam_buf_u8 = &(UINT8  *)campaign_buffer;
	UINT64 **log_buf_u64 = &(UINT64 *)log_buffer;
	UINT8  **log_buf_u8 = &(UINT8  *)log_buffer;

	while (campaign_buffer < campaign_buffer_end) {
		entry = campaign_buffer;
		*cam_buf_u8 += 7; //sizeof(entry) delivers 8...

		switch (entry->type) {
		case TYPE_WAIT:
			KeQuerySystemTimePrecise(&start_time);
			KeStallExecutionProcessor(entry->info.waiting_info.waiting_time);
			// KeDelayExecutionThread for longer waits?
			KeQuerySystemTimePrecise(&end_time);
			exec_time = end_time.QuadPart - start_time.QuadPart;

			*(*log_buf_u64)++ = exec_time;

			break;
		case TYPE_CALL:
			memcpy(input_page_virtual, campaign_buffer, entry->info.hypercall_info.input_page_size);
			*cam_buf_u8 += entry->info.hypercall_info.input_page_size;

			for (; entry->info.hypercall_info.count > 0; entry->info.hypercall_info.count--) {
				KeQuerySystemTimePrecise(&start_time);
				PHV_PERFORM_HYPERCALL(entry->info.hypercall_info.callcode, input_page_physical, output_page_physical);
				KeQuerySystemTimePrecise(&end_time);
				exec_time = end_time.QuadPart - start_time.QuadPart;

				*(*log_buf_u64)++ = exec_time;

				memcpy(log_buffer, output_page_virtual, PAGE_SIZE);
				*log_buf_u8 += PAGE_SIZE;

				RtlZeroMemory(output_page_virtual, PAGE_SIZE);
			}
		}
	}
	return STATUS_SUCCESS;
}

NTSTATUS perform_memory_log_exectime_result_output(HANDLE input, HANDLE output){
	UNREFERENCED_PARAMETER(input);
	UNREFERENCED_PARAMETER(output);
	
	struct campaign_entry *entry;
	LARGE_INTEGER start_time, end_time;
	LONGLONG exec_time;
	UINT64 result_val;

	UINT8  **cam_buf_u8 = &(UINT8  *)campaign_buffer;
	UINT64 **log_buf_u64 = &(UINT64 *)log_buffer;
	UINT8  **log_buf_u8 = &(UINT8  *)log_buffer;

	while (campaign_buffer < campaign_buffer_end) {
		entry = campaign_buffer;
		*cam_buf_u8 += 7; //sizeof(entry) delivers 8...

		switch (entry->type) {
		case TYPE_WAIT:
			KeQuerySystemTimePrecise(&start_time);
			KeStallExecutionProcessor(entry->info.waiting_info.waiting_time);
			// KeDelayExecutionThread for longer waits?
			KeQuerySystemTimePrecise(&end_time);
			exec_time = end_time.QuadPart - start_time.QuadPart;

			*(*log_buf_u64)++ = exec_time;

			break;
		case TYPE_CALL:
			memcpy(input_page_virtual, campaign_buffer, entry->info.hypercall_info.input_page_size);
			*cam_buf_u8 += entry->info.hypercall_info.input_page_size;

			for (; entry->info.hypercall_info.count > 0; entry->info.hypercall_info.count--) {
				KeQuerySystemTimePrecise(&start_time);
				result_val = PHV_PERFORM_HYPERCALL(entry->info.hypercall_info.callcode, input_page_physical, output_page_physical);
				KeQuerySystemTimePrecise(&end_time);
				exec_time = end_time.QuadPart - start_time.QuadPart;

				*(*log_buf_u64)++ = exec_time;

				*(*log_buf_u64)++ = result_val;

				memcpy(log_buffer, output_page_virtual, PAGE_SIZE);
				*log_buf_u8 += PAGE_SIZE;

				RtlZeroMemory(output_page_virtual, PAGE_SIZE);
			}
		}
	}
	return STATUS_SUCCESS;
}

NTSTATUS perform_memory_log_timestamps(HANDLE input, HANDLE output){
	UNREFERENCED_PARAMETER(input);
	UNREFERENCED_PARAMETER(output);
	
	struct campaign_entry *entry;
	LARGE_INTEGER start_time, end_time;

	UINT8  **cam_buf_u8 = &(UINT8  *)campaign_buffer;
	UINT64 **log_buf_u64 = &(UINT64 *)log_buffer;

	while (campaign_buffer < campaign_buffer_end) {
		entry = campaign_buffer;
		*cam_buf_u8 += 7; //sizeof(entry) delivers 8...

		switch (entry->type) {
		case TYPE_WAIT:
			KeQuerySystemTimePrecise(&start_time);
			KeStallExecutionProcessor(entry->info.waiting_info.waiting_time);
			// KeDelayExecutionThread for longer waits?
			KeQuerySystemTimePrecise(&end_time);

			*(*log_buf_u64)++ = start_time.QuadPart;
			*(*log_buf_u64)++ = end_time.QuadPart;

			break;
		case TYPE_CALL:
			memcpy(input_page_virtual, campaign_buffer, entry->info.hypercall_info.input_page_size);
			*cam_buf_u8 += entry->info.hypercall_info.input_page_size;

			for (; entry->info.hypercall_info.count > 0; entry->info.hypercall_info.count--) {
				KeQuerySystemTimePrecise(&start_time);
				PHV_PERFORM_HYPERCALL(entry->info.hypercall_info.callcode, input_page_physical, output_page_physical);
				KeQuerySystemTimePrecise(&end_time);

				*(*log_buf_u64)++ = start_time.QuadPart;
				*(*log_buf_u64)++ = end_time.QuadPart;
			}
		}
	}
	return STATUS_SUCCESS;
}

NTSTATUS perform_memory_log_timestamps_result(HANDLE input, HANDLE output){
	UNREFERENCED_PARAMETER(input);
	UNREFERENCED_PARAMETER(output);
	
	struct campaign_entry *entry;
	LARGE_INTEGER start_time, end_time;
	UINT64 result_val;

	UINT8  **cam_buf_u8 = &(UINT8  *)campaign_buffer;
	UINT64 **log_buf_u64 = &(UINT64 *)log_buffer;

	while (campaign_buffer < campaign_buffer_end) {
		entry = campaign_buffer;
		*cam_buf_u8 += 7; //sizeof(entry) delivers 8...

		switch (entry->type) {
		case TYPE_WAIT:
			KeQuerySystemTimePrecise(&start_time);
			KeStallExecutionProcessor(entry->info.waiting_info.waiting_time);
			// KeDelayExecutionThread for longer waits?
			KeQuerySystemTimePrecise(&end_time);

			*(*log_buf_u64)++ = start_time.QuadPart;
			*(*log_buf_u64)++ = end_time.QuadPart;

			break;
		case TYPE_CALL:
			memcpy(input_page_virtual, campaign_buffer, entry->info.hypercall_info.input_page_size);
			*cam_buf_u8 += entry->info.hypercall_info.input_page_size;

			for (; entry->info.hypercall_info.count > 0; entry->info.hypercall_info.count--) {
				KeQuerySystemTimePrecise(&start_time);
				result_val = PHV_PERFORM_HYPERCALL(entry->info.hypercall_info.callcode, input_page_physical, output_page_physical);
				KeQuerySystemTimePrecise(&end_time);

				*(*log_buf_u64)++ = start_time.QuadPart;
				*(*log_buf_u64)++ = end_time.QuadPart;

				*(*log_buf_u64)++ = result_val;
			}
		}
	}
	return STATUS_SUCCESS;
}

NTSTATUS perform_memory_log_timestamps_output(HANDLE input, HANDLE output){
	UNREFERENCED_PARAMETER(input);
	UNREFERENCED_PARAMETER(output);
	
	struct campaign_entry *entry;
	LARGE_INTEGER start_time, end_time;

	UINT8  **cam_buf_u8 = &(UINT8  *)campaign_buffer;
	UINT64 **log_buf_u64 = &(UINT64 *)log_buffer;
	UINT8  **log_buf_u8 = &(UINT8  *)log_buffer;

	while (campaign_buffer < campaign_buffer_end) {
		entry = campaign_buffer;
		*cam_buf_u8 += 7; //sizeof(entry) delivers 8...

		switch (entry->type) {
		case TYPE_WAIT:
			KeQuerySystemTimePrecise(&start_time);
			KeStallExecutionProcessor(entry->info.waiting_info.waiting_time);
			// KeDelayExecutionThread for longer waits?
			KeQuerySystemTimePrecise(&end_time);

			*(*log_buf_u64)++ = start_time.QuadPart;
			*(*log_buf_u64)++ = end_time.QuadPart;

			break;
		case TYPE_CALL:
			memcpy(input_page_virtual, campaign_buffer, entry->info.hypercall_info.input_page_size);
			*cam_buf_u8 += entry->info.hypercall_info.input_page_size;

			for (; entry->info.hypercall_info.count > 0; entry->info.hypercall_info.count--) {
				KeQuerySystemTimePrecise(&start_time);
				PHV_PERFORM_HYPERCALL(entry->info.hypercall_info.callcode, input_page_physical, output_page_physical);
				KeQuerySystemTimePrecise(&end_time);

				*(*log_buf_u64)++ = start_time.QuadPart;
				*(*log_buf_u64)++ = end_time.QuadPart;

				memcpy(log_buffer, output_page_virtual, PAGE_SIZE);
				*log_buf_u8 += PAGE_SIZE;

				RtlZeroMemory(output_page_virtual, PAGE_SIZE);
			}
		}
	}
	return STATUS_SUCCESS;
}

NTSTATUS perform_memory_log_timestamps_result_output(HANDLE input, HANDLE output){
	UNREFERENCED_PARAMETER(input);
	UNREFERENCED_PARAMETER(output);
	
	struct campaign_entry *entry;
	LARGE_INTEGER start_time, end_time;
	UINT64 result_val;

	UINT8  **cam_buf_u8  = &(UINT8  *)campaign_buffer;
	UINT64 **log_buf_u64 = &(UINT64 *)log_buffer;
	UINT8  **log_buf_u8  = &(UINT8  *)log_buffer;

	while (campaign_buffer < campaign_buffer_end) {
		entry = campaign_buffer;
		*cam_buf_u8 += 7; //sizeof(entry) delivers 8...

		switch (entry->type) {
		case TYPE_WAIT:
			KeQuerySystemTimePrecise(&start_time);
			KeStallExecutionProcessor(entry->info.waiting_info.waiting_time);
			// KeDelayExecutionThread for longer waits?
			KeQuerySystemTimePrecise(&end_time);

			*(*log_buf_u64)++ = start_time.QuadPart;
			*(*log_buf_u64)++ = end_time.QuadPart;

			break;
		case TYPE_CALL:
			memcpy(input_page_virtual, campaign_buffer, entry->info.hypercall_info.input_page_size);
			*cam_buf_u8 += entry->info.hypercall_info.input_page_size;

			for (; entry->info.hypercall_info.count > 0; entry->info.hypercall_info.count--) {
				KeQuerySystemTimePrecise(&start_time);
				result_val = PHV_PERFORM_HYPERCALL(entry->info.hypercall_info.callcode, input_page_physical, output_page_physical);
				KeQuerySystemTimePrecise(&end_time);

				*(*log_buf_u64)++ = start_time.QuadPart;
				*(*log_buf_u64)++ = end_time.QuadPart;

				*(*log_buf_u64)++ = result_val;

				memcpy(log_buffer, output_page_virtual, PAGE_SIZE);
				*log_buf_u8 += PAGE_SIZE;

				RtlZeroMemory(output_page_virtual, PAGE_SIZE);
			}
		}
	}
	return STATUS_SUCCESS;
}

NTSTATUS perform_file(HANDLE input, HANDLE output){
	UNREFERENCED_PARAMETER(output);
	NTSTATUS status = STATUS_SUCCESS;
	IO_STATUS_BLOCK io_status;
	struct campaign_entry entry;

	while (1) {
		status = ZwReadFile(input,
			NULL,
			NULL,
			NULL,
			&io_status,
			&entry,
			sizeof(entry),
			0,
			NULL);

		if (!NT_SUCCESS(status)) {
			if (status == STATUS_END_OF_FILE && io_status.Information == 0) {
				return STATUS_SUCCESS;
			}
		}

		switch (entry.type) {
		case TYPE_WAIT:
			KeStallExecutionProcessor(entry.info.waiting_info.waiting_time);
			// KeDelayExecutionThread for longer waits?

			break;
		case TYPE_CALL:
			status = ZwReadFile(input,
				NULL,
				NULL,
				NULL,
				&io_status,
				input_page_virtual,
				entry.info.hypercall_info.input_page_size,
				0,
				NULL);

			if (!NT_SUCCESS(status)) {
				return status;
			}


			for (; entry.info.hypercall_info.count > 0; entry.info.hypercall_info.count--) {
				PHV_PERFORM_HYPERCALL(entry.info.hypercall_info.callcode, input_page_physical, output_page_physical);
			}
		}
	}
}

NTSTATUS perform_file_log_result(HANDLE input, HANDLE output) {
	NTSTATUS status = STATUS_SUCCESS;
	IO_STATUS_BLOCK io_status;
	struct campaign_entry entry;
	UINT64 result_val;

	while (1) {
		status = ZwReadFile(input,
			NULL,
			NULL,
			NULL,
			&io_status,
			&entry,
			sizeof(entry),
			0,
			NULL);

		if (!NT_SUCCESS(status)) {
			if (status == STATUS_END_OF_FILE && io_status.Information == 0) {
				return STATUS_SUCCESS;
			}
		}

		switch (entry.type) {
		case TYPE_WAIT:
			KeStallExecutionProcessor(entry.info.waiting_info.waiting_time);
			// KeDelayExecutionThread for longer waits?

			break;
		case TYPE_CALL:
			status = ZwReadFile(input,
				NULL,
				NULL,
				NULL,
				&io_status,
				input_page_virtual,
				entry.info.hypercall_info.input_page_size,
				0,
				NULL);

			if (!NT_SUCCESS(status)) {
				return status;
			}


			for (; entry.info.hypercall_info.count > 0; entry.info.hypercall_info.count--) {
				result_val = PHV_PERFORM_HYPERCALL(entry.info.hypercall_info.callcode, input_page_physical, output_page_physical);

				status = ZwWriteFile(output,
					NULL,
					NULL,
					NULL,
					&io_status,
					&result_val,
					sizeof(result_val),
					0,
					NULL);

				if (!NT_SUCCESS(status)) {
					return status;
				}
			}
		}
	}
}

NTSTATUS perform_file_log_output(HANDLE input, HANDLE output) {
	NTSTATUS status = STATUS_SUCCESS;
	IO_STATUS_BLOCK io_status;
	struct campaign_entry entry;

	while (1) {
		status = ZwReadFile(input,
			NULL,
			NULL,
			NULL,
			&io_status,
			&entry,
			sizeof(entry),
			0,
			NULL);

		if (!NT_SUCCESS(status)) {
			if (status == STATUS_END_OF_FILE && io_status.Information == 0) {
				return STATUS_SUCCESS;
			}
		}

		switch (entry.type) {
		case TYPE_WAIT:
			KeStallExecutionProcessor(entry.info.waiting_info.waiting_time);
			// KeDelayExecutionThread for longer waits?

			break;
		case TYPE_CALL:
			status = ZwReadFile(input,
				NULL,
				NULL,
				NULL,
				&io_status,
				input_page_virtual,
				entry.info.hypercall_info.input_page_size,
				0,
				NULL);

			if (!NT_SUCCESS(status)) {
				return status;
			}


			for (; entry.info.hypercall_info.count > 0; entry.info.hypercall_info.count--) {
				PHV_PERFORM_HYPERCALL(entry.info.hypercall_info.callcode, input_page_physical, output_page_physical);

				status = ZwWriteFile(output,
					NULL,
					NULL,
					NULL,
					&io_status,
					output_page_virtual,
					PAGE_SIZE,
					0,
					NULL);

				if (!NT_SUCCESS(status)) {
					return status;
				}

				RtlZeroMemory(output_page_virtual, PAGE_SIZE);
			}
		}
	}
}

NTSTATUS perform_file_log_result_output(HANDLE input, HANDLE output) {
	NTSTATUS status = STATUS_SUCCESS;
	IO_STATUS_BLOCK io_status;
	struct campaign_entry entry;
	UINT64 result_val;

	while (1) {
		status = ZwReadFile(input,
			NULL,
			NULL,
			NULL,
			&io_status,
			&entry,
			sizeof(entry),
			0,
			NULL);

		if (!NT_SUCCESS(status)) {
			if (status == STATUS_END_OF_FILE && io_status.Information == 0) {
				return STATUS_SUCCESS;
			}
		}

		switch (entry.type) {
		case TYPE_WAIT:
			KeStallExecutionProcessor(entry.info.waiting_info.waiting_time);
			// KeDelayExecutionThread for longer waits?

			break;
		case TYPE_CALL:
			status = ZwReadFile(input,
				NULL,
				NULL,
				NULL,
				&io_status,
				input_page_virtual,
				entry.info.hypercall_info.input_page_size,
				0,
				NULL);

			if (!NT_SUCCESS(status)) {
				return status;
			}


			for (; entry.info.hypercall_info.count > 0; entry.info.hypercall_info.count--) {
				result_val = PHV_PERFORM_HYPERCALL(entry.info.hypercall_info.callcode, input_page_physical, output_page_physical);

				status = ZwWriteFile(output,
					NULL,
					NULL,
					NULL,
					&io_status,
					&result_val,
					sizeof(result_val),
					0,
					NULL);

				if (!NT_SUCCESS(status)) {
					return status;
				}

				status = ZwWriteFile(output,
					NULL,
					NULL,
					NULL,
					&io_status,
					output_page_virtual,
					PAGE_SIZE,
					0,
					NULL);

				if (!NT_SUCCESS(status)) {
					return status;
				}

				RtlZeroMemory(output_page_virtual, PAGE_SIZE);
			}
		}
	}
}

NTSTATUS perform_file_log_exectime(HANDLE input, HANDLE output){
	NTSTATUS status = STATUS_SUCCESS;
	IO_STATUS_BLOCK io_status;
	struct campaign_entry entry;
	LARGE_INTEGER start_time, end_time;
	LONGLONG exec_time;

	while (1) {
		status = ZwReadFile(input,
			NULL,
			NULL,
			NULL,
			&io_status,
			&entry,
			sizeof(entry),
			0,
			NULL);

		if (!NT_SUCCESS(status)) {
			if (status == STATUS_END_OF_FILE && io_status.Information == 0) {
				return STATUS_SUCCESS;
			}
		}

		switch (entry.type) {
		case TYPE_WAIT:
			KeQuerySystemTimePrecise(&start_time);
			KeStallExecutionProcessor(entry.info.waiting_info.waiting_time);
			// KeDelayExecutionThread for longer waits?
			KeQuerySystemTimePrecise(&end_time);
			exec_time = end_time.QuadPart - start_time.QuadPart;

			status = ZwWriteFile(output,
				NULL,
				NULL,
				NULL,
				&io_status,
				&exec_time,
				sizeof(exec_time),
				0,
				NULL);

			if (!NT_SUCCESS(status)) {
				return status;
			}

			break;
		case TYPE_CALL:
			status = ZwReadFile(input,
				NULL,
				NULL,
				NULL,
				&io_status,
				input_page_virtual,
				entry.info.hypercall_info.input_page_size,
				0,
				NULL);

			if (!NT_SUCCESS(status)) {
				return status;
			}


			for (; entry.info.hypercall_info.count > 0; entry.info.hypercall_info.count--) {
				KeQuerySystemTimePrecise(&start_time);
				PHV_PERFORM_HYPERCALL(entry.info.hypercall_info.callcode, input_page_physical, output_page_physical);
				KeQuerySystemTimePrecise(&end_time);
				exec_time = end_time.QuadPart - start_time.QuadPart;

				status = ZwWriteFile(output,
					NULL,
					NULL,
					NULL,
					&io_status,
					&exec_time,
					sizeof(exec_time),
					0,
					NULL);

				if (!NT_SUCCESS(status)) {
					return status;
				}
			}
		}
	}
}

NTSTATUS perform_file_log_exectime_result(HANDLE input, HANDLE output){
	NTSTATUS status = STATUS_SUCCESS;
	IO_STATUS_BLOCK io_status;
	struct campaign_entry entry;
	LARGE_INTEGER start_time, end_time;
	LONGLONG exec_time;
	UINT64 result_val;

	while (1) {
		status = ZwReadFile(input,
			NULL,
			NULL,
			NULL,
			&io_status,
			&entry,
			sizeof(entry),
			0,
			NULL);

		if (!NT_SUCCESS(status)) {
			if (status == STATUS_END_OF_FILE && io_status.Information == 0) {
				return STATUS_SUCCESS;
			}
		}

		switch (entry.type) {
		case TYPE_WAIT:
			KeQuerySystemTimePrecise(&start_time);
			KeStallExecutionProcessor(entry.info.waiting_info.waiting_time);
			// KeDelayExecutionThread for longer waits?
			KeQuerySystemTimePrecise(&end_time);
			exec_time = end_time.QuadPart - start_time.QuadPart;

			status = ZwWriteFile(output,
				NULL,
				NULL,
				NULL,
				&io_status,
				&exec_time,
				sizeof(exec_time),
				0,
				NULL);

			if (!NT_SUCCESS(status)) {
				return status;
			}

			break;
		case TYPE_CALL:
			status = ZwReadFile(input,
				NULL,
				NULL,
				NULL,
				&io_status,
				input_page_virtual,
				entry.info.hypercall_info.input_page_size,
				0,
				NULL);

			if (!NT_SUCCESS(status)) {
				return status;
			}


			for (; entry.info.hypercall_info.count > 0; entry.info.hypercall_info.count--) {
				KeQuerySystemTimePrecise(&start_time);
				result_val = PHV_PERFORM_HYPERCALL(entry.info.hypercall_info.callcode, input_page_physical, output_page_physical);
				KeQuerySystemTimePrecise(&end_time);
				exec_time = end_time.QuadPart - start_time.QuadPart;

				status = ZwWriteFile(output,
					NULL,
					NULL,
					NULL,
					&io_status,
					&exec_time,
					sizeof(exec_time),
					0,
					NULL);

				if (!NT_SUCCESS(status)) {
					return status;
				}

				status = ZwWriteFile(output,
					NULL,
					NULL,
					NULL,
					&io_status,
					&result_val,
					sizeof(result_val),
					0,
					NULL);

				if (!NT_SUCCESS(status)) {
					return status;
				}
			}
		}
	}
}

NTSTATUS perform_file_log_exectime_output(HANDLE input, HANDLE output){
	NTSTATUS status = STATUS_SUCCESS;
	IO_STATUS_BLOCK io_status;
	struct campaign_entry entry;
	LARGE_INTEGER start_time, end_time;
	LONGLONG exec_time;

	while (1) {
		status = ZwReadFile(input,
			NULL,
			NULL,
			NULL,
			&io_status,
			&entry,
			sizeof(entry),
			0,
			NULL);

		if (!NT_SUCCESS(status)) {
			if (status == STATUS_END_OF_FILE && io_status.Information == 0) {
				return STATUS_SUCCESS;
			}
		}

		switch (entry.type) {
		case TYPE_WAIT:
			KeQuerySystemTimePrecise(&start_time);
			KeStallExecutionProcessor(entry.info.waiting_info.waiting_time);
			// KeDelayExecutionThread for longer waits?
			KeQuerySystemTimePrecise(&end_time);
			exec_time = end_time.QuadPart - start_time.QuadPart;

			status = ZwWriteFile(output,
				NULL,
				NULL,
				NULL,
				&io_status,
				&exec_time,
				sizeof(exec_time),
				0,
				NULL);

			if (!NT_SUCCESS(status)) {
				return status;
			}

			break;
		case TYPE_CALL:
			status = ZwReadFile(input,
				NULL,
				NULL,
				NULL,
				&io_status,
				input_page_virtual,
				entry.info.hypercall_info.input_page_size,
				0,
				NULL);

			if (!NT_SUCCESS(status)) {
				return status;
			}


			for (; entry.info.hypercall_info.count > 0; entry.info.hypercall_info.count--) {
				KeQuerySystemTimePrecise(&start_time);
				PHV_PERFORM_HYPERCALL(entry.info.hypercall_info.callcode, input_page_physical, output_page_physical);
				KeQuerySystemTimePrecise(&end_time);
				exec_time = end_time.QuadPart - start_time.QuadPart;

				status = ZwWriteFile(output,
					NULL,
					NULL,
					NULL,
					&io_status,
					&exec_time,
					sizeof(exec_time),
					0,
					NULL);

				if (!NT_SUCCESS(status)) {
					return status;
				}

				status = ZwWriteFile(output,
					NULL,
					NULL,
					NULL,
					&io_status,
					output_page_virtual,
					PAGE_SIZE,
					0,
					NULL);

				if (!NT_SUCCESS(status)) {
					return status;
				}

				RtlZeroMemory(output_page_virtual, PAGE_SIZE);
			}
		}
	}
}

NTSTATUS perform_file_log_exectime_result_output(HANDLE input, HANDLE output){
	NTSTATUS status = STATUS_SUCCESS;
	IO_STATUS_BLOCK io_status;
	struct campaign_entry entry;
	LARGE_INTEGER start_time, end_time;
	LONGLONG exec_time;
	UINT64 result_val;

	while (1) {
		status = ZwReadFile(input,
			NULL,
			NULL,
			NULL,
			&io_status,
			&entry,
			sizeof(entry),
			0,
			NULL);

		if (!NT_SUCCESS(status)) {
			if (status == STATUS_END_OF_FILE && io_status.Information == 0) {
				return STATUS_SUCCESS;
			}
		}

		switch (entry.type) {
		case TYPE_WAIT:
			KeQuerySystemTimePrecise(&start_time);
			KeStallExecutionProcessor(entry.info.waiting_info.waiting_time);
			// KeDelayExecutionThread for longer waits?
			KeQuerySystemTimePrecise(&end_time);
			exec_time = end_time.QuadPart - start_time.QuadPart;

			status = ZwWriteFile(output,
				NULL,
				NULL,
				NULL,
				&io_status,
				&exec_time,
				sizeof(exec_time),
				0,
				NULL);

			if (!NT_SUCCESS(status)) {
				return status;
			}

			break;
		case TYPE_CALL:
			status = ZwReadFile(input,
				NULL,
				NULL,
				NULL,
				&io_status,
				input_page_virtual,
				entry.info.hypercall_info.input_page_size,
				0,
				NULL);

			if (!NT_SUCCESS(status)) {
				return status;
			}


			for (; entry.info.hypercall_info.count > 0; entry.info.hypercall_info.count--) {
				KeQuerySystemTimePrecise(&start_time);
				result_val = PHV_PERFORM_HYPERCALL(entry.info.hypercall_info.callcode, input_page_physical, output_page_physical);
				KeQuerySystemTimePrecise(&end_time);
				exec_time = end_time.QuadPart - start_time.QuadPart;

				status = ZwWriteFile(output,
					NULL,
					NULL,
					NULL,
					&io_status,
					&exec_time,
					sizeof(exec_time),
					0,
					NULL);

				if (!NT_SUCCESS(status)) {
					return status;
				}

				status = ZwWriteFile(output,
					NULL,
					NULL,
					NULL,
					&io_status,
					&result_val,
					sizeof(result_val),
					0,
					NULL);

				if (!NT_SUCCESS(status)) {
					return status;
				}

				status = ZwWriteFile(output,
					NULL,
					NULL,
					NULL,
					&io_status,
					output_page_virtual,
					PAGE_SIZE,
					0,
					NULL);

				if (!NT_SUCCESS(status)) {
					return status;
				}

				RtlZeroMemory(output_page_virtual, PAGE_SIZE);
			}
		}
	}
}

NTSTATUS perform_file_log_timestamps(HANDLE input, HANDLE output){
	NTSTATUS status = STATUS_SUCCESS;
	IO_STATUS_BLOCK io_status;
	struct campaign_entry entry;
	LARGE_INTEGER start_time, end_time;

	while (1) {
		status = ZwReadFile(input,
			NULL,
			NULL,
			NULL,
			&io_status,
			&entry,
			sizeof(entry),
			0,
			NULL);

		if (!NT_SUCCESS(status)) {
			if (status == STATUS_END_OF_FILE && io_status.Information == 0) {
				return STATUS_SUCCESS;
			}
		}

		switch (entry.type) {
		case TYPE_WAIT:
			KeQuerySystemTimePrecise(&start_time);
			KeStallExecutionProcessor(entry.info.waiting_info.waiting_time);
			// KeDelayExecutionThread for longer waits?
			KeQuerySystemTimePrecise(&end_time);

			status = ZwWriteFile(output,
				NULL,
				NULL,
				NULL,
				&io_status,
				&start_time,
				sizeof(start_time),
				0,
				NULL);

			if (!NT_SUCCESS(status)) {
				return status;
			}

			status = ZwWriteFile(output,
				NULL,
				NULL,
				NULL,
				&io_status,
				&end_time,
				sizeof(end_time),
				0,
				NULL);

			if (!NT_SUCCESS(status)) {
				return status;
			}

			break;
		case TYPE_CALL:
			status = ZwReadFile(input,
				NULL,
				NULL,
				NULL,
				&io_status,
				input_page_virtual,
				entry.info.hypercall_info.input_page_size,
				0,
				NULL);

			if (!NT_SUCCESS(status)) {
				return status;
			}


			for (; entry.info.hypercall_info.count > 0; entry.info.hypercall_info.count--) {
				KeQuerySystemTimePrecise(&start_time);
				PHV_PERFORM_HYPERCALL(entry.info.hypercall_info.callcode, input_page_physical, output_page_physical);
				KeQuerySystemTimePrecise(&end_time);

				status = ZwWriteFile(output,
					NULL,
					NULL,
					NULL,
					&io_status,
					&start_time,
					sizeof(start_time),
					0,
					NULL);

				if (!NT_SUCCESS(status)) {
					return status;
				}

				status = ZwWriteFile(output,
					NULL,
					NULL,
					NULL,
					&io_status,
					&end_time,
					sizeof(end_time),
					0,
					NULL);

				if (!NT_SUCCESS(status)) {
					return status;
				}
			}
		}
	}
}

NTSTATUS perform_file_log_timestamps_result(HANDLE input, HANDLE output){
	NTSTATUS status = STATUS_SUCCESS;
	IO_STATUS_BLOCK io_status;
	struct campaign_entry entry;
	LARGE_INTEGER start_time, end_time;
	UINT64 result_val;

	while (1) {
		status = ZwReadFile(input,
			NULL,
			NULL,
			NULL,
			&io_status,
			&entry,
			sizeof(entry),
			0,
			NULL);

		if (!NT_SUCCESS(status)) {
			if (status == STATUS_END_OF_FILE && io_status.Information == 0) {
				return STATUS_SUCCESS;
			}
		}

		switch (entry.type) {
		case TYPE_WAIT:
			KeQuerySystemTimePrecise(&start_time);
			KeStallExecutionProcessor(entry.info.waiting_info.waiting_time);
			// KeDelayExecutionThread for longer waits?
			KeQuerySystemTimePrecise(&end_time);

			status = ZwWriteFile(output,
				NULL,
				NULL,
				NULL,
				&io_status,
				&start_time,
				sizeof(start_time),
				0,
				NULL);

			if (!NT_SUCCESS(status)) {
				return status;
			}

			status = ZwWriteFile(output,
				NULL,
				NULL,
				NULL,
				&io_status,
				&end_time,
				sizeof(end_time),
				0,
				NULL);

			if (!NT_SUCCESS(status)) {
				return status;
			}

			break;
		case TYPE_CALL:
			status = ZwReadFile(input,
				NULL,
				NULL,
				NULL,
				&io_status,
				input_page_virtual,
				entry.info.hypercall_info.input_page_size,
				0,
				NULL);

			if (!NT_SUCCESS(status)) {
				return status;
			}


			for (; entry.info.hypercall_info.count > 0; entry.info.hypercall_info.count--) {
				KeQuerySystemTimePrecise(&start_time);
				result_val = PHV_PERFORM_HYPERCALL(entry.info.hypercall_info.callcode, input_page_physical, output_page_physical);
				KeQuerySystemTimePrecise(&end_time);

				status = ZwWriteFile(output,
					NULL,
					NULL,
					NULL,
					&io_status,
					&start_time,
					sizeof(start_time),
					0,
					NULL);

				if (!NT_SUCCESS(status)) {
					return status;
				}

				status = ZwWriteFile(output,
					NULL,
					NULL,
					NULL,
					&io_status,
					&end_time,
					sizeof(end_time),
					0,
					NULL);

				if (!NT_SUCCESS(status)) {
					return status;
				}

				status = ZwWriteFile(output,
					NULL,
					NULL,
					NULL,
					&io_status,
					&result_val,
					sizeof(result_val),
					0,
					NULL);

				if (!NT_SUCCESS(status)) {
					return status;
				}
			}
		}
	}
}

NTSTATUS perform_file_log_timestamps_output(HANDLE input, HANDLE output) {
	NTSTATUS status = STATUS_SUCCESS;
	IO_STATUS_BLOCK io_status;
	struct campaign_entry entry;
	LARGE_INTEGER start_time, end_time;

	while (1) {
		status = ZwReadFile(input,
			NULL,
			NULL,
			NULL,
			&io_status,
			&entry,
			sizeof(entry),
			0,
			NULL);

		if (!NT_SUCCESS(status)) {
			if (status == STATUS_END_OF_FILE && io_status.Information == 0) {
				return STATUS_SUCCESS;
			}
		}

		switch (entry.type) {
		case TYPE_WAIT:
			KeQuerySystemTimePrecise(&start_time);
			KeStallExecutionProcessor(entry.info.waiting_info.waiting_time);
			// KeDelayExecutionThread for longer waits?
			KeQuerySystemTimePrecise(&end_time);

			status = ZwWriteFile(output,
				NULL,
				NULL,
				NULL,
				&io_status,
				&start_time,
				sizeof(start_time),
				0,
				NULL);

			if (!NT_SUCCESS(status)) {
				return status;
			}

			status = ZwWriteFile(output,
				NULL,
				NULL,
				NULL,
				&io_status,
				&end_time,
				sizeof(end_time),
				0,
				NULL);

			if (!NT_SUCCESS(status)) {
				return status;
			}

			break;
		case TYPE_CALL:
			status = ZwReadFile(input,
				NULL,
				NULL,
				NULL,
				&io_status,
				input_page_virtual,
				entry.info.hypercall_info.input_page_size,
				0,
				NULL);

			if (!NT_SUCCESS(status)) {
				return status;
			}


			for (; entry.info.hypercall_info.count > 0; entry.info.hypercall_info.count--) {
				KeQuerySystemTimePrecise(&start_time);
				PHV_PERFORM_HYPERCALL(entry.info.hypercall_info.callcode, input_page_physical, output_page_physical);
				KeQuerySystemTimePrecise(&end_time);

				status = ZwWriteFile(output,
					NULL,
					NULL,
					NULL,
					&io_status,
					&start_time,
					sizeof(start_time),
					0,
					NULL);

				if (!NT_SUCCESS(status)) {
					return status;
				}

				status = ZwWriteFile(output,
					NULL,
					NULL,
					NULL,
					&io_status,
					&end_time,
					sizeof(end_time),
					0,
					NULL);

				if (!NT_SUCCESS(status)) {
					return status;
				}

				status = ZwWriteFile(output,
					NULL,
					NULL,
					NULL,
					&io_status,
					output_page_virtual,
					PAGE_SIZE,
					0,
					NULL);

				if (!NT_SUCCESS(status)) {
					return status;
				}

				RtlZeroMemory(output_page_virtual, PAGE_SIZE);
			}
		}
	}
}

NTSTATUS perform_file_log_timestamps_result_output(HANDLE input, HANDLE output){
	NTSTATUS status = STATUS_SUCCESS;
	IO_STATUS_BLOCK io_status;
	struct campaign_entry entry;
	LARGE_INTEGER start_time, end_time;
	UINT64 result_val;

	while (1) {
		status = ZwReadFile(input,
			NULL,
			NULL,
			NULL,
			&io_status,
			&entry,
			sizeof(entry),
			0,
			NULL);

		if (!NT_SUCCESS(status)) {
			if (status == STATUS_END_OF_FILE && io_status.Information == 0) {
				return STATUS_SUCCESS;
			}
		}

		switch (entry.type) {
		case TYPE_WAIT:
			KeQuerySystemTimePrecise(&start_time);
			KeStallExecutionProcessor(entry.info.waiting_info.waiting_time);
			// KeDelayExecutionThread for longer waits?
			KeQuerySystemTimePrecise(&end_time);

			status = ZwWriteFile(output,
				NULL,
				NULL,
				NULL,
				&io_status,
				&start_time,
				sizeof(start_time),
				0,
				NULL);

			if (!NT_SUCCESS(status)) {
				return status;
			}

			status = ZwWriteFile(output,
				NULL,
				NULL,
				NULL,
				&io_status,
				&end_time,
				sizeof(end_time),
				0,
				NULL);

			if (!NT_SUCCESS(status)) {
				return status;
			}

			break;
		case TYPE_CALL:
			status = ZwReadFile(input,
				NULL,
				NULL,
				NULL,
				&io_status,
				input_page_virtual,
				entry.info.hypercall_info.input_page_size,
				0,
				NULL);

			if (!NT_SUCCESS(status)) {
				return status;
			}


			for (; entry.info.hypercall_info.count > 0; entry.info.hypercall_info.count--) {
				KeQuerySystemTimePrecise(&start_time);
				result_val = PHV_PERFORM_HYPERCALL(entry.info.hypercall_info.callcode, input_page_physical, output_page_physical);
				KeQuerySystemTimePrecise(&end_time);

				status = ZwWriteFile(output,
					NULL,
					NULL,
					NULL,
					&io_status,
					&start_time,
					sizeof(start_time),
					0,
					NULL);

				if (!NT_SUCCESS(status)) {
					return status;
				}

				status = ZwWriteFile(output,
					NULL,
					NULL,
					NULL,
					&io_status,
					&end_time,
					sizeof(end_time),
					0,
					NULL);

				if (!NT_SUCCESS(status)) {
					return status;
				}

				status = ZwWriteFile(output,
					NULL,
					NULL,
					NULL,
					&io_status,
					&result_val,
					sizeof(result_val),
					0,
					NULL);

				if (!NT_SUCCESS(status)) {
					return status;
				}

				status = ZwWriteFile(output,
					NULL,
					NULL,
					NULL,
					&io_status,
					output_page_virtual,
					PAGE_SIZE,
					0,
					NULL);

				if (!NT_SUCCESS(status)) {
					return status;
				}

				RtlZeroMemory(output_page_virtual, PAGE_SIZE);
			}
		}
	}
}