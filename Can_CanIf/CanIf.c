/*
 * CanIf.c
 *
 *  Created on: Mar 15, 2020
 *      Author: Rita
 */

#include "CanIf.h"
#include "CanIf_Cbk.h"
#include "Dem.h"
#include "Det.h"
#include "MemMap.h"

/* TO BE ADDED */
// #include "PduR_CanIf.h"
// #include "PduR_Cbk.h"

CanIf_ConfigType CanIf_ConfigTypePtr = NULL_PTR;
CanIf_StateType CANIF_STATE = CANIF_UNINIT;

void CanIf_Init(const CanIf_ConfigType* ConfigPtr)
{
	#if (CANIF_PUBLIC_DEV_ERROR_DETECT == STD_ON)
		if(NULL_PTR == ConfigPtr)
		{
			Det_ReportError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CAN_INIT_SID, CANIF_E_PARAM_POINTER);
			return;
		}
	#endif
	/* Copy init config */
	CanIf_ConfigTypePtr = ConfigPtr;
	CANIF_STATE = CANIF_READY;
}

/* Requests transmission of a PDU */
Std_ReturnType CanIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
	#if (CANIF_PUBLIC_DEV_ERROR_DETECT == STD_ON)

		/* Check initialization status of CanIf */
		if (CANIF_STATE != CANIF_READY)
		{
			Det_ReportError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CANIF_TRANSMIT_SID, CANIF_E_UNINIT);
		}

		// IS THIS CORRECT?
		if (CanIf_ConfigTypePtr->canIfInitCfg->canIfTxPduCfg->CanIfTxPduTriggerTransmit != TRUE &&
				PduInfoPtr->SduDataPtr == NULL_PTR)
		{
			Det_ReportError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CANIF_TRANSMIT_SID, CANIF_E_PARAM_POINTER);
		}
		// ^^

		if (PduInfoPtr == NULL_PTR)
		{
			Det_ReportError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CANIF_TRANSMIT_SID, CANIF_E_PARAM_POINTER);
		}

		// IS THIS CORRECT?
		if (CanIf_ConfigTypePtr->canIfInitCfg->canIfTxPduCfg->CanIfTxPduId != TxPduId)
		{
			Det_ReportError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CANIF_TRANSMIT_SID, CANIF_E_INVALID_TXPDUID);
		}
		// ^^

		if (PduInfoPtr->SduLength > 8 && CanIf_ConfigTypePtr->canIfInitCfg->canIfTxPduCfg->canIfTxPduCanIdType
											== STANDARD_CAN)
		{
			Det_ReportRuntimeError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CANIF_TRANSMIT_SID,
					CANIF_E_DATA_LENGTH_MISMATCH);
			if(CanIf_ConfigTypePtr->canIfInitCfg->canIfTxPduCfg->CanIfTxPduTruncation == FALSE)
			{
				Det_ReportRuntimeError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CANIF_TRANSMIT_SID,
						CANIF_E_TXPDU_LENGTH_EXCEEDED);
				return E_NOT_OK;
			}
		}
	#endif

	/* Determine HTH for access to the CAN hardware transmit object */
	Can_HwHandleType Hth = CanIf_ConfigTypePtr->canIfInitCfg->canIfInitHohCfg->canIfHthCfg->
			CanIfHthIdSymRef->CanObjectId;

	/* Determine PDU to be sent on CAN BUS */
	Can_PduType Pdu;
	Pdu->swPduHandle = (uint8)CanIf_ConfigTypePtr->canIfInitCfg->canIfTxPduCfg->CanIfTxPduId;
	Pdu->length = PduInfoPtr->SduLength;
	Pdu->id = TxPduId;									// ?? CanId = TxPduId ?
	Pdu->sdu = PduInfoPtr->SduDataPtr;

	/* Call Can_Write API with the above parameters */
	Can_ReturnType CanWriteReturn;
	CanWriteReturn = Can_Write(Hth,&Pdu);
	if (CanWriteReturn == CAN_OK)
	{
		return E_OK;
	}
	else
	{
		return E_NOT_OK;
	}
}

void CanIf_TxConfirmation(PduIdType CanTxPduId)
{
	#if (CANIF_PUBLIC_DEV_ERROR_DETECT == STD_ON)
		if (CANIF_STATE != CANIF_READY)
		{
			Det_ReportError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CANIF_TXCONFIRMATION_SID, CANIF_E_UNINIT);
			return;
		}
		if(CanTxPduId != CanIf_ConfigTypePtr->canIfInitCfg->canIfTxPduCfg->CanIfTxPduId)
		{
			Det_ReportError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CANIF_TXCONFIRMATION_SID, CANIF_E_PARAM_LPDU);
		}
	#endif

	/* Confirmation to upper layer (PduR) */
	CanIfTxPduUserTxConfirmationName(CanTxPduId, E_OK);
}

void CanIf_RxIndication(const Can_HwType* Mailbox,const PduInfoType* PduInfoPtr)
{
	#if (CANIF_PUBLIC_DEV_ERROR_DETECT == STD_ON)
		if (CANIF_STATE != CANIF_READY)
		{
			Det_ReportError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CANIF_RXINDICATION_SID, CANIF_E_UNINIT);
			return;
		}
		if(Mailbox->Hoh != canHardwareReceiveObject->CanObjectId)
		{
			Det_ReportError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CANIF_RXINDICATION_SID, CANIF_E_PARAM_HOH);
		}
		if(Mailbox->CanId != ReceiveMailbox->CanId)
		{
			Det_ReportError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CANIF_RXINDICATION_SID, CANIF_E_PARAM_CANID);
		}
		if(Mailbox == NULL_PTR ||  PduInfoPtr == NULL_PTR)
		{
			Det_ReportError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CANIF_RXINDICATION_SID, CANIF_E_PARAM_POINTER);
		}
		if (PduInfoPtr->SduLength != CanIf_ConfigTypePtr->canIfInitCfg->canIfRxPduCfg->CanIfRxPduDataLength)
		{
			Det_ReportRuntimeError(CANIF_MODULE_ID, CANIF_INSTANCE_ID, CANIF_RXINDICATION_SID,
					CANIF_E_INVALID_DATA_LENGTH);
		}
	#endif
	PduIdType RxPduId;
	RxPduId = CanIf_ConfigTypePtr->canIfInitCfg->canIfRxPduCfg->CanIfRxPduId;
	/* Confirmation to upper layer (PduR) */
	CanIfRxPduUserRxIndicationName(RxPduId, PduInfoPtr);
}

/*
 * Note:
 * In CanIf_RxIndication API, we have only one RxPdu so we don't need to find its specific struct
 * Otherwise loop to find required Rx config
 * */
