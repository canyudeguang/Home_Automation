/*
 * Copyright (c) 2016-2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 */
// Copyright (c) 2018 Qualcomm Technologies, Inc.
// All rights reserved.
// Redistribution and use in source and binary forms, with or without modification, are permitted (subject to the limitations in the disclaimer below) 
// provided that the following conditions are met:
// Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright notice, 
// this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
// Neither the name of Qualcomm Technologies, Inc. nor the names of its contributors may be used to endorse or promote products derived 
// from this software without specific prior written permission.
// NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE. 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, 
// BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
// IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, 
// OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "malloc.h"
#include "string.h"
#include "stringl.h"
#include "qcli_api.h"
#include "qcli_util.h"
#include "zigbee_demo.h"
#include "zcl_demo.h"
#include "zcl_doorlock_demo.h"

#include "qapi_zb.h"
#include "qapi_zb_cl.h"
#include "qapi_zb_cl_doorlock.h"

#define ZCL_DOORLOCK_DEMO_PIN_MAX_LENGTH           (8)

/* Structure representing the ZigBee door lock demo context information. */
typedef struct ZigBee_DoorLock_Demo_Context_s
{
   QCLI_Group_Handle_t QCLI_Handle;     /*< QCLI handle for the main ZigBee demo. */
} ZigBee_DoorLock_Demo_Context_t;

/* The ZigBee door lock demo context. */
static ZigBee_DoorLock_Demo_Context_t ZigBee_DoorLock_Demo_Context;

/* Function prototypes. */
static QCLI_Command_Status_t cmd_ZCL_DoorLock_Lock(uint32_t Parameter_Count, QCLI_Parameter_t *Parameter_List);
static QCLI_Command_Status_t cmd_ZCL_DoorLock_Unlock(uint32_t Parameter_Count, QCLI_Parameter_t *Parameter_List);
static QCLI_Command_Status_t cmd_ZCL_DoorLock_Toggle(uint32_t Parameter_Count, QCLI_Parameter_t *Parameter_List);
static QCLI_Command_Status_t cmd_ZCL_DoorLock_SetServerPINCode(uint32_t Parameter_Count, QCLI_Parameter_t *Parameter_List);
static QCLI_Command_Status_t cmd_ZCL_DoorLock_GetServerPINCode(uint32_t Parameter_Count, QCLI_Parameter_t *Parameter_List);

static void ZCL_DoorLock_Demo_Server_CB(qapi_ZB_Handle_t ZB_Handle, qapi_ZB_Cluster_t Cluster, qapi_ZB_CL_DoorLock_Server_Event_Data_t *EventData, uint32_t CB_Param);
static void ZCL_DoorLock_Demo_Client_CB(qapi_ZB_Handle_t ZB_Handle, qapi_ZB_Cluster_t Cluster, qapi_ZB_CL_DoorLock_Client_Event_Data_t *EventData, uint32_t CB_Param);

/* Command list for the ZigBee light demo. */
const QCLI_Command_t ZigBee_DoorLock_CMD_List[] =
{
   /* cmd_function                     thread  cmd_string          usage_string                               description */
   {cmd_ZCL_DoorLock_Lock,             false,  "Lock",             "[DevId][ClientEndpoint][PIN (optional)]", "Sends a Lock command to a DoorLock server."},
   {cmd_ZCL_DoorLock_Unlock,           false,  "Unlock",           "[DevId][ClientEndpoint][PIN (optional)]", "Sends an Unlock command to a DoorLock server."},
   {cmd_ZCL_DoorLock_Toggle,           false,  "Toggle",           "[DevId][ClientEndpoint][PIN (optional)]", "Sends a Toggle command to a DoorLock server."},
   {cmd_ZCL_DoorLock_SetServerPINCode, false,  "SetServerPINCode", "[ServerEndpoint][PINCode]",               "Sets the PIN code on a door lock server cluster."},
   {cmd_ZCL_DoorLock_GetServerPINCode, false,  "GetServerPINCode", "[ServerEndpoint]",                        "Gets the PIN code from a door lock server cluster."}
};

const QCLI_Command_Group_t ZCL_DoorLock_Cmd_Group = {"DoorLock", sizeof(ZigBee_DoorLock_CMD_List) / sizeof(QCLI_Command_t), ZigBee_DoorLock_CMD_List};

