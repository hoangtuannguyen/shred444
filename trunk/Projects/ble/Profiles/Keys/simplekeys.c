/**************************************************************************************************
  Filename:       simplekey.c
  Revised:        $Date: 2011-03-10 12:40:02 -0800 (Thu, 10 Mar 2011) $
  Revision:       $Revision: 25357 $

  Description:    Simple Keys Profile


  Copyright 2010 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED �AS IS� WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, 
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE, 
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com. 
**************************************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "bcomdef.h"
#include "OSAL.h"
#include "linkdb.h"
#include "att.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"
#include "gapbondmgr.h"

#include "simplekeys.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

#define SERVAPP_NUM_ATTR_SUPPORTED        5

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
// SK Service UUID: 0x1800
CONST uint8 skServUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(SK_SERV_UUID), HI_UINT16(SK_SERV_UUID)
};

// Key Pressed UUID: 0x1801
CONST uint8 keyPressedUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(SK_KEYPRESSED_UUID), HI_UINT16(SK_KEYPRESSED_UUID)
};

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

/*********************************************************************
 * Profile Attributes - variables
 */

// SK Service attribute
static CONST gattAttrType_t skService = { ATT_BT_UUID_SIZE, skServUUID };

// Keys Pressed Characteristic Properties
static uint8 skCharProps = GATT_PROP_NOTIFY;

// Key Pressed State Characteristic
static uint8 skKeyPressed = 0;

// Key Pressed Characteristic Configs
static gattCharCfg_t skConfig[GATT_MAX_NUM_CONN];

// Key Pressed Characteristic User Description
static uint8 skCharUserDesp[16] = "Key Press State\0";


/*********************************************************************
 * Profile Attributes - Table
 */

static gattAttribute_t simplekeysAttrTbl[SERVAPP_NUM_ATTR_SUPPORTED] = 
{
  // Simple Keys Service
  { 
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&skService                       /* pValue */
  },

    // Characteristic Declaration for Keys
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &skCharProps 
    },

      // Characteristic Value- Key Pressed
      { 
        { ATT_BT_UUID_SIZE, keyPressedUUID },
        0, 
        0, 
        &skKeyPressed 
      },

      // Characteristic configuration
      { 
        { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
        0, 
        (uint8 *)skConfig 
      },

      // Characteristic User Description
      { 
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ, 
        0, 
        skCharUserDesp 
      },      
};


/*********************************************************************
 * LOCAL FUNCTIONS
 */
static uint8 sk_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen );
static bStatus_t sk_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset );
static void sk_ProcessCharCfg( gattCharCfg_t *charCfgTbl, 
                                     uint8 *pValue, uint8 authenticated );
static gattCharCfg_t *sk_FindCharCfgItem( uint16 connHandle, gattCharCfg_t *charCfgTbl );
static void sk_ResetCharCfg( uint16 connHandle, gattCharCfg_t *charCfgTbl );
static uint16 sk_ReadCharCfg( uint16 connHandle, gattCharCfg_t *charCfgTbl );
static uint8 sk_WriteCharCfg( uint16 connHandle, gattCharCfg_t *charCfgTbl, uint16 value );
static void sk_HandleConnStatusCB( uint16 connHandle, uint8 changeType );



/*********************************************************************
 * NETWORK LAYER CALLBACKS
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      SK_AddService
 *
 * @brief   Initializes the Simple Key service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 *
 * @return  Success or Failure
 */
bStatus_t SK_AddService( uint32 services )
{
  uint8 status = SUCCESS;

  // Initialize Client Characteristic Configuration attributes
  for ( uint8 i = 0; i < GATT_MAX_NUM_CONN; i++ )
  {
    skConfig[i].connHandle = INVALID_CONNHANDLE;
    skConfig[i].value = GATT_CFG_NO_OPERATION;

  }

  // Register with Link DB to receive link status change callback
  VOID linkDB_Register( sk_HandleConnStatusCB );  
  
  
  if ( services & SK_SERVICE )
  {
    // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService( simplekeysAttrTbl, GATT_NUM_ATTRS( simplekeysAttrTbl ),
                                          sk_ReadAttrCB, sk_WriteAttrCB, NULL );
  }

  return ( status );
}

