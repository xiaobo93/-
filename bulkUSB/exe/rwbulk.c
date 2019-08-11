/*++

Copyright (c) 1997-1998  Microsoft Corporation

Module Name:

    RWBulk.c

Abstract:

    Console test app for BulkUsb.sys driver

Environment:

    user mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1997-1998 Microsoft Corporation.  All Rights Reserved.


Revision History:

        11/17/97: created

--*/

#include <windows.h>

#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "devioctl.h"

#include <setupapi.h>
#include <basetyps.h>
//#include "BulkUsr.h"
#include"../sys/bulkusr.h"

#include "usbdi.h"

#pragma comment(lib,"Setupapi.lib")

#define NOISY(_x_) printf _x_ ;

char inPipe[32] = "PIPE00";     // pipe name for bulk input pipe on our test board
char outPipe[32] = "PIPE01";    // pipe name for bulk output pipe on our test board 在我们的测试板上批量输出管道的名称
char completeDeviceName[256] = "";  //generated from the GUID registered by the driver itself 由驱动程序本身注册的GUID生成

BOOL fDumpUsbConfig = FALSE;    // flags set in response to console command line switches为响应控制台命令行开关而设置的标志
BOOL fDumpReadData = FALSE;
BOOL fRead = FALSE;
BOOL fWrite = FALSE;

int gDebugLevel = 1;      // higher == more verbose, default is 1, 0 turns off all

ULONG IterationCount = 1; //count of iterations of the test we are to perform
int WriteLen = 0;         // #bytes to write
int ReadLen = 0;          // #bytes to read

// functions


HANDLE
OpenOneDevice (
    IN       HDEVINFO                    HardwareDeviceInfo,
    IN       PSP_DEVICE_INTERFACE_DATA   DeviceInfoData,
        IN               char *devName
    )
/*++
Routine Description:

    Given the HardwareDeviceInfo, representing a handle to the plug and
    play information, and deviceInfoData, representing a specific usb device,
    open that device and fill in all the relevant information in the given
    USB_DEVICE_DESCRIPTOR structure.
	给定硬件设备信息，表示插头和的句柄
	播放信息和deviceInfoData，代表一个特定的usb设备，
	打开该设备，并填写所有相关信息
	USB_DEVICE_DESCRIPTOR结构。

Arguments:

    HardwareDeviceInfo:  handle to info obtained from Pnp mgr via SetupDiGetClassDevs()
    DeviceInfoData:      ptr to info obtained via SetupDiEnumDeviceInterfaces()

Return Value:

    return HANDLE if the open and initialization was successfull,
        else INVLAID_HANDLE_VALUE.

--*/
{
    PSP_DEVICE_INTERFACE_DETAIL_DATA     functionClassDeviceData = NULL;
    ULONG                                predictedLength = 0;
    ULONG                                requiredLength = 0;
    HANDLE                                                           hOut = INVALID_HANDLE_VALUE;

    //
    // allocate a function class device data structure to receive the
    // goods about this particular device.
    //
	SetupDiGetDeviceInterfaceDetail(
            HardwareDeviceInfo,
            DeviceInfoData,
            NULL, // probing so no output buffer yet
            0, // probing so output buffer length of zero
            &requiredLength,
            NULL); // not interested in the specific dev-node


    predictedLength = requiredLength;
    // sizeof (SP_FNCLASS_DEVICE_DATA) + 512;

    functionClassDeviceData = malloc (predictedLength);
    functionClassDeviceData->cbSize = sizeof (SP_DEVICE_INTERFACE_DETAIL_DATA);

    //
    // Retrieve the information from Plug and Play.
    //
    if (!SetupDiGetDeviceInterfaceDetail(
               HardwareDeviceInfo,
               DeviceInfoData,
               functionClassDeviceData,
               predictedLength,
               &requiredLength,
               NULL)) {
                free( functionClassDeviceData );
        return INVALID_HANDLE_VALUE;
    }

	strcpy(devName, functionClassDeviceData->DevicePath);
	printf("Attempting to open %s\n", devName);

    hOut = CreateFile (
                  functionClassDeviceData->DevicePath,
                  GENERIC_READ | GENERIC_WRITE,
                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                  NULL, // no SECURITY_ATTRIBUTES structure
                  OPEN_EXISTING, // No special create flags
                  0, // No special attributes
                  NULL); // No template file

    if (INVALID_HANDLE_VALUE == hOut) {
                printf( "FAILED to open %s\n", devName );
    }
        free( functionClassDeviceData );
        return hOut;
}


