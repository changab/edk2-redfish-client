/** @file
  This file defines the Redfish system topology library interface.

  Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef REDFISH_SUSTEM_TOPOLOGY_LIB_H_
#define REDFISH_SUSTEM_TOPOLOGY_LIB_H_

#include <Uefi.h>

typedef enum {
  REDFISH_SYSTEM_TOPOLOGY_TYPE_NONE = 1,
  REDFISH_SYSTEM_TOPOLOGY_TYPE_PCIE,
  REDFISH_SYSTEM_TOPOLOGY_TYPE_MAX = 255
} REDFISH_SYSTEM_TOPOLOGY_TYPE;

typedef struct  {
  UINT8  Version;
} REDFISH_SYSTEM_TOPOLOGY_HEADER;

typedef struct  {
  UINTN  Index;
} REDFISH_SYSTEM_TOPOLOGY_PCIE;

typedef union {
  REDFISH_SYSTEM_TOPOLOGY_PCIE  PcieDevice;
} EDKII_REDFISH_SYSTEM_TOPOLOGY_DEVICE_TYPE;

typedef struct  {
  REDFISH_SYSTEM_TOPOLOGY_HEADER             Header;
  EDKII_REDFISH_SYSTEM_TOPOLOGY_DEVICE_TYPE  DeviceType;
} EDKII_REDFISH_SYSTEM_TOPOLOGY_DEVICE;

/**
  Get the count of specific REDFISH_SYSTEM_TOPOLOGY_TYPE.

  @param[in]    TopologyType     REDFISH_SYSTEM_TOPOLOGY_TYPE.
  @param[out]   Count            Count of this REDFISH_SYSTEM_TOPOLOGY_TYPE.
  @retval EFI_STATUS             Some error happened.

**/
EFI_STATUS
RedfishSystemTopologyGetCount (
  IN   REDFISH_SYSTEM_TOPOLOGY_TYPE  TopologyType,
  OUT  UINTN                         *Count
  );

/**
  Get the count of specific REDFISH_SYSTEM_TOPOLOGY_TYPE.

  @param[in]    TopologyType     REDFISH_SYSTEM_TOPOLOGY_TYPE.
  @param[out]   DeviceEntry      NULL to get the first entry of REDFISH_SYSTEM_TOPOLOGY_TYPE device.
                                 Otherwise, returns the next entry of REDFISH_SYSTEM_TOPOLOGY_TYPE device.
  @retval EFI_STATUS             Some error happened.

**/
EFI_STATUS
RedfishGetSystemTopologyGetEntry (
  IN  REDFISH_SYSTEM_TOPOLOGY_TYPE          TopologyType,
  IN  EDKII_REDFISH_SYSTEM_TOPOLOGY_DEVICE  **DeviceEntry
  );

#endif  // REDFISH_SUSTEM_TOPOLOGY_LIB_H_