/**
   @brief Executes the "Lock" command to send a door lock Lock command to a
          device.

   Parameter_List[0] is the index of the device to send to. Use index zero to
                     use the binding table (if setup).
   Parameter_List[1] Endpoint of the door lock client cluster to use to send the
                     command.
   Parameter_List[2] is the optional pincode.

   @param Parameter_Count is number of elements in Parameter_List.
   @param Parameter_List is list of parsed arguments associate with this
          command.

   @return
    - QCLI_STATUS_SUCCESS_E indicates the command is executed successfully.
    - QCLI_STATUS_ERROR_E indicates the command is failed to execute.
    - QCLI_STATUS_USAGE_E indicates there is usage error associated with this
      command.
*/
static QCLI_Command_Status_t cmd_ZCL_DoorLock_Lock(uint32_t Parameter_Count, QCLI_Parameter_t *Parameter_List)
{
   qapi_Status_t                   Result;
   QCLI_Command_Status_t           Ret_Val;
   ZCL_Demo_Cluster_Info_t        *ClusterInfo;
   qapi_ZB_CL_General_Send_Info_t  SendInfo;
   uint8_t                         DeviceId;
   qapi_ZB_CL_DoorLock_PIN_t       PIN;

   /* Ensure both the stack is initialized and the switch endpoint. */
   if(GetZigBeeHandle() != NULL)
   {
      Ret_Val = QCLI_STATUS_SUCCESS_E;

      /* Validate the device ID. */
      if((Parameter_Count >= 2) &&
         (Parameter_List[0].Integer_Is_Valid) &&
         (Verify_Integer_Parameter(&(Parameter_List[1]), QAPI_ZB_APS_MIN_ENDPOINT, QAPI_ZB_APS_MAX_ENDPOINT)))
      {
         DeviceId      = Parameter_List[0].Integer_Value;
         ClusterInfo   = ZCL_FindClusterByEndpoint((uint8_t)(Parameter_List[1].Integer_Value), QAPI_ZB_CL_CLUSTER_ID_DOORLOCK, ZCL_DEMO_CLUSTERTYPE_CLIENT);
         PIN.PINCode   = NULL;
         PIN.PINLength = 0;

         if(Parameter_Count >= 3)
         {
            PIN.PINLength = strlen(Parameter_List[2].String_Value);
            PIN.PINCode   = (uint8_t *)(Parameter_List[2].String_Value);
         }

         if(ClusterInfo != NULL)
         {
            memset(&SendInfo, 0, sizeof(SendInfo));

            /* Format the destination addr. mode, address, and endpoint. */
            if(Format_Send_Info_By_Device(DeviceId, &SendInfo))
            {
               Result = qapi_ZB_CL_DoorLock_Send_Lock(ClusterInfo->Handle, &SendInfo, &PIN);
               if(Result == QAPI_OK)
               {
                  Display_Function_Success(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "qapi_ZB_CL_DoorLock_Send_Lock");
               }
               else
               {
                  Ret_Val = QCLI_STATUS_ERROR_E;
                  Display_Function_Error(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "qapi_ZB_CL_DoorLock_Send_Lock", Result);
               }
            }
            else
            {
               QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "Invalid device ID.\n");
               Ret_Val = QCLI_STATUS_ERROR_E;
            }
         }
         else
         {
            QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "Invalid Cluster Index.\n");
            Ret_Val = QCLI_STATUS_ERROR_E;
         }
      }
      else
      {
         Ret_Val = QCLI_STATUS_USAGE_E;
      }
   }
   else
   {
      QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "ZigBee stack is not initialized.\n");
      Ret_Val = QCLI_STATUS_ERROR_E;
   }

   return(Ret_Val);
}