HANDLE
OpenUsbDevice( LPGUID  pGuid, char *outNameBuf)
/*++
Routine Description:

   Do the required PnP things in order to find
   the next available proper device in the system at this time.
   为了找到所需的PnP
   此时系统中可用的下一个合适的设备。

Arguments:

    pGuid:      ptr to GUID registered by the driver itself
    outNameBuf: the generated name for this device

Return Value:

    return HANDLE if the open and initialization was successful,
        else INVLAID_HANDLE_VALUE.
--*/
{
   ULONG NumberDevices;
   HANDLE hOut = INVALID_HANDLE_VALUE;
   HDEVINFO                 hardwareDeviceInfo;
   SP_DEVICE_INTERFACE_DATA deviceInfoData;
   ULONG                    i;
   BOOLEAN                  done;
   PUSB_DEVICE_DESCRIPTOR   usbDeviceInst;
   PUSB_DEVICE_DESCRIPTOR       *UsbDevices = &usbDeviceInst;

   *UsbDevices = NULL;
   NumberDevices = 0;

   //
   // Open a handle to the plug and play dev node.  打开即插即用开发节点的句柄
   // SetupDiGetClassDevs() returns a device information set that contains info on all
   // installed devices of a specified class.
   //
   hardwareDeviceInfo = SetupDiGetClassDevs(
                           pGuid,
                           NULL, // Define no enumerator (global)
                           NULL, // Define no
                           (DIGCF_PRESENT | // Only Devices present
                            DIGCF_DEVICEINTERFACE)); // Function class devices.

   //
   // Take a wild guess at the number of devices we have;
   // Be prepared to realloc and retry if there are more than we guessed
   //
   NumberDevices = 4;
   done = FALSE;
   deviceInfoData.cbSize = sizeof (SP_DEVICE_INTERFACE_DATA);

   i=0;
   while (!done) {
      NumberDevices *= 2;

      if (*UsbDevices) {
         *UsbDevices = realloc (*UsbDevices, (NumberDevices * sizeof (USB_DEVICE_DESCRIPTOR)));
      } else {
         *UsbDevices = calloc (NumberDevices, sizeof (USB_DEVICE_DESCRIPTOR));
      }

      if (NULL == *UsbDevices) {

         // SetupDiDestroyDeviceInfoList destroys a device information set
         // and frees all associated memory.

         SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
         return INVALID_HANDLE_VALUE;
      }

      usbDeviceInst = *UsbDevices + i;
	  int mm = 0;
      for (; i < NumberDevices; i++) {

         // SetupDiEnumDeviceInterfaces() returns information about device interfaces
         // exposed by one or more devices. Each call returns information about one interface;
         // the routine can be called repeatedly to get information about several interfaces
         // exposed by one or more devices.
		  /*
			// SetupDiEnumDeviceInterfaces()返回关于设备接口的信息
			//由一个或多个设备暴露。每个调用返回一个接口的信息;
			//可以反复调用这个例程来获得关于几个接口的信息
			//由一个或多个设备暴露。
		  */
         if (SetupDiEnumDeviceInterfaces (hardwareDeviceInfo,
                                         0, // We don't care about specific PDOs
                                         pGuid,//指向GUID的指针，该GUID指定所请求接口的设备接口类。
                                         i,
                                         &deviceInfoData)) {

            hOut = OpenOneDevice (hardwareDeviceInfo, &deviceInfoData, outNameBuf);
            if ( hOut != INVALID_HANDLE_VALUE ) {
               done = TRUE;
               break;
            }

         } else {
            if (ERROR_NO_MORE_ITEMS == GetLastError()) {
               done = TRUE;
               break;
            }
         }
		 mm = GetLastError();
      }
   }

   NumberDevices = i;

   // SetupDiDestroyDeviceInfoList() destroys a device information set
   // and frees all associated memory.

   SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
   free ( *UsbDevices );
   return hOut;
}