/*********************************************************************
 * @fn      SK_SetParameter
 *
 * @brief   Set a Simple Key Profile parameter.
 *
 * @param   param - Profile parameter ID
 * @param   len - length of data to right
 * @param   pValue - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t SK_SetParameter( uint8 param, uint8 len, void *pValue )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
    case SK_KEY_ATTR:
      if ( len == sizeof ( uint8 ) ) 
      {
        skKeyPressed = *((uint8*)pValue);
        
         // See if Notification/Indication has been enabled
        sk_ProcessCharCfg( skConfig, &skKeyPressed, FALSE );

      }
      else
      {
        ret = bleInvalidRange;
      }
      break;
      
    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

/*********************************************************************
 * @fn      SK_GetParameter
 *
 * @brief   Get a Simple Key Profile parameter.
 *
 * @param   param - Profile parameter ID
 * @param   pValue - pointer to data to put.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t SK_GetParameter( uint8 param, void *pValue )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
    case SK_KEY_ATTR:
      *((uint8*)pValue) = skKeyPressed;
      break;
      
    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

/*********************************************************************
 * @fn          sk_ProcessCharCfg
 *
 * @brief       Process Client Charateristic Configuration change.
 *
 * @param       charCfgTbl - characteristic configuration table
 * @param       pValue - pointer to attribute value
 * @param       authenticated - whether an authenticated link is required
 *
 * @return      none
 */
static void sk_ProcessCharCfg( gattCharCfg_t *charCfgTbl, 
                                     uint8 *pValue, uint8 authenticated )
{
  for ( uint8 i = 0; i < GATT_MAX_NUM_CONN; i++ )
  {
    gattCharCfg_t *pItem = &(charCfgTbl[i]);

    if ( ( pItem->connHandle != INVALID_CONNHANDLE ) &&
         ( pItem->value != GATT_CFG_NO_OPERATION ) )
    {
      gattAttribute_t *pAttr;
  
      // Find the characteristic value attribute 
      pAttr = GATTServApp_FindAttr( simplekeysAttrTbl, GATT_NUM_ATTRS( simplekeysAttrTbl ), pValue );
      if ( pAttr != NULL )
      {
        attHandleValueNoti_t noti;
    
        // If the attribute value is longer than (ATT_MTU - 3) octets, then
        // only the first (ATT_MTU - 3) octets of this attributes value can
        // be sent in a notification.
        if ( sk_ReadAttrCB( pItem->connHandle, pAttr, noti.value,
                                  &noti.len, 0, (ATT_MTU_SIZE-3) ) == SUCCESS )
        {
          noti.handle = pAttr->handle;
  
          if ( pItem->value & GATT_CLIENT_CFG_NOTIFY )
          {
            VOID GATT_Notification( pItem->connHandle, &noti, authenticated );
          }

        }
      }
    }
  } // for
}

/*********************************************************************
 * @fn      sk_FindCharCfgItem
 *
 * @brief   Find the characteristic configuration for a given client.
 *          Uses the connection handle to search the charactersitic 
 *          configuration table of a client.
 *
 * @param   connHandle - connection handle.
 * @param   charCfgTbl - characteristic configuration table.
 *
 * @return  pointer to the found item. NULL, otherwise.
 */
static gattCharCfg_t *sk_FindCharCfgItem( uint16 connHandle, gattCharCfg_t *charCfgTbl )
{
  for ( uint8 i = 0; i < GATT_MAX_NUM_CONN; i++ )
  {
    if ( charCfgTbl[i].connHandle == connHandle )
    {
      // Entry found
      return ( &(charCfgTbl[i]) );
    }
  }

  return ( (gattCharCfg_t *)NULL );
}