/**
   @brief Executes the "Unlock" command to send a door lock Unlock command to a
          device.

   Parameter_List[0] is the index of the device to send to. Use index zero to
                     use the binding table (if setup).
   Parameter_List[1] Endpoint of the door lock client cluster to use to send the
                     command.
   Parameter_List[2] is the optional pincode.

   @param Parameter_Count is number of elements in Parameter_List.
   @param Parameter_List is list of parsed arguments associate with this
          command.

   @return
    - QCLI_STATUS_SUCCESS_E indicates the command is executed successfully.
    - QCLI_STATUS_ERROR_E indicates the command is failed to execute.
    - QCLI_STATUS_USAGE_E indicates there is usage error associated with this
      command.
*/
static QCLI_Command_Status_t cmd_ZCL_DoorLock_Unlock(uint32_t Parameter_Count, QCLI_Parameter_t *Parameter_List)
{
   qapi_Status_t                   Result;
   QCLI_Command_Status_t           Ret_Val;
   ZCL_Demo_Cluster_Info_t        *ClusterInfo;
   qapi_ZB_CL_General_Send_Info_t  SendInfo;
   uint8_t                         DeviceId;
   qapi_ZB_CL_DoorLock_PIN_t       PIN;

   /* Ensure both the stack is initialized and the switch endpoint. */
   if(GetZigBeeHandle() != NULL)
   {
      Ret_Val = QCLI_STATUS_SUCCESS_E;

      /* Validate the device ID. */
      if((Parameter_Count >= 2) &&
         (Parameter_List[0].Integer_Is_Valid) &&
         (Verify_Integer_Parameter(&(Parameter_List[1]), QAPI_ZB_APS_MIN_ENDPOINT, QAPI_ZB_APS_MAX_ENDPOINT)))
      {
         DeviceId      = Parameter_List[0].Integer_Value;
         ClusterInfo   = ZCL_FindClusterByEndpoint((uint8_t)(Parameter_List[1].Integer_Value), QAPI_ZB_CL_CLUSTER_ID_DOORLOCK, ZCL_DEMO_CLUSTERTYPE_CLIENT);
         PIN.PINCode   = NULL;
         PIN.PINLength = 0;

         if(Parameter_Count >= 3)
         {
            PIN.PINLength = strlen(Parameter_List[2].String_Value);
            PIN.PINCode   = (uint8_t *)(Parameter_List[2].String_Value);
         }

         if(ClusterInfo != NULL)
         {
            memset(&SendInfo, 0, sizeof(SendInfo));

            /* Format the destination addr. mode, address, and endpoint. */
            if(Format_Send_Info_By_Device(DeviceId, &SendInfo))
            {
               Result = qapi_ZB_CL_DoorLock_Send_Unlock(ClusterInfo->Handle, &SendInfo, &PIN);
               if(Result == QAPI_OK)
               {
                  Display_Function_Success(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "qapi_ZB_CL_DoorLock_Send_Unlock");
               }
               else
               {
                  Ret_Val = QCLI_STATUS_ERROR_E;
                  Display_Function_Error(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "qapi_ZB_CL_DoorLock_Send_Unlock", Result);
               }
            }
            else
            {
               QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "Invalid device ID.\n");
               Ret_Val = QCLI_STATUS_ERROR_E;
            }
         }
         else
         {
            QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "Invalid Cluster Index.\n");
            Ret_Val = QCLI_STATUS_ERROR_E;
         }
      }
      else
      {
         Ret_Val = QCLI_STATUS_USAGE_E;
      }
   }
   else
   {
      QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "ZigBee stack is not initialized.\n");
      Ret_Val = QCLI_STATUS_ERROR_E;
   }

   return(Ret_Val);
}