BOOL
GetUsbDeviceFileName( LPGUID  pGuid, char *outNameBuf)
/*++
Routine Description:

    Given a ptr to a driver-registered GUID, give us a string with the device name
    that can be used in a CreateFile() call.
    Actually briefly opens and closes the device and sets outBuf if successfull;
    returns FALSE if not

	给驱动程序注册的GUID一个ptr，给我们一个带有设备名称的字符串
	可以在CreateFile()调用中使用。
	实际上简单地打开和关闭设备，如果成功，则设置为outBuf;
	如果没有返回FALSE

Arguments:

    pGuid:      ptr to GUID registered by the driver itself
    outNameBuf: the generated zero-terminated name for this device

Return Value:

    TRUE on success else FALSE

--*/
{
        HANDLE hDev = OpenUsbDevice( pGuid, outNameBuf );
        if ( hDev != INVALID_HANDLE_VALUE )
        {
                CloseHandle( hDev );
                return TRUE;
        }
        return FALSE;

}

HANDLE
open_dev()
/*++
Routine Description:

    Called by dumpUsbConfig() to open an instance of our device
	由dumpUsbConfig()调用，以打开设备的实例
Arguments:

    None

Return Value:

    Device handle on success else NULL

--*/
{

        HANDLE hDEV = OpenUsbDevice( (LPGUID)&GUID_CLASS_I82930_BULK, completeDeviceName);


        if (hDEV == INVALID_HANDLE_VALUE) {
                printf("Failed to open (%s) = %d", completeDeviceName, GetLastError());
        } else {
                printf("DeviceName = (%s)\n", completeDeviceName);
    }           

        return hDEV;
}


HANDLE
open_file( char *filename)
/*++
Routine Description:

    Called by main() to open an instance of our device after obtaining its name

	打开设备实例  在获取设备名称之后

Arguments:

    None

Return Value:

    Device handle on success else NULL

--*/
{

	int success = 1;
	HANDLE h;

	if (!GetUsbDeviceFileName(
		(LPGUID)&GUID_CLASS_I82930_BULK,
		completeDeviceName))
	{
		int k = GetLastError();
		NOISY(("Failed to GetUsbDeviceFileName\n", GetLastError()));
		return  INVALID_HANDLE_VALUE;
	}

    strcat (completeDeviceName,
                        "\\"
                        );                      

    strcat (completeDeviceName,
                        filename
                        );                                      

   printf("completeDeviceName = (%s)\n", completeDeviceName);

   h = CreateFile(completeDeviceName,
                GENERIC_WRITE | GENERIC_READ,
                FILE_SHARE_WRITE | FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                0,
                NULL);
   int kkk = GetLastError();
        if (h == INVALID_HANDLE_VALUE) {
                NOISY(("Failed to open (%s) = %d", completeDeviceName, GetLastError()));
                success = 0;
        } else {
                        NOISY(("Opened successfully.\n"));
    }           

        return h;
}

void
usage()
/*++
Routine Description:

    Called by main() to dump usage info to the console when
    the app is called with no parms or with an invalid parm

Arguments:

    None

Return Value:

    None

--*/
{
    static int i=1;

    if (i) {
        printf("Usage for Read/Write test:\n");
        printf("-r [n] where n is number of bytes to read\n");
        printf("-w [n] where n is number of bytes to write\n");
        printf("-c [n] where n is number of iterations (default = 1)\n");
        printf("-i [s] where s is the input pipe\n");
        printf("-o [s] where s is the output pipe\n");
        printf("-v verbose -- dumps read data\n");

        printf("\nUsage for USB and Endpoint info:\n");
        printf("-u to dump USB configuration and pipe info \n");
        i = 0;
    }
}


void
parse(
    int argc,
    char *argv[] )
/*++
Routine Description:

    Called by main() to parse command line parms

Arguments:

    argc and argv that was passed to main()

Return Value:

    Sets global flags as per user function request

--*/
{
    int i;

        if ( argc < 2 ) // give usage if invoked with no parms
                usage();

    for (i=0; i<argc; i++) {
        if (argv[i][0] == '-' ||
            argv[i][0] == '/') {
            switch(argv[i][1]) {
            case 'r':
            case 'R':
                ReadLen = atoi(&argv[i+1][0]);
                                fRead = TRUE;
                i++;
                break;
            case 'w':
            case 'W':
                WriteLen = atoi(&argv[i+1][0]);
                                fWrite = TRUE;
                i++;
                break;
            case 'c':
            case 'C':
                IterationCount = atoi(&argv[i+1][0]);
                i++;
                break;
            case 'i':
            case 'I':
                strcpy(inPipe, &argv[i+1][0]);
                i++;
                break;
            case 'u':
            case 'U':
                fDumpUsbConfig = TRUE;
                                i++;
                break;
            case 'v':
            case 'V':
                fDumpReadData = TRUE;
                                i++;
                break;
                         case 'o':
             case 'O':
                strcpy(outPipe, &argv[i+1][0]);
                i++;
                break;
            default:
                usage();
            }
        }
    }
}