/*********************************************************************
 * @fn      sk_ResetCharCfg
 *
 * @brief   Reset the client characteristic configuration for a given
 *          client.
 *
 * @param   connHandle - connection handle.
 * @param   charCfgTbl - client characteristic configuration table.
 *
 * @return  none
 */
static void sk_ResetCharCfg( uint16 connHandle, gattCharCfg_t *charCfgTbl )
{
  gattCharCfg_t *pItem;

  pItem = sk_FindCharCfgItem( connHandle, charCfgTbl );
  if ( pItem != NULL )
  {
    pItem->connHandle = INVALID_CONNHANDLE;
    pItem->value = GATT_CFG_NO_OPERATION;
  }
}

/*********************************************************************
 * @fn      sk_ReadCharCfg
 *
 * @brief   Read the client characteristic configuration for a given
 *          client.
 *
 *          Note: Each client has its own instantiation of the Client
 *                Characteristic Configuration. Reads of the Client 
 *                Characteristic Configuration only shows the configuration
 *                for that client.
 *
 * @param   connHandle - connection handle.
 * @param   charCfgTbl - client characteristic configuration table.
 *
 * @return  attribute value
 */
static uint16 sk_ReadCharCfg( uint16 connHandle, gattCharCfg_t *charCfgTbl )
{
  gattCharCfg_t *pItem;

  pItem = sk_FindCharCfgItem( connHandle, charCfgTbl );
  if ( pItem != NULL )
  {
    return ( (uint16)(pItem->value) );
  }

  return ( (uint16)GATT_CFG_NO_OPERATION );
}

/*********************************************************************
 * @fn      sk_WriteCharCfg
 *
 * @brief   Write the client characteristic configuration for a given
 *          client.
 *
 *          Note: Each client has its own instantiation of the Client 
 *                Characteristic Configuration. Writes of the Client
 *                Characteristic Configuration only only affect the 
 *                configuration of that client.
 *
 * @param   connHandle - connection handle.
 * @param   charCfgTbl - client characteristic configuration table.
 * @param   value - attribute new value.
 *
 * @return  Success or Failure
 */
static uint8 sk_WriteCharCfg( uint16 connHandle, gattCharCfg_t *charCfgTbl,
                                    uint16 value )
{
  gattCharCfg_t *pItem;

  pItem = sk_FindCharCfgItem( connHandle, charCfgTbl );
  if ( pItem == NULL )
  {
    pItem = sk_FindCharCfgItem( INVALID_CONNHANDLE, charCfgTbl );
    if ( pItem == NULL )
    {
      return ( ATT_ERR_INSUFFICIENT_RESOURCES );
    }

    pItem->connHandle = connHandle;
  }

  // Write the new value for this client
  pItem->value = value;

  return ( SUCCESS );
}

/*********************************************************************
 * @fn          sk_ReadAttrCB
 *
 * @brief       Read an attribute.
 *
 * @param       connHandle - connection message was received on
 * @param       pAttr - pointer to attribute
 * @param       pValue - pointer to data to be read
 * @param       pLen - length of data to be read
 * @param       offset - offset of the first octet to be read
 * @param       maxLen - maximum length of data to be read
 *
 * @return      Success or Failure
 */