/**
   @brief Executes the "Toggle" command to send a door lock Toggle command to a
          device.

   Parameter_List[0] is the index of the device to send to. Use index zero to
                     use the binding table (if setup).
   Parameter_List[1] Endpoint of the door lock client cluster to use to send the
                     command.
   Parameter_List[2] is the optional pincode.

   @param Parameter_Count is number of elements in Parameter_List.
   @param Parameter_List is list of parsed arguments associate with this
          command.

   @return
    - QCLI_STATUS_SUCCESS_E indicates the command is executed successfully.
    - QCLI_STATUS_ERROR_E indicates the command is failed to execute.
    - QCLI_STATUS_USAGE_E indicates there is usage error associated with this
      command.
*/
static QCLI_Command_Status_t cmd_ZCL_DoorLock_Toggle(uint32_t Parameter_Count, QCLI_Parameter_t *Parameter_List)
{
   qapi_Status_t                   Result;
   QCLI_Command_Status_t           Ret_Val;
   ZCL_Demo_Cluster_Info_t        *ClusterInfo;
   qapi_ZB_CL_General_Send_Info_t  SendInfo;
   uint8_t                         DeviceId;
   qapi_ZB_CL_DoorLock_PIN_t       PIN;

   /* Ensure both the stack is initialized and the switch endpoint. */
   if(GetZigBeeHandle() != NULL)
   {
      Ret_Val = QCLI_STATUS_SUCCESS_E;

      /* Validate the device ID. */
      if((Parameter_Count >= 2) &&
         (Parameter_List[0].Integer_Is_Valid) &&
         (Verify_Integer_Parameter(&(Parameter_List[1]), QAPI_ZB_APS_MIN_ENDPOINT, QAPI_ZB_APS_MAX_ENDPOINT)))
      {
         DeviceId      = Parameter_List[0].Integer_Value;
         ClusterInfo   = ZCL_FindClusterByEndpoint((uint8_t)(Parameter_List[1].Integer_Value), QAPI_ZB_CL_CLUSTER_ID_DOORLOCK, ZCL_DEMO_CLUSTERTYPE_CLIENT);
         PIN.PINCode   = NULL;
         PIN.PINLength = 0;

         if(Parameter_Count >= 3)
         {
            PIN.PINLength = strlen(Parameter_List[2].String_Value);
            PIN.PINCode   = (uint8_t *)(Parameter_List[2].String_Value);
         }

         if(ClusterInfo != NULL)
         {
            memset(&SendInfo, 0, sizeof(SendInfo));

            /* Format the destination addr. mode, address, and endpoint. */
            if(Format_Send_Info_By_Device(DeviceId, &SendInfo))
            {
               Result = qapi_ZB_CL_DoorLock_Send_Toggle(ClusterInfo->Handle, &SendInfo, &PIN);
               if(Result == QAPI_OK)
               {
                  Display_Function_Success(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "qapi_ZB_CL_DoorLock_Send_Toggle");
               }
               else
               {
                  Ret_Val = QCLI_STATUS_ERROR_E;
                  Display_Function_Error(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "qapi_ZB_CL_DoorLock_Send_Toggle", Result);
               }
            }
            else
            {
               QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "Invalid device ID.\n");
               Ret_Val = QCLI_STATUS_ERROR_E;
            }
         }
         else
         {
            QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "Invalid Cluster Index.\n");
            Ret_Val = QCLI_STATUS_ERROR_E;
         }
      }
      else
      {
         Ret_Val = QCLI_STATUS_USAGE_E;
      }
   }
   else
   {
      QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "ZigBee stack is not initialized.\n");
      Ret_Val = QCLI_STATUS_ERROR_E;
   }

   return(Ret_Val);
}

/**
   @brief Executes the "SetServerPINCode" command to set the PIN code on a
          door lock server.

   Parameter_List[0] Endpoint of the door lock server cluster.
   Parameter_List[1] The PIN code to be set.

   @param Parameter_Count is number of elements in Parameter_List.
   @param Parameter_List is list of parsed arguments associate with this
          command.

   @return
    - QCLI_STATUS_SUCCESS_E indicates the command is executed successfully.
    - QCLI_STATUS_ERROR_E indicates the command is failed to execute.
    - QCLI_STATUS_USAGE_E indicates there is usage error associated with this
      command.
*/
static QCLI_Command_Status_t cmd_ZCL_DoorLock_SetServerPINCode(uint32_t Parameter_Count, QCLI_Parameter_t *Parameter_List)
{
   qapi_Status_t             Result;
   QCLI_Command_Status_t     Ret_Val;
   ZCL_Demo_Cluster_Info_t  *ClusterInfo;
   qapi_ZB_CL_DoorLock_PIN_t PIN;

   /* Ensure both the stack is initialized and the switch endpoint. */
   if(GetZigBeeHandle() != NULL)
   {
      if((Parameter_Count >= 2) &&
         (Verify_Integer_Parameter(&(Parameter_List[0]), QAPI_ZB_APS_MIN_ENDPOINT, QAPI_ZB_APS_MAX_ENDPOINT)))
      {
         ClusterInfo = ZCL_FindClusterByEndpoint((uint8_t)(Parameter_List[0].Integer_Value), QAPI_ZB_CL_CLUSTER_ID_DOORLOCK, ZCL_DEMO_CLUSTERTYPE_SERVER);
         if(ClusterInfo != NULL)
         {
            PIN.PINLength = strlen(Parameter_List[1].String_Value);
            PIN.PINCode   = (uint8_t *)(Parameter_List[1].String_Value);
            Result = qapi_ZB_CL_DoorLock_Server_Set_PIN_Code(ClusterInfo->Handle, &PIN);
            if(Result == QAPI_OK)
            {
               Display_Function_Success(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "qapi_ZB_CL_DoorLock_Server_Set_PIN_Code");
               Ret_Val = QCLI_STATUS_SUCCESS_E;
            }
            else
            {
               Display_Function_Error(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "qapi_ZB_CL_DoorLock_Server_Set_PIN_Code", Result);
               Ret_Val = QCLI_STATUS_ERROR_E;
            }
         }
         else
         {
            QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "Invalid Cluster Index.\n");
            Ret_Val = QCLI_STATUS_ERROR_E;
         }
      }
      else
      {
         Ret_Val = QCLI_STATUS_USAGE_E;
      }
   }
   else
   {
      QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "ZigBee stack is not initialized.\n");
      Ret_Val = QCLI_STATUS_ERROR_E;
   }

   return(Ret_Val);
}

