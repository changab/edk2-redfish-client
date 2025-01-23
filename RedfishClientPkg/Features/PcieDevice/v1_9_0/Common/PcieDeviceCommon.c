/** @file
  Redfish feature driver implementation - common functions

  Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PcieDeviceCommon.h"
#include <Library/PrintLib.h>

CHAR8  PcieDeviceEmptyJson[] = "{\"@odata.id\": \"\", \"@odata.type\": \"#PCIeDevice.v1_9_0.PCIeDevice\", \"Id\": \"\", \"Name\": \"\"}";

REDFISH_RESOURCE_COMMON_PRIVATE  *mRedfishResourcePrivate             = NULL;
EFI_HANDLE                       mRedfishResourceConfigProtocolHandle = NULL;
REDFISH_SCHEMA_INFO              mSchemaInfo                          = {
  { RESOURCE_SCHEMA        },
  { RESOURCE_SCHEMA_MAJOR  },
  { RESOURCE_SCHEMA_MINOR  },
  { RESOURCE_SCHEMA_ERRATA }
};

/**
  Consume resource from given URI.

  @param[in]   This                Pointer to REDFISH_RESOURCE_COMMON_PRIVATE instance.
  @param[in]   Json                The JSON to consume.
  @param[in]   HeaderEtag          The Etag string returned in HTTP header.

  @retval EFI_SUCCESS              Value is returned successfully.
  @retval Others                   Some error happened.

**/
EFI_STATUS
RedfishConsumeResourceCommon (
  IN  REDFISH_RESOURCE_COMMON_PRIVATE  *Private,
  IN  CHAR8                            *Json,
  IN  CHAR8                            *HeaderEtag OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
PatchPCIeInterface(
  IN      REDFISH_SYSTEM_TOPOLOGY_PCIE  *ThisPcieTopology,
  IN OUT  CHAR8                         **Json
  )
{
  CHAR8   *PatchedJson = NULL;
  CHAR8   *OriginalJson;
  UINTN   PatchedJsonLen;

  OriginalJson = *Json;

  PatchedJsonLen = AsciiStrLen(OriginalJson) * 2;

  PatchedJson = AllocateZeroPool (PatchedJsonLen);
  if(PatchedJson != NULL) {
    AsciiSPrint(PatchedJson,
                PatchedJsonLen,
                "{\n\"MaxLanes\": %d,\n\"LanesInUse\": %d,\n\"MaxPCIeType\": \"%d\",\n\"PCIeType\": \"%d\",\n%a",
                ThisPcieTopology->MaxLanes,
                ThisPcieTopology->LanesInUse,
                ThisPcieTopology->MaxPCIeType,
                ThisPcieTopology->PCIeType,
                OriginalJson + 4);

    FreePool(*Json);
    *Json = PatchedJson;
  }

  return EFI_SUCCESS;
}

/**
  Provisioning one redfish PCIrDevice resource

  @param[in]    JsonStructProtocol  Pointer EFI_REST_JSON_STRUCTURE_PROTOCOL.
  @param[in]    InputJson           Input PCIeDevice JSON template.
  @param[in]    ThisPcieTopology    Pointer to this REDFISH_SYSTEM_TOPOLOGY_PCIE.
  @param[in]    IsCreateOrUpdate    Is to create the new resource of PCIeDevice.
  @param[out]   ResultJson          The result JSON of new PCIeDevice resource.
  @retval EFI_STATUS                Some error happened.

**/
EFI_STATUS
ProvisioningPcieProperties (
  IN  EFI_REST_JSON_STRUCTURE_PROTOCOL *JsonStructProtocol,
  IN  CHAR8                            *InputJson,
  IN  REDFISH_SYSTEM_TOPOLOGY_PCIE     *ThisPcieTopology,
  IN  BOOLEAN                           IsCreateOrUpdate,
  OUT CHAR8                             **ResultJson
  )
{
  //
  // We dont use IsCreateOrUpdate to update PCIeDevice resource
  // at the moment as we are provision the inventory information.
  //
  EFI_REDFISH_PCIEDEVICE_V1_9_0     *PcieDevice;
  EFI_REDFISH_PCIEDEVICE_V1_9_0_CS  *PcieDeviceCs;
  EFI_STATUS                        Status;
  CHAR8                             *PatchedJson;
  UINT8                              StringLength = 17;

  if ((JsonStructProtocol == NULL) || (ResultJson == NULL) || IS_EMPTY_STRING (InputJson)) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((REDFISH_DEBUG_TRACE, "%a provision PCIeDevice with: %s\n", __func__, (IsCreateOrUpdate ? L"Provision resource" : L"Update resource")));

  *ResultJson     = NULL;
  PcieDevice      = NULL;
  PatchedJson     = NULL;
  
  if (PcdGetBool (PcdRedfishCompatibleSchemaSupport)) {
    Status = RedfishSetCompatibleSchemaVersion (&mSchemaInfo, InputJson, &PatchedJson);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a, cannot set compatible schema version: %r\n", __func__, Status));
      return Status;
    }
  }

  Status = JsonStructProtocol->ToStructure (
                                 JsonStructProtocol,
                                 NULL,
                                 (PatchedJson == NULL ? InputJson : PatchedJson),
                                 (EFI_REST_JSON_STRUCTURE_HEADER **)&PcieDevice
                                 );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a, ToStructure failure: %r\n", __func__, Status));
    goto ON_RELEASE;
  }

  PcieDeviceCs = PcieDevice->PCIeDevice;

  //
  // Handle SerialNumber
  //
  PcieDeviceCs->SerialNumber = AllocateZeroPool (StringLength);
  if(PcieDeviceCs->SerialNumber != NULL) {
    AsciiSPrint(PcieDeviceCs->SerialNumber,
                StringLength,
                "%08x%08x",
                ThisPcieTopology->SerialNumber.Upper, 
                ThisPcieTopology->SerialNumber.Lower);
    DEBUG((DEBUG_INFO, "Serial Number = %a\n", PcieDeviceCs->SerialNumber));
  }

  PcieDeviceCs->DeviceType = AllocateZeroPool (StringLength);
  if(PcieDeviceCs->DeviceType != NULL) {
    AsciiSPrint(PcieDeviceCs->DeviceType,
                StringLength,
                "%a",
                ThisPcieTopology->DeviceType ? "Multi Function" : "Single Function");
    DEBUG((DEBUG_INFO, "Device Type = %a\n", PcieDeviceCs->DeviceType));
  }

  PcieDeviceCs->Manufacturer = AllocateZeroPool (StringLength);
  if(PcieDeviceCs->Manufacturer != NULL) {
    AsciiSPrint(PcieDeviceCs->Manufacturer,
                StringLength,
                "%04x",
                ThisPcieTopology->Manufacturer);
    DEBUG((DEBUG_INFO, "Manufacturer = %a\n", PcieDeviceCs->Manufacturer));
  }

  PcieDeviceCs->Model = AllocateZeroPool (StringLength);
  if(PcieDeviceCs->Model != NULL) {
    AsciiSPrint(PcieDeviceCs->Model,
                StringLength,
                "%04x",
                ThisPcieTopology->Model);
    DEBUG((DEBUG_INFO, "Model = %a\n", PcieDeviceCs->Model));
  }
  //
  // Convert C structure back to JSON text.
  //
  Status = JsonStructProtocol->ToJson (
                                 JsonStructProtocol,
                                 (EFI_REST_JSON_STRUCTURE_HEADER *)PcieDevice,
                                 ResultJson
                                 );

  PatchPCIeInterface(ThisPcieTopology, ResultJson);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a, ToJson() failed: %r\n", __func__, Status));
  }