BOOL
compare_buffs(char *buff1, char *buff2, int length)
/*++
Routine Description:

    Called to verify read and write buffers match for loopback test

Arguments:

    buffers to compare and length

Return Value:

    TRUE if buffers match, else FALSE

--*/
{
    int ok = 1;

        if (memcmp(buff1, buff2, length )) {

                // Edi, and Esi point to the mismatching char and ecx indicates the
                // remaining length.
                ok = 0;
        }


    return ok;
}

#define NPERLN 8

void
dump(
   UCHAR *b,
   int len
)
/*++
Routine Description:

    Called to do formatted ascii dump to console of the io buffer

Arguments:

    buffer and length

Return Value:

    none

--*/
{
    ULONG i;
        ULONG longLen = (ULONG)len / sizeof( ULONG );
        PULONG pBuf = (PULONG) b;

        // dump an ordinal ULONG for each sizeof(ULONG)'th byte
    printf("\n****** BEGIN DUMP LEN decimal %d, 0x%x\n", len,len);
    for (i=0; i<longLen; i++) {
        printf("%04X ", *pBuf++);
        if (i % NPERLN == (NPERLN - 1)) {
            printf("\n");
        }
    }
    if (i % NPERLN != 0) {
        printf("\n");
    }
    printf("\n****** END DUMP LEN decimal %d, 0x%x\n", len,len);
}





// Begin, routines for USB configuration dump (Cmdline "rwbulk -u" )


char
*usbDescriptorTypeString(UCHAR bDescriptorType )
/*++
Routine Description:

    Called to get ascii string of USB descriptor

Arguments:

        PUSB_ENDPOINT_DESCRIPTOR->bDescriptorType or
        PUSB_DEVICE_DESCRIPTOR->bDescriptorType or
        PUSB_INTERFACE_DESCRIPTOR->bDescriptorType or
        PUSB_STRING_DESCRIPTOR->bDescriptorType or
        PUSB_POWER_DESCRIPTOR->bDescriptorType or
        PUSB_CONFIGURATION_DESCRIPTOR->bDescriptorType

Return Value:

    ptr to string

--*/{

        switch(bDescriptorType) {

        case USB_DEVICE_DESCRIPTOR_TYPE:
                return "USB_DEVICE_DESCRIPTOR_TYPE";

        case USB_CONFIGURATION_DESCRIPTOR_TYPE:
                return "USB_CONFIGURATION_DESCRIPTOR_TYPE";
                

        case USB_STRING_DESCRIPTOR_TYPE:
                return "USB_STRING_DESCRIPTOR_TYPE";
                

        case USB_INTERFACE_DESCRIPTOR_TYPE:
                return "USB_INTERFACE_DESCRIPTOR_TYPE";
                

        case USB_ENDPOINT_DESCRIPTOR_TYPE:
                return "USB_ENDPOINT_DESCRIPTOR_TYPE";
                

#ifdef USB_POWER_DESCRIPTOR_TYPE // this is the older definintion which is actually obsolete
    // workaround for temporary bug in 98ddk, older USB100.h file
        case USB_POWER_DESCRIPTOR_TYPE:
                return "USB_POWER_DESCRIPTOR_TYPE";
#endif
                
#ifdef USB_RESERVED_DESCRIPTOR_TYPE  // this is the current version of USB100.h as in NT5DDK

        case USB_RESERVED_DESCRIPTOR_TYPE:
                return "USB_RESERVED_DESCRIPTOR_TYPE";

        case USB_CONFIG_POWER_DESCRIPTOR_TYPE:
                return "USB_CONFIG_POWER_DESCRIPTOR_TYPE";

        case USB_INTERFACE_POWER_DESCRIPTOR_TYPE:
                return "USB_INTERFACE_POWER_DESCRIPTOR_TYPE";
#endif // for current nt5ddk version of USB100.h

        default:
                return "??? UNKNOWN!!"; 
        }
}