/**
   @brief Executes the "GetServerPINCode" command to get the PIN code from a
          door lock server.

   Parameter_List[0] Endpoint of the IAS ACE server cluster.

   @param Parameter_Count is number of elements in Parameter_List.
   @param Parameter_List is list of parsed arguments associate with this
          command.

   @return
    - QCLI_STATUS_SUCCESS_E indicates the command is executed successfully.
    - QCLI_STATUS_ERROR_E indicates the command is failed to execute.
    - QCLI_STATUS_USAGE_E indicates there is usage error associated with this
      command.
*/
static QCLI_Command_Status_t cmd_ZCL_DoorLock_GetServerPINCode(uint32_t Parameter_Count, QCLI_Parameter_t *Parameter_List)
{
   qapi_Status_t              Result;
   QCLI_Command_Status_t      Ret_Val;
   ZCL_Demo_Cluster_Info_t   *ClusterInfo;
   qapi_ZB_CL_DoorLock_PIN_t  PIN;
   uint8_t                    Index;

   /* Ensure both the stack is initialized and the switch endpoint. */
   if(GetZigBeeHandle() != NULL)
   {
      if((Parameter_Count >= 1) &&
         (Verify_Integer_Parameter(&(Parameter_List[0]), QAPI_ZB_APS_MIN_ENDPOINT, QAPI_ZB_APS_MAX_ENDPOINT)))
      {
         ClusterInfo = ZCL_FindClusterByEndpoint((uint8_t)(Parameter_List[0].Integer_Value), QAPI_ZB_CL_CLUSTER_ID_DOORLOCK, ZCL_DEMO_CLUSTERTYPE_SERVER);
         if(ClusterInfo != NULL)
         {
            memset(&PIN, 0, sizeof(qapi_ZB_CL_DoorLock_PIN_t));
            PIN.PINLength = QAPI_ZB_CL_DOORLOCK_PIN_CODE_MAX_LENGTH;
            PIN.PINCode   = malloc(PIN.PINLength);
            if(PIN.PINCode != NULL)
            {
               Result = qapi_ZB_CL_DoorLock_Server_Get_PIN_Code(ClusterInfo->Handle, &PIN);
               if(Result == QAPI_OK)
               {
                  QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "Server PIN code: ");
                  for(Index = 0; Index < PIN.PINLength; Index++)
                  {
                     QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "%c", PIN.PINCode[Index]);
                  }
                  QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "\n");

                  Ret_Val = QCLI_STATUS_SUCCESS_E;
               }
               else
               {
                  Display_Function_Error(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "qapi_ZB_CL_DoorLock_Server_Get_PIN_Code", Result);
                  Ret_Val = QCLI_STATUS_ERROR_E;
               }

               free(PIN.PINCode);
            }
            else
            {
               QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "Failed to allocate the memory for PIN code.\n");
               Ret_Val = QCLI_STATUS_ERROR_E;
            }
         }
         else
         {
            QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "Invalid Cluster Index.\n");
            Ret_Val = QCLI_STATUS_ERROR_E;
         }
      }
      else
      {
         Ret_Val = QCLI_STATUS_USAGE_E;
      }
   }
   else
   {
      QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "ZigBee stack is not initialized.\n");
      Ret_Val = QCLI_STATUS_ERROR_E;
   }

   return(Ret_Val);
}

