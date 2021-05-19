       
#include <DriverSpecs.h>
_Analysis_mode_(_Analysis_code_type_user_code_)  

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include "public.h"


BOOLEAN
ManageDriver(
    IN LPCTSTR  DriverName,
    IN LPCTSTR  ServiceName,
    IN USHORT   Function
    );

BOOLEAN
SetupDriverName(
    _Inout_updates_all_(BufferLength) PCHAR DriverLocation,
    _In_ ULONG BufferLength
    );

BOOLEAN
DoHypercallFile(
	HANDLE HDevice,
	LPWSTR campaignpath,
	UINT32 flags
);



VOID __cdecl
main(
    _In_ ULONG argc,
    _In_reads_(argc) PCHAR argv[]
    )
{
    HANDLE   hDevice;
    CHAR     driverLocation [MAX_PATH];
    BOOL     ok;
		
	//WCHAR buf[] = L"\\??\\C:\\Users\\vmtest\\Desktop\\nonpnp\\test.txt";
	//fputws(buf, stdout);
	if (argc == 1) {
		printf("Usage: nonpnp.exe <campaignfile> [--exectime --timestamps --memory --result --output]");
		return;
	}
	
	WIN32_FIND_DATA FindFileData;
	if (FindFirstFile(argv[1], &FindFileData) == INVALID_HANDLE_VALUE) {
		printf("File \"%s\" does not exist.\n", argv[1]);
		return;
	}

	CHAR abspath[MAX_PATH];
	CHAR fullpath[MAX_PATH];
	GetFullPathName(argv[1], MAX_PATH, abspath, 0);
	sprintf_s(fullpath, MAX_PATH, "\\??\\%s", abspath);

	WCHAR wideFullpath[MAX_PATH];
	mbstowcs_s(NULL, wideFullpath, strlen(fullpath) + 1, fullpath, 100);

	struct {
		UINT32 exectime : 1;
		UINT32 timestamps : 1;
		UINT32 memory : 1;
		UINT32 result : 1;
		UINT32 output : 1;
		UINT32 reserved : 27;
	} flags;

	memset(&flags, 0, sizeof(flags));

	UINT16 i;
	for (i = 2; i < argc; i++) {
		if (!strcmp(argv[i], "--exectime"))
			flags.exectime = 1;
		else if (!strcmp(argv[i], "--timestamps"))
			flags.timestamps = 1;
		else if (!strcmp(argv[i], "--memory"))
			flags.memory = 1;
		else if (!strcmp(argv[i], "--result"))
			flags.result = 1;
		else if (!strcmp(argv[i], "--output"))
			flags.output = 1;
		else {
			printf("Unknown parameter: %s\n", argv[i]);
			return;
		}
	}



	ok = SetupDriverName(driverLocation, MAX_PATH);
	if (!ok) {
		return;
	}

	ok = ManageDriver(DRIVER_NAME,
		driverLocation,
		DRIVER_FUNC_INSTALL);
	if (!ok) {
		printf("Unable to install driver. \n");

		ManageDriver(DRIVER_NAME,
			driverLocation,
			DRIVER_FUNC_REMOVE);
		return;
	}

	hDevice = CreateFile(DEVICE_NAME,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hDevice == INVALID_HANDLE_VALUE) {
		printf("Error: CreateFile Failed : %d\n", GetLastError());
		return;
	}


	DoHypercallFile(hDevice, wideFullpath, *(UINT32*)&flags);


    CloseHandle (hDevice);

    ManageDriver(DRIVER_NAME,
                 driverLocation,
                 DRIVER_FUNC_REMOVE);

    return;
}

BOOLEAN
DoHypercallFile(
	HANDLE HDevice,
	LPWSTR campaignpath,
	UINT32 flags
)
{

	PVOID buf = malloc(sizeof(UINT32) + wcslen(campaignpath) * sizeof(WCHAR));
	*(PUINT32)buf = flags;
	memcpy((PUINT32)buf + 1, campaignpath, wcslen(campaignpath) * sizeof(WCHAR));
	ULONG bytesWritten;
	
	if (!WriteFile(HDevice,
		buf,
		sizeof(UINT32) + (DWORD)wcslen(campaignpath) * sizeof(WCHAR),
		&bytesWritten,
		NULL)) {

		printf("ReadFile failed with error 0x%x\n", GetLastError());

		return FALSE;
	}

	return TRUE;
}