char
*usbEndPointTypeString(UCHAR bmAttributes)
/*++
Routine Description:

    Called to get ascii string of endpt descriptor type

Arguments:

        PUSB_ENDPOINT_DESCRIPTOR->bmAttributes

Return Value:

    ptr to string

--*/
{
        UINT typ = bmAttributes & USB_ENDPOINT_TYPE_MASK;


        switch( typ) {
        case USB_ENDPOINT_TYPE_INTERRUPT:
                return "USB_ENDPOINT_TYPE_INTERRUPT";

        case USB_ENDPOINT_TYPE_BULK:
                return "USB_ENDPOINT_TYPE_BULK";        

        case USB_ENDPOINT_TYPE_ISOCHRONOUS:
                return "USB_ENDPOINT_TYPE_ISOCHRONOUS"; 
                
        case USB_ENDPOINT_TYPE_CONTROL:
                return "USB_ENDPOINT_TYPE_CONTROL";     
                
        default:
                return "??? UNKNOWN!!"; 
        }
}


char
*usbConfigAttributesString(UCHAR bmAttributes)
/*++
Routine Description:

    Called to get ascii string of USB_CONFIGURATION_DESCRIPTOR attributes

Arguments:

        PUSB_CONFIGURATION_DESCRIPTOR->bmAttributes

Return Value:

    ptr to string

--*/
{
        UINT typ = bmAttributes & USB_CONFIG_POWERED_MASK;


        switch( typ) {

        case USB_CONFIG_BUS_POWERED:
                return "USB_CONFIG_BUS_POWERED";

        case USB_CONFIG_SELF_POWERED:
                return "USB_CONFIG_SELF_POWERED";
                
        case USB_CONFIG_REMOTE_WAKEUP:
                return "USB_CONFIG_REMOTE_WAKEUP";

                
        default:
                return "??? UNKNOWN!!"; 
        }
}


void
print_USB_CONFIGURATION_DESCRIPTOR(PUSB_CONFIGURATION_DESCRIPTOR cd)
/*++
Routine Description:

    Called to do formatted ascii dump to console of a USB config descriptor

Arguments:

    ptr to USB configuration descriptor

Return Value:

    none

--*/
{
    printf("\n===================\nUSB_CONFIGURATION_DESCRIPTOR\n");

    printf(
    "bLength = 0x%x, decimal %d\n", cd->bLength, cd->bLength
    );

    printf(
    "bDescriptorType = 0x%x ( %s )\n", cd->bDescriptorType, usbDescriptorTypeString( cd->bDescriptorType )
    );

    printf(
    "wTotalLength = 0x%x, decimal %d\n", cd->wTotalLength, cd->wTotalLength
    );

    printf(
    "bNumInterfaces = 0x%x, decimal %d\n", cd->bNumInterfaces, cd->bNumInterfaces
    );

    printf(
    "bConfigurationValue = 0x%x, decimal %d\n", cd->bConfigurationValue, cd->bConfigurationValue
    );

    printf(
    "iConfiguration = 0x%x, decimal %d\n", cd->iConfiguration, cd->iConfiguration
    );

    printf(
    "bmAttributes = 0x%x ( %s )\n", cd->bmAttributes, usbConfigAttributesString( cd->bmAttributes )
    );

    printf(
    "MaxPower = 0x%x, decimal %d\n", cd->MaxPower, cd->MaxPower
    );
}