/**
   @brief Handles callbacks for the door lock server cluster.

   @param ZB_Handle is the handle of the ZigBee instance.
   @param EventData is the information for the cluster event.
   @param CB_Param  is the user specified parameter for the callback function.
*/
static void ZCL_DoorLock_Demo_Server_CB(qapi_ZB_Handle_t ZB_Handle, qapi_ZB_Cluster_t Cluster, qapi_ZB_CL_DoorLock_Server_Event_Data_t *EventData, uint32_t CB_Param)
{
   uint8_t PINCode[ZCL_DOORLOCK_DEMO_PIN_MAX_LENGTH + 1];
   uint8_t PINLength;

   if((ZB_Handle != NULL) && (Cluster != NULL) && (EventData != NULL))
   {
      switch(EventData->Event_Type)
      {
         case QAPI_ZB_CL_DOORLOCK_SERVER_EVENT_TYPE_LOCK_STATE_CHANGE_E:
            QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "DoorLock Server State Change:\n");
            QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "State:   %s\n", EventData->Data.State_Changed.Locked ? "Locked" : "Unlocked");
            if(EventData->Data.State_Changed.PIN.PINCode)
            {
               PINLength = EventData->Data.State_Changed.PIN.PINLength;
               if(PINLength > ZCL_DOORLOCK_DEMO_PIN_MAX_LENGTH)
               {
                  PINLength = ZCL_DOORLOCK_DEMO_PIN_MAX_LENGTH;
               }

               memscpy(PINCode, sizeof(PINCode), EventData->Data.State_Changed.PIN.PINCode, PINLength);
               PINCode[PINLength] = '\0';
               QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "PINCode: %s\n", PINCode);
            }
            break;

         default:
            QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "Unhandled DoorLock server event %d.\n", EventData->Event_Type);
            break;
      }

      QCLI_Display_Prompt();
   }
}

/**
   @brief Handles callbacks for the door lock client cluster.

   @param ZB_Handle is the handle of the ZigBee instance.
   @param EventData is the information for the cluster event.
   @param CB_Param  is the user specified parameter for the callback function.
*/
static void ZCL_DoorLock_Demo_Client_CB(qapi_ZB_Handle_t ZB_Handle, qapi_ZB_Cluster_t Cluster, qapi_ZB_CL_DoorLock_Client_Event_Data_t *EventData, uint32_t CB_Param)
{
   if((ZB_Handle != NULL) && (Cluster != NULL) && (EventData != NULL))
   {
      switch(EventData->Event_Type)
      {
         case QAPI_ZB_CL_DOORLOCK_CLIENT_EVENT_TYPE_DEFAULT_RESPONSE_E:
            QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "DoorLock Client Default Response:\n");
            QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "  Status:        %d\n", EventData->Data.Default_Response.Status);
            QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "  CommandID:     0x%02X\n", EventData->Data.Default_Response.CommandId);
            QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "  CommandStatus: %d\n", EventData->Data.Default_Response.CommandStatus);
            break;

         case QAPI_ZB_CL_DOORLOCK_CLIENT_EVENT_TYPE_LOCK_RESPONSE_E:
            QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "DoorLock Client Lock Response:\n");
            QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "  Status: %d\n",  EventData->Data.Lock_Response.Status);
            break;

         case QAPI_ZB_CL_DOORLOCK_CLIENT_EVENT_TYPE_UNLOCK_RESPONSE_E:
            QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "DoorLock Client UnLock Response:\n");
            QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "  Status: %d\n", EventData->Data.Unlock_Response.Status);
            break;

         case QAPI_ZB_CL_DOORLOCK_CLIENT_EVENT_TYPE_TOGGLE_RESPONSE_E:
            QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "DoorLock Client Toggle Response:\n");
            QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "  Status: %d\n",  EventData->Data.Toggle_Response.Status);
            break;

         case QAPI_ZB_CL_DOORLOCK_CLIENT_EVENT_TYPE_COMMAND_COMPLETE_E:
            QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "Command Complete:\n");
            QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "  CommandStatus: %d\n", EventData->Data.Command_Complete.CommandStatus);
            break;

         default:
            QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "Unhandled DoorLock client event %d.\n", EventData->Event_Type);
            break;
      }

      QCLI_Display_Prompt();
   }
}