static uint8 sk_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen )
{
  bStatus_t status = SUCCESS;
 
  // Make sure it's not a blob operation (no attributes in the profile are long
  if ( offset > 0 )
  {
    return ( ATT_ERR_ATTR_NOT_LONG );
  }
 
  if ( pAttr->type.len == ATT_BT_UUID_SIZE )
  {
    // 16-bit UUID
    uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);
    switch ( uuid )
    {
      // No need for "GATT_SERVICE_UUID" case;
      // gattserverapp handles this type for reads
      case GATT_CLIENT_CHAR_CFG_UUID:
        {
          uint16 value = sk_ReadCharCfg( connHandle, (gattCharCfg_t *)(pAttr->pValue) );
          *pLen = 2;
          pValue[0] = LO_UINT16( value );
          pValue[1] = HI_UINT16( value );
        }
        break;

      // simple keys characteristic does not have read permissions, but because it
      //   can be sent as a notification, it must be included here
      case SK_KEYPRESSED_UUID:
        *pLen = 1;
        pValue[0] = *pAttr->pValue;
        break;

      default:
        // Should never get here!
        *pLen = 0;
        status = ATT_ERR_ATTR_NOT_FOUND;
        break;
    }
  }
  else
  {
    // 128-bit UUID
    *pLen = 0;
    status = ATT_ERR_INVALID_HANDLE;
  }

  return ( status );
}

/*********************************************************************
 * @fn      sk_WriteAttrCB
 *
 * @brief   Validate attribute data prior to a write operation
 *
 * @param   connHandle - connection message was received on
 * @param   pAttr - pointer to attribute
 * @param   pValue - pointer to data to be written
 * @param   len - length of data
 * @param   offset - offset of the first octet to be written
 * @param   complete - whether this is the last packet
 * @param   oper - whether to validate and/or write attribute value  
 *
 * @return  Success or Failure
 */
static bStatus_t sk_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset )
{
  bStatus_t status = SUCCESS;

  if ( pAttr->type.len == ATT_BT_UUID_SIZE )
  {
    // 16-bit UUID
    uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);
    switch ( uuid )
    {
      case GATT_CLIENT_CHAR_CFG_UUID:
        // Validate the value
        // Make sure it's not a blob operation
        if ( offset == 0 )
        {
          if ( len == 2 )
          {
            uint16 charCfg = BUILD_UINT16( pValue[0], pValue[1] );
            
            // Validate characteristic configuration bit field
            if ( !( charCfg == GATT_CLIENT_CFG_NOTIFY   ||
                    charCfg == GATT_CFG_NO_OPERATION ) )
            {
              status = ATT_ERR_INVALID_VALUE;
            }
          }
          else
          {
            status = ATT_ERR_INVALID_VALUE_SIZE;
          }
        }
        else
        {
          status = ATT_ERR_ATTR_NOT_LONG;
        }
  
        // Write the value
        if ( status == SUCCESS )
        {
          uint16 value = BUILD_UINT16( pValue[0], pValue[1] );
          
          status = sk_WriteCharCfg( connHandle, (gattCharCfg_t *)(pAttr->pValue),
                                          value );

          if ( status == SUCCESS )
          {
            // Update Bond Manager
            VOID GAPBondMgr_UpdateCharCfg( connHandle, pAttr->handle, value );
          }

        }
        break;
       
      default:
        // Should never get here!
        status = ATT_ERR_ATTR_NOT_FOUND;
        break;
    }
  }
  else
  {
    // 128-bit UUID
    status = ATT_ERR_INVALID_HANDLE;
  }

  return ( status );
}

/*********************************************************************
 * @fn          sk_HandleConnStatusCB
 *
 * @brief       Simple Keys Profile link status change handler function.
 *
 * @param       connHandle - connection handle
 * @param       changeType - type of change
 *
 * @return      none
 */
static void sk_HandleConnStatusCB( uint16 connHandle, uint8 changeType )
{ 
  // Make sure this is not loopback connection
  if ( connHandle != LOOPBACK_CONNHANDLE )
  {
    // Reset Client Char Config if connection has dropped
    if ( ( changeType == LINKDB_STATUS_UPDATE_REMOVED )      ||
         ( ( changeType == LINKDB_STATUS_UPDATE_STATEFLAGS ) && 
           ( !linkDB_Up( connHandle ) ) ) )
    { 
      sk_ResetCharCfg( connHandle, skConfig );
    }
  }
}


/*********************************************************************
*********************************************************************/