ON_RELEASE:
  //
  // Release resource.
  //
  if (PcieDevice != NULL) {
    JsonStructProtocol->DestoryStructure (
                          JsonStructProtocol,
                          (EFI_REST_JSON_STRUCTURE_HEADER *)PcieDevice
                          );
  }

  if (PatchedJson != NULL) {
    FreePool (PatchedJson);
  }

  return Status;
}

/**
  Provisioning one redfish PCIrDevice resource

  @param[in]    Private           Pointer to REDFISH_RESOURCE_COMMON_PRIVATE.
  @param[in]    ThisPcieTopology  Pointer to this REDFISH_SYSTEM_TOPOLOGY_PCIE.
  @param[in]    PciIndexId        Zero-based index of PCIe device.
  @param[out]   UriReturned       The URI returned that contains newly created resource.
  @retval EFI_STATUS             Some error happened.

**/
EFI_STATUS
ProvisioningPcieDeviceResource (
  IN  REDFISH_RESOURCE_COMMON_PRIVATE  *Private,
  IN  REDFISH_SYSTEM_TOPOLOGY_PCIE     *ThisPcieTopology,
  IN  UINTN                            PciIndexId,
  OUT CHAR8                            **UriReturned
  )
{
  CHAR8             *Json;
  EFI_STATUS        Status;
  EFI_STRING        NewResourceLocation;
  CHAR16            ResourceId[16];
  CHAR16            *UriInstance;
  UINTN             SizeString;
  REDFISH_RESPONSE  Response;

  if ((Private == NULL) || ( ThisPcieTopology == NULL) || (UriReturned == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Json                = NULL;
  UriInstance         = NULL;
  NewResourceLocation = NULL;

  ZeroMem (&Response, sizeof (REDFISH_RESPONSE));
  UnicodeSPrint (ResourceId, sizeof (ResourceId), L"%d", PciIndexId);

  Status = ProvisioningPcieProperties (
             Private->JsonStructProtocol,
             PcieDeviceEmptyJson,
             ThisPcieTopology,
             TRUE,
             &Json
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a, provisioning resource for #%d PCIe device is failed: %r\n", __func__, PciIndexId, Status));
    return Status;
  }

  //
  // Generate the URI for the new resource
  //
  SizeString = (StrLen (Private->Uri) + StrLen (L"/") + StrLen (ResourceId) + 1) * sizeof (CHAR16);
  UriInstance = AllocateZeroPool (SizeString);
  if (UriInstance == NULL) {
    DEBUG ((DEBUG_ERROR, "%a, Memory allocate fail for Generating the URI of the new resource: %r\n", __func__, Status));
    return Status;
  }
  UnicodeSPrint (UriInstance, SizeString, L"%s%s%s", Private->Uri, L"/", ResourceId);
  Status = RedfishHttpPostResource (Private->RedfishService, UriInstance, Json, &Response);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a, post PCIe resource failed: %r\n", __func__, Status));
    goto RELEASE_RESOURCE;
  }

  //
  // Per Redfish spec. the URL of new resource will be returned in "Location" header.
  //
  Status = GetHttpResponseLocation (&Response, &NewResourceLocation);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: cannot find new location: %r\n", __func__, Status));
    goto RELEASE_RESOURCE;
  }

  //
  // Keep location of new resource.
  //
  if (NewResourceLocation != NULL) {
    DEBUG ((DEBUG_MANAGEABILITY, "%a: Location: %s\n", __func__, NewResourceLocation));
  }

RELEASE_RESOURCE:
  if (NewResourceLocation != NULL) {
    FreePool (NewResourceLocation);
  }

  if (Json != NULL) {
    FreePool (Json);
  }

  if (UriInstance != NULL) {
    FreePool(UriInstance);
  }

  return Status;
}
/**
  Provisioning redfish PCIrDevice resources

  @param[in]   Private             Pointer to REDFISH_RESOURCE_COMMON_PRIVATE.
  @retval EFI_STATUS               Some error happened.

**/
EFI_STATUS
ProvisioningPcieDeviceResources (
  IN  REDFISH_RESOURCE_COMMON_PRIVATE  *Private
  )
{
  UINTN                                 NumberOfPcie;
  UINTN                                 Index;
  EFI_STATUS                            Status;
  EDKII_REDFISH_SYSTEM_TOPOLOGY_DEVICE  *PciTopology;
  CHAR8                                 **PcieResourceUri;
  CHAR8                                 **UriReturned;
  REDFISH_FEATURE_ARRAY_TYPE_URI        *ReturnUris;

  if (Private == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  Status = RedfishSystemTopologyGetCount (REDFISH_SYSTEM_TOPOLOGY_TYPE_PCIE, &NumberOfPcie);
  DEBUG((DEBUG_INFO, "NumberOfPcie = %d\n", NumberOfPcie));
  if (EFI_ERROR (Status)) {
    if (Status == EFI_NOT_FOUND || NumberOfPcie == 0) {
      DEBUG ((REDFISH_DEBUG_TRACE, "%a, No PCIe device to provision.\n", __func__));
      return EFI_SUCCESS;
    } else {
      DEBUG ((DEBUG_ERROR, "%a, Failed to get Pcie device count.\n", __func__));
      return Status;
    }
  }

  PcieResourceUri = AllocateZeroPool (sizeof(CHAR8 *) * NumberOfPcie);
  if (PcieResourceUri == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Not enough memory for PCIeResourceUri\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }
  PciTopology = NULL;
  UriReturned = PcieResourceUri;
  Index       = 0;
  do {
    Status = RedfishGetSystemTopologyGetEntry (REDFISH_SYSTEM_TOPOLOGY_TYPE_PCIE, &PciTopology);
    DEBUG((DEBUG_INFO, "RedfishGetSystemTopologyGetEntry Status = %r\n", Status));
    DEBUG((DEBUG_INFO, "PciTopology Address = 0x%x\n", PciTopology));
    if (!EFI_ERROR (Status)) {
      Status = ProvisioningPcieDeviceResource(Private, &PciTopology->DeviceType.PcieDevice, Index, UriReturned);
      DEBUG((DEBUG_INFO, "ProvisioningPcieDeviceResource Status = %r\n", Status));
      if (EFI_ERROR(Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Failed to provision this PCIe device.\n", __func__));
      }
    }
    Index++;
    if (!EFI_ERROR (Status)) {
      UriReturned++;
    }
  } while (PciTopology != NULL);

  // Set up the return exchange information.
  ReturnUris = AllocateZeroPool (sizeof(REDFISH_FEATURE_ARRAY_TYPE_URI));
  if (ReturnUris == NULL) {
    DEBUG ((DEBUG_ERROR, "%a, Not enough memory for REDFISH_FEATURE_ARRAY_TYPE_URI\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }
  ReturnUris->Count = ((UINTN)UriReturned - (UINTN)PcieResourceUri) / sizeof (*PcieResourceUri);
  ReturnUris->List  = PcieResourceUri;
  Private->InformationExchange->ReturnedInformation.Type                            =  InformationTypeArrayMemberUri;
  Private->InformationExchange->ReturnedInformation.ResourceTypeReturnedInformation = ReturnUris;
  return EFI_SUCCESS;
}

EFI_STATUS
ProvisioningPcieDeviceExistResource (
  IN  REDFISH_RESOURCE_COMMON_PRIVATE  *Private
  )
{

  return EFI_UNSUPPORTED;
}

/**
  Provisioning redfish resource by given URI.

  @param[in]   This                Pointer to EFI_HP_REDFISH_HII_PROTOCOL instance.
  @param[in]   ResourceExist       TRUE if resource exists, PUT method will be used.
                                   FALSE if resource does not exist POST method is used.

  @retval EFI_SUCCESS              Value is returned successfully.
  @retval Others                   Some error happened.

**/
EFI_STATUS
RedfishProvisioningResourceCommon (
  IN     REDFISH_RESOURCE_COMMON_PRIVATE  *Private,
  IN     BOOLEAN                          ResourceExist
  )
{
  if (Private == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return (ResourceExist ? ProvisioningPcieDeviceExistResource (Private) : ProvisioningPcieDeviceResources (Private));
}

/**
  Check resource from given URI.

  @param[in]   This                Pointer to REDFISH_RESOURCE_COMMON_PRIVATE instance.
  @param[in]   Json                The JSON to consume.
  @param[in]   HeaderEtag          The Etag string returned in HTTP header.

  @retval EFI_SUCCESS              Value is returned successfully.
  @retval Others                   Some error happened.

**/
EFI_STATUS
RedfishCheckResourceCommon (
  IN     REDFISH_RESOURCE_COMMON_PRIVATE  *Private,
  IN     CHAR8                            *Json,
  IN     CHAR8                            *HeaderEtag OPTIONAL
  )
{

  return EFI_SUCCESS;
}

/**
  Update resource to given URI.

  @param[in]   This                Pointer to REDFISH_RESOURCE_COMMON_PRIVATE instance.
  @param[in]   Json                The JSON to consume.

  @retval EFI_SUCCESS              Value is returned successfully.
  @retval Others                   Some error happened.

**/
EFI_STATUS
RedfishUpdateResourceCommon (
  IN     REDFISH_RESOURCE_COMMON_PRIVATE  *Private,
  IN     CHAR8                            *InputJson
  )
{

  return EFI_SUCCESS;
}

/**
  Identify resource from given URI.

  @param[in]   This                Pointer to REDFISH_RESOURCE_COMMON_PRIVATE instance.
  @param[in]   Json                The JSON to consume.

  @retval EFI_SUCCESS              Value is returned successfully.
  @retval Others                   Some error happened.

**/
EFI_STATUS
RedfishIdentifyResourceCommon (
  IN     REDFISH_RESOURCE_COMMON_PRIVATE  *Private,
  IN     CHAR8                            *Json
  )
{

  return EFI_SUCCESS;
}