/**
   @brief Initializes the ZCL door lock demo.

   @param ZigBee_QCLI_Handle is the parent QCLI handle for the door lock demo.

   @return true if the ZigBee light demo initialized successfully, false
           otherwise.
*/
qbool_t Initialize_ZCL_DoorLock_Demo(QCLI_Group_Handle_t ZigBee_QCLI_Handle)
{
   qbool_t Ret_Val;

   /* Register door lock command group. */
   ZigBee_DoorLock_Demo_Context.QCLI_Handle = QCLI_Register_Command_Group(ZigBee_QCLI_Handle, &ZCL_DoorLock_Cmd_Group);
   if(ZigBee_DoorLock_Demo_Context.QCLI_Handle != NULL)
   {
      Ret_Val = true;
   }
   else
   {
      QCLI_Printf(ZigBee_QCLI_Handle, "Failed to register ZCL DoorLock command group.\n");
      Ret_Val = false;
   }

   return(Ret_Val);
}

/**
   @brief Creates an door lock server cluster.

   @param Endpoint is the endpoint the cluster will be part of.
   @param PrivData is a pointer to the private data for the cluster demo.  This
                   will be initaialized to NULL before the create function is
                   called so can be ignored if the demo has no private data.

   @return The handle for the newly created function or NULL if there was an
           error.
*/
qapi_ZB_Cluster_t ZCL_DoorLock_Demo_Create_Server(uint8_t Endpoint, void **PrivData)
{
   qapi_ZB_Cluster_t         Ret_Val;
   qapi_Status_t             Result;
   qapi_ZB_Handle_t          ZigBee_Handle;
   qapi_ZB_CL_Cluster_Info_t ClusterInfo;

   ZigBee_Handle = GetZigBeeHandle();
   if(ZigBee_Handle != NULL)
   {
      memset(&ClusterInfo, 0, sizeof(qapi_ZB_CL_Cluster_Info_t));
      ClusterInfo.Endpoint = Endpoint;

      Result = qapi_ZB_CL_DoorLock_Create_Server(ZigBee_Handle, &Ret_Val, &ClusterInfo, ZCL_DoorLock_Demo_Server_CB, 0);
      if(Result != QAPI_OK)
      {
         Display_Function_Error(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "qapi_ZB_CL_DoorLock_Create_Server", Result);
         Ret_Val = NULL;
      }
   }
   else
   {
      QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "ZigBee stack is not initialized.\n");
      Ret_Val = NULL;
   }

   return(Ret_Val);
}

/**
   @brief Creates an door lock client cluster.

   @param Endpoint is the endpoint the cluster will be part of.
   @param PrivData is a pointer to the private data for the cluster demo.  This
                   will be initaialized to NULL before the create function is
                   called so can be ignored if the demo has no private data.

   @return The handle for the newly created function or NULL if there was an
           error.
*/
qapi_ZB_Cluster_t ZCL_DoorLock_Demo_Create_Client(uint8_t Endpoint, void **PrivData)
{
   qapi_ZB_Cluster_t         Ret_Val;
   qapi_Status_t             Result;
   qapi_ZB_Handle_t          ZigBee_Handle;
   qapi_ZB_CL_Cluster_Info_t ClusterInfo;

   ZigBee_Handle = GetZigBeeHandle();
   if(ZigBee_Handle != NULL)
   {
      memset(&ClusterInfo, 0, sizeof(qapi_ZB_CL_Cluster_Info_t));
      ClusterInfo.Endpoint = Endpoint;

      Result = qapi_ZB_CL_DoorLock_Create_Client(ZigBee_Handle, &Ret_Val, &ClusterInfo, ZCL_DoorLock_Demo_Client_CB, 0);
      if(Result != QAPI_OK)
      {
         Display_Function_Error(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "qapi_ZB_CL_DoorLock_Create_Client", Result);
         Ret_Val = NULL;
      }
   }
   else
   {
      QCLI_Printf(ZigBee_DoorLock_Demo_Context.QCLI_Handle, "ZigBee stack is not initialized.\n");
      Ret_Val = NULL;
   }

   return(Ret_Val);
}