void
print_USB_INTERFACE_DESCRIPTOR(PUSB_INTERFACE_DESCRIPTOR id, UINT ix)
/*++
Routine Description:

    Called to do formatted ascii dump to console of a USB interface descriptor

Arguments:

    ptr to USB interface descriptor

Return Value:

    none

--*/
{
    printf("\n-----------------------------\nUSB_INTERFACE_DESCRIPTOR #%d\n", ix);


    printf(
    "bLength = 0x%x\n", id->bLength
    );


    printf(
    "bDescriptorType = 0x%x ( %s )\n", id->bDescriptorType, usbDescriptorTypeString( id->bDescriptorType )
    );


    printf(
    "bInterfaceNumber = 0x%x\n", id->bInterfaceNumber
    );
    printf(
    "bAlternateSetting = 0x%x\n", id->bAlternateSetting
    );
    printf(
    "bNumEndpoints = 0x%x\n", id->bNumEndpoints
    );
    printf(
    "bInterfaceClass = 0x%x\n", id->bInterfaceClass
    );
    printf(
    "bInterfaceSubClass = 0x%x\n", id->bInterfaceSubClass
    );
    printf(
    "bInterfaceProtocol = 0x%x\n", id->bInterfaceProtocol
    );
    printf(
    "bInterface = 0x%x\n", id->iInterface
    );
}


void
print_USB_ENDPOINT_DESCRIPTOR(PUSB_ENDPOINT_DESCRIPTOR ed, int i)
/*++
Routine Description:

    Called to do formatted ascii dump to console of a USB endpoint descriptor

Arguments:

    ptr to USB endpoint descriptor,
        index of this endpt in interface desc

Return Value:

    none

--*/
{
    printf(
        "------------------------------\nUSB_ENDPOINT_DESCRIPTOR for Pipe%02d\n", i
        );

    printf(
    "bLength = 0x%x\n", ed->bLength
    );

    printf(
    "bDescriptorType = 0x%x ( %s )\n", ed->bDescriptorType, usbDescriptorTypeString( ed->bDescriptorType )
    );


        if ( USB_ENDPOINT_DIRECTION_IN( ed->bEndpointAddress ) ) {
                printf(
                "bEndpointAddress= 0x%x ( INPUT )\n", ed->bEndpointAddress
                );
        } else {
                printf(
                "bEndpointAddress= 0x%x ( OUTPUT )\n", ed->bEndpointAddress
                );
        }

    printf(
    "bmAttributes= 0x%x ( %s )\n", ed->bmAttributes, usbEndPointTypeString ( ed->bmAttributes )
    );


    printf(
    "wMaxPacketSize= 0x%x, decimal %d\n", ed->wMaxPacketSize, ed->wMaxPacketSize
    );
    printf(
    "bInterval = 0x%x, decimal %d\n", ed->bInterval, ed->bInterval
    );
}

void
rw_dev( HANDLE hDEV )
/*++
Routine Description:

    Called to do formatted ascii dump to console of  USB
    configuration, interface, and endpoint descriptors
    (Cmdline "rwbulk -u" )

Arguments:

    handle to device

Return Value:

    none

--*/
{
        UINT success;
        int siz, nBytes;
        char buf[256];
    PUSB_CONFIGURATION_DESCRIPTOR cd;
    PUSB_INTERFACE_DESCRIPTOR id;
    PUSB_ENDPOINT_DESCRIPTOR ed;

        siz = sizeof(buf);

        if (hDEV == INVALID_HANDLE_VALUE) {
                NOISY(("DEV not open"));
                return;
        }
        
        success = DeviceIoControl(hDEV,
                        IOCTL_BULKUSB_GET_CONFIG_DESCRIPTOR,
                        buf,
                        siz,
                        buf,
                        siz,
                        &nBytes,
                        NULL);

        NOISY(("request complete, success = %d nBytes = %d\n", success, nBytes));
        
        if (success) {
        ULONG i;
                UINT  j, n;
        char *pch;

        pch = buf;
                n = 0;

        cd = (PUSB_CONFIGURATION_DESCRIPTOR) pch;

        print_USB_CONFIGURATION_DESCRIPTOR( cd );

        pch += cd->bLength;

        do {

            id = (PUSB_INTERFACE_DESCRIPTOR) pch;

            print_USB_INTERFACE_DESCRIPTOR(id, n++);

            pch += id->bLength;
            for (j=0; j<id->bNumEndpoints; j++) {

                ed = (PUSB_ENDPOINT_DESCRIPTOR) pch;

                print_USB_ENDPOINT_DESCRIPTOR(ed,j);

                pch += ed->bLength;
            }
            i = (ULONG)(pch - buf);
        } while (i<cd->wTotalLength);

        }
        
        return;

}


