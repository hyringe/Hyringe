#include <ntddk.h>
#include <wdf.h>


#define NTDEVICE_NAME_STRING      L"\\Device\\NONPNP"
#define SYMBOLIC_NAME_STRING     L"\\DosDevices\\NONPNP"
#define OUTPUT_FILE_ENDING_STRING L".out"

#define TYPE_CALL	0xca
#define TYPE_WAIT	0x51


/*struct campaign_entry {
	UINT8 type;
	union {
		UINT32 waiting_time;
		struct {
			UINT16 callcode;
			UINT16 count;
		} hypercall_info;
	} info;
};*/


DRIVER_INITIALIZE DriverEntry;
//
// Don't use EVT_WDF_DRIVER_DEVICE_ADD for NonPnpDeviceAdd even though 
// the signature is same because this is not an event called by the 
// framework.
//
NTSTATUS
NonPnpDeviceAdd(
    IN WDFDRIVER Driver,
    IN PWDFDEVICE_INIT DeviceInit
    );
EVT_WDF_DRIVER_UNLOAD NonPnpEvtDriverUnload;
EVT_WDF_IO_QUEUE_IO_WRITE FileEvtIoWrite;



NTSTATUS perform_memory(HANDLE input, HANDLE output);
NTSTATUS perform_memory_log_result(HANDLE input, HANDLE output);
NTSTATUS perform_memory_log_output(HANDLE input, HANDLE output);
NTSTATUS perform_memory_log_result_output(HANDLE input, HANDLE output);
NTSTATUS perform_memory_log_exectime(HANDLE input, HANDLE output);
NTSTATUS perform_memory_log_exectime_result(HANDLE input, HANDLE output);
NTSTATUS perform_memory_log_exectime_output(HANDLE input, HANDLE output);
NTSTATUS perform_memory_log_exectime_result_output(HANDLE input, HANDLE output);
NTSTATUS perform_memory_log_timestamps(HANDLE input, HANDLE output);
NTSTATUS perform_memory_log_timestamps_result(HANDLE input, HANDLE output);
NTSTATUS perform_memory_log_timestamps_output(HANDLE input, HANDLE output);
NTSTATUS perform_memory_log_timestamps_result_output(HANDLE input, HANDLE output);
NTSTATUS perform_file(HANDLE input, HANDLE output);
NTSTATUS perform_file_log_result(HANDLE input, HANDLE output);
NTSTATUS perform_file_log_output(HANDLE input, HANDLE output);
NTSTATUS perform_file_log_result_output(HANDLE input, HANDLE output);
NTSTATUS perform_file_log_exectime(HANDLE input, HANDLE output);
NTSTATUS perform_file_log_exectime_result(HANDLE input, HANDLE output);
NTSTATUS perform_file_log_exectime_output(HANDLE input, HANDLE output);
NTSTATUS perform_file_log_exectime_result_output(HANDLE input, HANDLE output);
NTSTATUS perform_file_log_timestamps(HANDLE input, HANDLE output);
NTSTATUS perform_file_log_timestamps_result(HANDLE input, HANDLE output);
NTSTATUS perform_file_log_timestamps_output(HANDLE input, HANDLE output);
NTSTATUS perform_file_log_timestamps_result_output(HANDLE input, HANDLE output);