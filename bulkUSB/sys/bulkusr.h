/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    sSUsr.h

Abstract:

Environment:

    Kernel mode

Notes:

    Copyright (c) 2000 Microsoft Corporation.  
    All Rights Reserved.

--*/

#ifndef _BULKUSB_USER_H
#define _BULKUSB_USER_H

#include <initguid.h>

//DEFINE_GUID(GUID_CLASS_I82930_BULK, 
//0x873fdf, 0x61a8, 0x11d1, 0xaa, 0x5e, 0x0, 0xc0, 0x4f, 0xb1, 0x72, 0x8b);
//f18a0e88-c30c-11d0-8815-00a0c906bed8
DEFINE_GUID(GUID_CLASS_I82930_BULK,
	0xf18a0e88, 0xc30c, 0x11d0, 0x88, 0x15, 0x0, 0xa0, 0xc9, 0x06, 0xbe, 0xd8);
//{f18a0e88 - c30c - 11d0 - 8815 - 00a0c906bed8}
#define BULKUSB_IOCTL_INDEX             0x0000


#define IOCTL_BULKUSB_GET_CONFIG_DESCRIPTOR CTL_CODE(FILE_DEVICE_UNKNOWN,     \
                                                     BULKUSB_IOCTL_INDEX,     \
                                                     METHOD_BUFFERED,         \
                                                     FILE_ANY_ACCESS)
                                                   
#define IOCTL_BULKUSB_RESET_DEVICE          CTL_CODE(FILE_DEVICE_UNKNOWN,     \
                                                     BULKUSB_IOCTL_INDEX + 1, \
                                                     METHOD_BUFFERED,         \
                                                     FILE_ANY_ACCESS)

#define IOCTL_BULKUSB_RESET_PIPE            CTL_CODE(FILE_DEVICE_UNKNOWN,     \
                                                     BULKUSB_IOCTL_INDEX + 2, \
                                                     METHOD_BUFFERED,         \
                                                     FILE_ANY_ACCESS)

#endif