int  dumpUsbConfig()
/*++
Routine Description:

    Called to do formatted ascii dump to console of  USB
    configuration, interface, and endpoint descriptors
    (Cmdline "rwbulk -u" )
	调用此函数可将格式化的ascii转储到USB控制台
	配置、接口和端点描述符

Arguments:

    none

Return Value:

    none

--*/
{

        HANDLE hDEV = open_dev();

        if ( hDEV )
        {
                rw_dev( hDEV );
                CloseHandle(hDEV);
        }

        return 0;
}
//  End, routines for USB configuration and pipe info dump  (Cmdline "rwbulk -u" )



int _cdecl main(
    int argc,
        char *argv[])
/*++
Routine Description:

    Entry point to rwbulk.exe
    Parses cmdline, performs user-requested tests

Arguments:

    argc, argv  standard console  'c' app arguments

Return Value:

    Zero

--*/

{
    char *pinBuf = NULL, *poutBuf = NULL;
    int nBytesRead, nBytesWrite, nBytes;
    ULONG i, j;
    int ok;
    UINT success;
    HANDLE hRead = INVALID_HANDLE_VALUE, hWrite = INVALID_HANDLE_VALUE;
	char buf[1024];
	clock_t start, finish;
	ULONG totalBytes = 0L;
	double seconds;
	ULONG fail = 0L;

	hWrite = open_file(outPipe);


    parse(argc, argv );

        // dump USB configuation and pipe info
		//转储USB配置和管道信息
        if( fDumpUsbConfig ) {
                dumpUsbConfig();
        }


        // doing a read, write, or both test
        if ((fRead) || (fWrite)) {

            if (fRead) {
            //
            // open the output file
            //
                        if ( fDumpReadData ) { // round size to sizeof ULONG for readable dumping
                                while( ReadLen % sizeof( ULONG ) )
                                                ReadLen++;
                        }

            hRead = open_file( inPipe);
        
                pinBuf = malloc(ReadLen);

            }

            if (fWrite) {

                        if ( fDumpReadData ) { // round size to sizeof ULONG for readable dumping
                                while( WriteLen % sizeof( ULONG ) )
                                                WriteLen++;
                        }

                hWrite = open_file( outPipe);
                poutBuf = malloc(WriteLen);
            }


        for (i=0; i<IterationCount; i++) {

            if (fWrite && poutBuf && hWrite != INVALID_HANDLE_VALUE) {

                                PULONG pOut = (PULONG) poutBuf;
                                ULONG  numLongs = WriteLen / sizeof( ULONG );
                //
                // put some data in the output buffer
                //

                for (j=0; j<numLongs; j++) {
                    *(pOut+j) = j;
                }

                //
                // send the write
                //

                    WriteFile(hWrite,
                              poutBuf,
                              WriteLen,
                              &nBytesWrite,
                              NULL);

                    printf("<%s> W (%04.4d) : request %06.6d bytes -- %06.6d bytes written\n",
                            outPipe, i, WriteLen, nBytesWrite);
                assert(nBytesWrite == WriteLen);
                }

                if (fRead && pinBuf) {

                    success = ReadFile(hRead,
                                  pinBuf,
                              ReadLen,
                                  &nBytesRead,
                                  NULL);

                    printf("<%s> R (%04.4d) : request %06.6d bytes -- %06.6d bytes read\n",
                        inPipe, i, ReadLen, nBytesRead);

                if (fWrite) {

                    //
                    // validate the input buffer against what
                    // we sent to the 82930 (loopback test)
                    //

                    ok = compare_buffs(pinBuf, poutBuf,  nBytesRead);

                                        if( fDumpReadData ) {
                                                printf("Dumping read buffer\n");
                                                dump( pinBuf, nBytesRead );     
                                                printf("Dumping write buffer\n");
                                                dump( poutBuf, nBytesRead );

                                        }

                    assert(ok);

                                        if(ok != 1)
                                                fail++;

                    assert(ReadLen == WriteLen);
                    assert(nBytesRead == ReadLen);
                    assert(nBytesWrite == WriteLen);
                }
                }
        
        }


        if (pinBuf) {
            free(pinBuf);
        }

        if (poutBuf) {
            free(poutBuf);
        }


                // close devices if needed
                if(hRead != INVALID_HANDLE_VALUE)
                        CloseHandle(hRead);
                if(hWrite != INVALID_HANDLE_VALUE)
                        CloseHandle(hWrite);

    }           

        return 0;
}


