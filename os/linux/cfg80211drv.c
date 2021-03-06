/****************************************************************************
 * Ralink Tech Inc.
 * Taiwan, R.O.C.
 *
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************/

/****************************************************************************

	Abstract:

	All related CFG80211 function body.

	History:

***************************************************************************/
#define RTMP_MODULE_OS


#ifdef RT_CFG80211_SUPPORT


#include "rt_config.h"


#define RT_CFG80211_DEBUG /* debug use */
#define CFG80211CB			(pAd->pCfg80211_CB)

#ifdef RT_CFG80211_DEBUG
#define CFG80211DBG(__Flg, __pMsg)		DBGPRINT(__Flg, __pMsg)
#else
#define CFG80211DBG(__Flg, __pMsg)
#endif /* RT_CFG80211_DEBUG */
extern INT RtmpIoctl_rt_ioctl_siwauth(
	IN      RTMP_ADAPTER                    *pAd,
	IN      VOID                            *pData,
	IN      ULONG                            Data);

BUILD_TIMER_FUNCTION(RemainOnChannelTimeout);
//DECLARE_TIMER_FUNCTION(RemainOnChannelTimeout);

VOID RemainOnChannelTimeout(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3)
{
	RTMP_ADAPTER *pAd = (RTMP_ADAPTER *) FunctionContext;
	DBGPRINT(RT_DEBUG_INFO, ("CFG80211_ROC: RemainOnChannelTimeout\n"));
	
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34))
	cfg80211_remain_on_channel_expired(pAd->net_dev, pAd->CfgChanInfo.cookie, pAd->CfgChanInfo.chan, pAd->CfgChanInfo.ChanType, GFP_KERNEL);
#endif
	pAd->Cfg80211RocTimerRunning = FALSE;
}

INT CFG80211DRV_IoctlHandle(
	IN	VOID					*pAdSrc,
	IN	RTMP_IOCTL_INPUT_STRUCT	*wrq,
	IN	INT						cmd,
	IN	USHORT					subcmd,
	IN	VOID					*pData,
	IN	ULONG					Data)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdSrc;


	switch(cmd)
	{
		case CMD_RTPRIV_IOCTL_80211_START:
		case CMD_RTPRIV_IOCTL_80211_END:
			/* nothing to do */
			break;

		case CMD_RTPRIV_IOCTL_80211_CB_GET:
			*(VOID **)pData = (VOID *)(pAd->pCfg80211_CB);
			break;

		case CMD_RTPRIV_IOCTL_80211_CB_SET:
			pAd->pCfg80211_CB = pData;
			break;

		case CMD_RTPRIV_IOCTL_80211_CHAN_SET:
			if (CFG80211DRV_OpsSetChannel(pAd, pData) != TRUE)
				return NDIS_STATUS_FAILURE;
			break;

		case CMD_RTPRIV_IOCTL_80211_VIF_CHG:
			if (CFG80211DRV_OpsChgVirtualInf(pAd, pData, Data) != TRUE)
				return NDIS_STATUS_FAILURE;
			break;

		case CMD_RTPRIV_IOCTL_80211_SCAN:
			if (CFG80211DRV_OpsScan(pAd) != TRUE) 
			{
				return NDIS_STATUS_FAILURE;
			}
			break;

		case CMD_RTPRIV_IOCTL_80211_IBSS_JOIN:
			CFG80211DRV_OpsJoinIbss(pAd, pData);
			break;

		case CMD_RTPRIV_IOCTL_80211_STA_LEAVE:
			CFG80211DRV_OpsLeave(pAd);
			break;

		case CMD_RTPRIV_IOCTL_80211_STA_GET:
			if (CFG80211DRV_StaGet(pAd, pData) != TRUE)
				return NDIS_STATUS_FAILURE;
			break;

		case CMD_RTPRIV_IOCTL_80211_STA_KEY_ADD:
			CFG80211DRV_StaKeyAdd(pAd, pData);
			break;

		case CMD_RTPRIV_IOCTL_80211_AP_KEY_ADD:
			CFG80211DRV_ApKeyAdd(pAd, pData);
			break;
		
		case CMD_RTPRIV_IOCTL_80211_AP_KEY_DEL:
			CFG80211DRV_ApKeyDel(pAd, pData);
			break;

		case CMD_RTPRIV_IOCTL_80211_KEY_DEFAULT_SET:
			CFG80211_setDefaultKey(pAd, Data);
			break;

		case CMD_RTPRIV_IOCTL_80211_CONNECT_TO:
			CFG80211DRV_Connect(pAd, pData);
			break;

#ifdef RFKILL_HW_SUPPORT
		case CMD_RTPRIV_IOCTL_80211_RFKILL:
		{
			UINT32 data = 0;
			BOOLEAN active;

			/* Read GPIO pin2 as Hardware controlled radio state */
			RTMP_IO_READ32(pAd, GPIO_CTRL_CFG, &data);
			active = !!(data & 0x04);

			if (!active)
			{
				RTMPSetLED(pAd, LED_RADIO_OFF);
				*(UINT8 *)pData = 0;
			}
			else
				*(UINT8 *)pData = 1;
		}
			break;
#endif /* RFKILL_HW_SUPPORT */

		case CMD_RTPRIV_IOCTL_80211_REG_NOTIFY_TO:
			CFG80211DRV_RegNotify(pAd, pData);
			break;

		case CMD_RTPRIV_IOCTL_80211_UNREGISTER:
			CFG80211_UnRegister(pAd, pData);
			break;

		case CMD_RTPRIV_IOCTL_80211_BANDINFO_GET:
		{
			CFG80211_BAND *pBandInfo = (CFG80211_BAND *)pData;
			CFG80211_BANDINFO_FILL(pAd, pBandInfo);
		}
			break;

		case CMD_RTPRIV_IOCTL_80211_SURVEY_GET:
			CFG80211DRV_SurveyGet(pAd, pData);
			break;
			
		case CMD_RTPRIV_IOCTL_80211_EXTRA_IES_SET:
			CFG80211DRV_OpsExtraIesSet(pAd);	
			break;

		case CMD_RTPRIV_IOCTL_80211_REMAIN_ON_CHAN_SET:			
			CFG80211DRV_OpsRemainOnChannel(pAd, pData, Data);			 		
			break;				
		case CMD_RTPRIV_IOCTL_80211_CANCEL_REMAIN_ON_CHAN_SET:
			CFG80211DRV_OpsCancelRemainOnChannel(pAd, Data);
			break;

		case CMD_RTPRIV_IOCTL_80211_MGMT_FRAME_REG:
			if (Data)
				pAd->Cfg80211ProbeReqCount++;
			else 
			{
				pAd->Cfg80211ProbeReqCount--;	
			}
	
			if (pAd->Cfg80211ProbeReqCount > 0)
				pAd->Cfg80211RegisterProbeReqFrame = TRUE;
			else 
				pAd->Cfg80211RegisterProbeReqFrame = FALSE;
			
			DBGPRINT(RT_DEBUG_TRACE, ("pAd->Cfg80211RegisterProbeReqFrame=%d[%d]\n",
					pAd->Cfg80211RegisterProbeReqFrame, pAd->Cfg80211ProbeReqCount));
			break;

		case CMD_RTPRIV_IOCTL_80211_ACTION_FRAME_REG:
			if (Data)
				pAd->Cfg80211ActionCount++;
			else
				pAd->Cfg80211ActionCount--;

			if (pAd->Cfg80211ActionCount > 0)
				pAd->Cfg80211RegisterActionFrame = TRUE;
			else
				pAd->Cfg80211RegisterActionFrame = FALSE;

			DBGPRINT(RT_DEBUG_TRACE, ("pAd->Cfg80211RegisterActionFrame=%d [%d]\n",
					pAd->Cfg80211RegisterActionFrame, pAd->Cfg80211ActionCount));
			break;

		case CMD_RTPRIV_IOCTL_80211_CHANNEL_LOCK:
			//pAd->CommonCfg.CentralChannel = Data;
			//DBGPRINT(RT_DEBUG_TRACE, ("CMD_RTPRIV_IOCTL_80211_CHANNEL_LOCK %d\n", Data));
			if (pAd->CommonCfg.Channel != Data)
			{
				pAd->CommonCfg.Channel= Data;
				AsicSwitchChannel(pAd, pAd->CommonCfg.Channel, FALSE);
				AsicLockChannel(pAd, pAd->CommonCfg.Channel);
			}
			break;

		case CMD_RTPRIV_IOCTL_80211_MGMT_FRAME_SEND:
			/* send a managment frame */
			pAd->TxStatusInUsed = TRUE;
			pAd->TxStatusSeq = pAd->Sequence;
			if (pData != NULL) 
			{
#ifdef WFD_SUPPORT
				if (pAd->StaCfg.WfdCfg.bSuppInsertWfdIe)
				{
					PP2P_PUBLIC_FRAME	pFrame = (PP2P_PUBLIC_FRAME)pData;
					ULONG	WfdIeLen = 0, WfdIeBitmap = 0;
				
					switch (pFrame->p80211Header.FC.SubType) 
					{
						case SUBTYPE_BEACON:
						case SUBTYPE_PROBE_REQ:
						case SUBTYPE_ASSOC_REQ:
						case SUBTYPE_REASSOC_REQ:
							WfdIeBitmap = (0x1 << SUBID_WFD_DEVICE_INFO) | (0x1 << SUBID_WFD_ASSOCIATED_BSSID) |
								(0x1 << SUBID_WFD_COUPLED_SINK_INFO);
							break;	

						case SUBTYPE_ASSOC_RSP:
						case SUBTYPE_REASSOC_RSP:
							WfdIeBitmap = (0x1 << SUBID_WFD_DEVICE_INFO) | (0x1 << SUBID_WFD_ASSOCIATED_BSSID) |
								(0x1 << SUBID_WFD_COUPLED_SINK_INFO) | (0x1 << SUBID_WFD_SESSION_INFO);
							break;	

						case SUBTYPE_PROBE_RSP:
							WfdIeBitmap = (0x1 << SUBID_WFD_DEVICE_INFO) | (0x1 << SUBID_WFD_ASSOCIATED_BSSID) |
								(0x1 << SUBID_WFD_COUPLED_SINK_INFO) | (0x1 << SUBID_WFD_SESSION_INFO);
							break;	

						case SUBTYPE_ACTION:
							if ((pFrame->Category == CATEGORY_PUBLIC) &&
								(pFrame->Action == ACTION_WIFI_DIRECT))
							{
								switch (pFrame->Subtype) 
								{
									case GO_NEGOCIATION_REQ:
									case GO_NEGOCIATION_RSP:
									case GO_NEGOCIATION_CONFIRM:
									case P2P_PROVISION_REQ:
										WfdIeBitmap = (0x1 << SUBID_WFD_DEVICE_INFO) | (0x1 << SUBID_WFD_ASSOCIATED_BSSID) |
											(0x1 << SUBID_WFD_COUPLED_SINK_INFO);
										break;
				
									case P2P_INVITE_REQ:
									case P2P_INVITE_RSP:
									case P2P_PROVISION_RSP:
										WfdIeBitmap = (0x1 << SUBID_WFD_DEVICE_INFO) | (0x1 << SUBID_WFD_ASSOCIATED_BSSID) |
											(0x1 << SUBID_WFD_COUPLED_SINK_INFO) | (0x1 << SUBID_WFD_SESSION_INFO);
										break;
								}
							}
							break;
					}
				
					if (WfdIeBitmap > 0)
					{
						PUCHAR		pOutBuffer;
						NDIS_STATUS   NStatus;
						
						NStatus = MlmeAllocateMemory(pAd, &pOutBuffer);  /* Get an unused nonpaged memory */
						if (NStatus != NDIS_STATUS_SUCCESS)
							DBGPRINT(RT_DEBUG_ERROR, ("%s: Allocate memory fail!!!\n", __FUNCTION__));
						else
						{
							memcpy(pOutBuffer, pData, Data);
							WfdMakeWfdIE(pAd, WfdIeBitmap, pOutBuffer + Data, &WfdIeLen);
							Data += WfdIeLen;
							
							if (pAd->pTxStatusBuf != NULL)
								os_free_mem(NULL, pAd->pTxStatusBuf);
							
							os_alloc_mem(NULL, (UCHAR **)&pAd->pTxStatusBuf, Data);
							if (pAd->pTxStatusBuf != NULL)
							{
								NdisCopyMemory(pAd->pTxStatusBuf, pOutBuffer, Data);
								pAd->TxStatusBufLen = Data;
							}
							else
							{
								DBGPRINT(RT_DEBUG_ERROR, ("CFG_TX_STATUS: MEM ALLOC ERROR\n"));
								MlmeFreeMemory(pAd, pOutBuffer);
								return NDIS_STATUS_FAILURE;
							}
							MiniportMMRequest(pAd, 0, pOutBuffer, Data);
						}
					}
				}
				else
#endif /* WFD_SUPPORT */
				{
					if (pAd->pTxStatusBuf != NULL)
						os_free_mem(NULL, pAd->pTxStatusBuf);
		
					os_alloc_mem(NULL, (UCHAR **)&pAd->pTxStatusBuf, Data);
					if (pAd->pTxStatusBuf != NULL)
					{
						NdisCopyMemory(pAd->pTxStatusBuf, pData, Data);
						pAd->TxStatusBufLen = Data;
					}
					else
					{
						DBGPRINT(RT_DEBUG_ERROR, ("CFG_TX_STATUS: MEM ALLOC ERROR\n"));
						return NDIS_STATUS_FAILURE;
					}

					MiniportMMRequest(pAd, 0, pData, Data);
				}
			}
			break;

		case CMD_RTPRIV_IOCTL_80211_REMAIN_ON_CHAN_DUR_TIMER_INIT:
			DBGPRINT(RT_DEBUG_TRACE, ("ROC TIMER INIT\n"));
			RTMPInitTimer(pAd, &pAd->Cfg80211RemainOnChannelDurationTimer, GET_TIMER_FUNCTION(RemainOnChannelTimeout), pAd, FALSE);
			break;

		case CMD_RTPRIV_IOCTL_80211_CHANNEL_LIST_SET:
			DBGPRINT(RT_DEBUG_INFO, ("CMD_RTPRIV_IOCTL_80211_CHANNEL_LIST_SET: %d\n", Data));
			UINT32 *pChanList = (UINT32 *) pData;

			if (pChanList != NULL) 
			{
			
				if (pAd->pCfg80211ChanList != NULL)
					os_free_mem(NULL, pAd->pCfg80211ChanList);

				os_alloc_mem(NULL, (UINT32 **)&pAd->pCfg80211ChanList, sizeof(UINT32 *) * Data);
				if (pAd->pCfg80211ChanList != NULL)
				{
					NdisCopyMemory(pAd->pCfg80211ChanList, pChanList, sizeof(UINT32 *) * Data);
					pAd->Cfg80211ChanListLan = Data;
				}
				else
				{
					return NDIS_STATUS_FAILURE;
				}
			}
			
			break;

		case CMD_RTPRIV_IOCTL_80211_BEACON_SET:
			CFG80211DRV_OpsBeaconSet(pAd, pData, 0);			
			break;
		
		case CMD_RTPRIV_IOCTL_80211_BEACON_ADD:
			CFG80211DRV_OpsBeaconSet(pAd, pData, 1);
			break;
			
		case CMD_RTPRIV_IOCTL_80211_BEACON_DEL:
#ifdef WFD_SUPPORT
			pAd->StaCfg.WfdCfg.bSuppGoOn = FALSE;
#endif /* WFD_SUPPORT */
			break;

		case CMD_RTPRIV_IOCTL_80211_CHANGE_BSS_PARM:
			CFG80211DRV_OpsChangeBssParm(pAd, pData);
			break;

		case CMD_RTPRIV_IOCTL_80211_AP_PROBE_RSP:
			break;

		case CMD_RTPRIV_IOCTL_80211_PORT_SECURED:
			CFG80211_StaPortSecured(pAd, pData, Data);
			break;

		case CMD_RTPRIV_IOCTL_80211_AP_STA_DEL:
			CFG80211_ApStaDel(pAd, pData);
			break;
		case CMD_RTPRIV_IOCTL_80211_BITRATE_SET:
		//	pAd->CommonCfg.PhyMode = PHY_11AN_MIXED;
		//	RTMPSetPhyMode(pAd,  pAd->CommonCfg.PhyMode);
			//Set_WirelessMode_Proc(pAd, PHY_11AGN_MIXED);
			break;
        case CMD_RTPRIV_IOCTL_80211_RESET:
            CFG80211_reSetToDefault(pAd);
            break;
		default:
			return NDIS_STATUS_FAILURE;
	}

	return NDIS_STATUS_SUCCESS;
}

static VOID CFG80211DRV_DisableApInterface(
        VOID                                            *pAdOrg)
{
#ifdef CONFIG_AP_SUPPORT
	UINT32 Value;
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	pAd->ApCfg.MBSSID[MAIN_MBSSID].bBcnSntReq = FALSE;
  
	if (pAd->CommonCfg.BBPCurrentBW == BW_40)
	{
   		pAd->CommonCfg.BBPCurrentBW = BW_20; // skynien follow YF's suggestion add here , for GO/p2p switch 
   		UCHAR ByteValue = 0;	
   		RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R4, &ByteValue);
   		ByteValue &= (~0x18);
   		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R4, ByteValue);
   	}

        /* Disable pre-tbtt interrupt */
        RTMP_IO_READ32(pAd, INT_TIMER_EN, &Value);
        Value &=0xe;
        RTMP_IO_WRITE32(pAd, INT_TIMER_EN, Value);

        if (!INFRA_ON(pAd))
        {
                /* Disable piggyback */
                RTMPSetPiggyBack(pAd, FALSE);
                AsicUpdateProtect(pAd, 0,  (ALLN_SETPROTECT|CCKSETPROTECT|OFDMSETPROTECT), TRUE, FALSE);

        }

        if (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST))
        {
                /*RTMP_ASIC_INTERRUPT_DISABLE(pAd); */
                AsicDisableSync(pAd);

#ifdef LED_CONTROL_SUPPORT
                /* Set LED */
                RTMPSetLED(pAd, LED_LINK_DOWN);
#endif /* LED_CONTROL_SUPPORT */
        }

#ifdef RTMP_MAC_USB
        /* For RT2870, we need to clear the beacon sync buffer. */
        RTUSBBssBeaconExit(pAd);
#endif /* RTMP_MAC_USB */
#endif
	
}


static VOID CFG80211DRV_DisableScanAction(
        VOID                                            *pAdOrg)
{
        PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
        BOOLEAN Cancelled;

        /* Stop Scan. */
        if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS))
        {
                RTMPCancelTimer(&pAd->MlmeAux.ScanTimer, &Cancelled);
                pAd->MlmeAux.Channel = 0;
                ScanNextChannel(pAd, OPMODE_STA);
        }


        pAd->FlgCfg80211Scanning = FALSE;
}

VOID CFG80211DRV_OpsChangeBssParm(
        VOID                                            *pAdOrg,
        VOID                                            *pData)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	CMD_RTPRIV_IOCTL_80211_BSS_PARM *pBssInfo;
	BOOLEAN TxPreamble;

	CFG80211DBG(RT_DEBUG_TRACE, ("%s\n", __FUNCTION__));

	pBssInfo = (CMD_RTPRIV_IOCTL_80211_BSS_PARM *)pData;	

	/* Short Preamble */
	if (pBssInfo->use_short_preamble != -1)
	{
		CFG80211DBG(RT_DEBUG_TRACE, ("%s: ShortPreamble %d\n", __FUNCTION__, pBssInfo->use_short_preamble));
        	pAd->CommonCfg.TxPreamble = (pBssInfo->use_short_preamble == 0 ? Rt802_11PreambleLong : Rt802_11PreambleShort);	
		TxPreamble = (pAd->CommonCfg.TxPreamble == Rt802_11PreambleLong ? 0 : 1);
		MlmeSetTxPreamble(pAd, (USHORT)pAd->CommonCfg.TxPreamble);			
	}
	
	/* CTS Protection */
	if (pBssInfo->use_cts_prot != -1)
	{
		CFG80211DBG(RT_DEBUG_TRACE, ("%s: CTS Protection %d\n", __FUNCTION__, pBssInfo->use_cts_prot));
	}
	
	/* Short Slot */
	if (pBssInfo->use_short_slot_time != -1)
	{
		CFG80211DBG(RT_DEBUG_TRACE, ("%s: Short Slot %d\n", __FUNCTION__, pBssInfo->use_short_slot_time));
	}
}

BOOLEAN CFG80211DRV_OpsSetChannel(
	VOID						*pAdOrg,
	VOID						*pData)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	CMD_RTPRIV_IOCTL_80211_CHAN *pChan;
	UINT8 ChanId;
	UINT8 IfType;
	UINT8 ChannelType;
	STRING ChStr[5] = { 0 };
#ifdef DOT11_N_SUPPORT
	BOOLEAN FlgIsChanged;
#endif /* DOT11_N_SUPPORT */

/*
 *  enum nl80211_channel_type {
 *	NL80211_CHAN_NO_HT,
 *	NL80211_CHAN_HT20,
 *	NL80211_CHAN_HT40MINUS,
 *	NL80211_CHAN_HT40PLUS
 *  };
 */
	/* init */
	pChan = (CMD_RTPRIV_IOCTL_80211_CHAN *)pData;
	ChanId = pChan->ChanId;
	IfType = pChan->IfType;
	ChannelType = pChan->ChanType;

#ifdef DOT11_N_SUPPORT
	if (IfType != RT_CMD_80211_IFTYPE_MONITOR)
	{
		/* get channel BW */
		FlgIsChanged = TRUE;
	
		/* set to new channel BW */
		if (ChannelType == RT_CMD_80211_CHANTYPE_HT20)
		{
			pAd->CommonCfg.RegTransmitSetting.field.BW = BW_20;
			pAd->CommonCfg.HT_Disable = 0;
		}
		else if ((ChannelType == RT_CMD_80211_CHANTYPE_HT40MINUS) ||
				(ChannelType == RT_CMD_80211_CHANTYPE_HT40PLUS))
		{
			/* not support NL80211_CHAN_HT40MINUS or NL80211_CHAN_HT40PLUS */
			/* i.e. primary channel = 36, secondary channel must be 40 */
			pAd->CommonCfg.RegTransmitSetting.field.BW = BW_40;
			pAd->CommonCfg.HT_Disable = 0;
		}
		else if  (ChannelType == RT_CMD_80211_CHANTYPE_NOHT)
		{
			pAd->CommonCfg.RegTransmitSetting.field.BW = BW_20;
			pAd->CommonCfg.HT_Disable = 1;	
		}
		
		CFG80211DBG(RT_DEBUG_TRACE, ("80211> New BW = %d\n", pAd->CommonCfg.RegTransmitSetting.field.BW));
		CFG80211DBG(RT_DEBUG_TRACE, ("80211> HT Disable = %d\n", pAd->CommonCfg.HT_Disable));	
	}
	else
	{
		/* for monitor mode */
		FlgIsChanged = TRUE;
		pAd->CommonCfg.HT_Disable = 0;
		pAd->CommonCfg.RegTransmitSetting.field.BW = BW_40;
	} /* End of if */

	if (FlgIsChanged == TRUE)
		SetCommonHT(pAd);
	/* End of if */
#endif /* DOT11_N_SUPPORT */

	/* switch to the channel with Common Channel */
	sprintf(ChStr, "%d", ChanId);
	if (Set_Channel_Proc(pAd, ChStr) == FALSE)
	{
		CFG80211DBG(RT_DEBUG_ERROR, ("80211> Change channel fail!\n"));
	} /* End of if */
	else
	{
		if (pAd->LatchRfRegs.Channel != pAd->CommonCfg.Channel)
		{
			AsicSwitchChannel(pAd, pAd->CommonCfg.Channel, FALSE);
			AsicLockChannel(pAd, pAd->CommonCfg.Channel);	
		}	
	}
	
	CFG80211_CB *p80211CB = pAd->pCfg80211_CB;

	//if (p80211CB->pCfg80211_Wdev->iftype == RT_CMD_80211_IFTYPE_AP)
	if(pAd->VifNextMode == RT_CMD_80211_IFTYPE_AP)
	{
		p80211CB->pCfg80211_Wdev->iftype = RT_CMD_80211_IFTYPE_AP;
		pAd->OpMode = OPMODE_AP;
		CFG80211DBG(RT_DEBUG_TRACE, ("80211> Set the channel in AP Mode\n"));

		return TRUE;
	}
#ifdef CONFIG_STA_SUPPORT
#ifdef DOT11_N_SUPPORT
	if ((IfType == RT_CMD_80211_IFTYPE_STATION) && (FlgIsChanged == TRUE))
	{
		/*
			1. Station mode;
			2. New BW settings is 20MHz but current BW is not 20MHz;
			3. New BW settings is 40MHz but current BW is 20MHz;

			Re-connect to the AP due to BW 20/40 or HT/non-HT change.
		*/
		// Set_SSID_Proc(pAd, (PSTRING)pAd->CommonCfg.Ssid);
	} /* End of if */
#endif /* DOT11_N_SUPPORT */

	if (IfType == RT_CMD_80211_IFTYPE_ADHOC)
	{
		/* update IBSS beacon */
		MlmeUpdateTxRates(pAd, FALSE, 0);
		MakeIbssBeacon(pAd);
		AsicEnableIbssSync(pAd);

		Set_SSID_Proc(pAd, (PSTRING)pAd->CommonCfg.Ssid);
	} /* End of if */

	if (IfType == RT_CMD_80211_IFTYPE_MONITOR)
	{
		/* reset monitor mode in the new channel */
		Set_NetworkType_Proc(pAd, "Monitor");
		RTMP_IO_WRITE32(pAd, RX_FILTR_CFG, pChan->MonFilterFlag);
	} /* End of if */
#endif /* CONFIG_STA_SUPPORT */

	return TRUE;
}


BOOLEAN CFG80211DRV_OpsChgVirtualInf(
	VOID						*pAdOrg,
	VOID						*pFlgFilter,
	UINT8						IfType)
{
#ifdef CONFIG_STA_SUPPORT
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	UINT32 FlgFilter = *(UINT32 *)pFlgFilter;

        CFG80211_CB *p80211CB = pAd->pCfg80211_CB;

	/* change type */
	if (IfType == RT_CMD_80211_IFTYPE_ADHOC)
		Set_NetworkType_Proc(pAd, "Adhoc");
	else if (IfType == RT_CMD_80211_IFTYPE_STATION)
	{
		//Set_NetworkType_Proc(pAd, "Infra");
		if (pAd->VifNextMode == RT_CMD_80211_IFTYPE_AP)
			CFG80211DRV_DisableApInterface(pAd);
			
		pAd->VifNextMode = RT_CMD_80211_IFTYPE_STATION;
		p80211CB->pCfg80211_Wdev->iftype = RT_CMD_80211_IFTYPE_STATION;
		pAd->OpMode = OPMODE_STA;
	}
	else if (IfType == RT_CMD_80211_IFTYPE_MONITOR)
	{
		/* set packet filter */
		Set_NetworkType_Proc(pAd, "Monitor");

		if (FlgFilter != 0)
		{
			UINT32 Filter;

			RTMP_IO_READ32(pAd, RX_FILTR_CFG, &Filter);

			if ((FlgFilter & RT_CMD_80211_FILTER_FCSFAIL) == \
												RT_CMD_80211_FILTER_FCSFAIL)
			{
				Filter = Filter & (~0x01);
			}
			else
				Filter = Filter | 0x01;
			/* End of if */
	
			if ((FlgFilter & RT_CMD_80211_FILTER_PLCPFAIL) == \
												RT_CMD_80211_FILTER_PLCPFAIL)
			{
				Filter = Filter & (~0x02);
			}
			else
				Filter = Filter | 0x02;
			/* End of if */
	
			if ((FlgFilter & RT_CMD_80211_FILTER_CONTROL) == \
												RT_CMD_80211_FILTER_CONTROL)
			{
				Filter = Filter & (~0xFF00);
			}
			else
				Filter = Filter | 0xFF00;
			/* End of if */
	
			if ((FlgFilter & RT_CMD_80211_FILTER_OTHER_BSS) == \
												RT_CMD_80211_FILTER_OTHER_BSS)
			{
				Filter = Filter & (~0x08);
			}
			else
				Filter = Filter | 0x08;
			/* End of if */

			RTMP_IO_WRITE32(pAd, RX_FILTR_CFG, Filter);
			*(UINT32 *)pFlgFilter = Filter;
		} /* End of if */

		return TRUE; /* not need to set SSID */
	} /* End of if */
	else if (IfType == RT_CMD_80211_IFTYPE_AP )	
	{
		CFG80211DBG(RT_DEBUG_TRACE, ("80211> Change the Interface to AP Mode\n"));		
		pAd->VifNextMode = RT_CMD_80211_IFTYPE_AP;
                //p80211CB->pCfg80211_Wdev->iftype = RT_CMD_80211_IFTYPE_AP;
                //pAd->OpMode = OPMODE_AP;

		return TRUE;	
	}/* End of if */

	return TRUE;

	//pAd->StaCfg.bAutoReconnect = TRUE;

	//CFG80211DBG(RT_DEBUG_ERROR, ("80211> SSID = %s\n", pAd->CommonCfg.Ssid));
	//Set_SSID_Proc(pAd, (PSTRING)pAd->CommonCfg.Ssid);
#endif /* CONFIG_STA_SUPPORT */

	return TRUE;
}


BOOLEAN CFG80211DRV_OpsScan(
	VOID						*pAdOrg)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
        CFG80211DBG(RT_DEBUG_TRACE, ("80211> CFG80211DRV_OpsScan ==> \n")); 

	if (pAd->FlgCfg80211Scanning == TRUE || 
		RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS))
	 {
		CFG80211DBG(RT_DEBUG_ERROR, ("pAd->FlgCfg80211Scanning == %d\n", 
				pAd->FlgCfg80211Scanning)); 	
		return FALSE; /* scanning */
	}	

	/* do scan */
	pAd->FlgCfg80211Scanning = TRUE;
	return TRUE;
}

static int CFG80211DRV_UpdateTimIE(
	VOID	*pAdOrg,
	PUCHAR 	pBeaconFrame,
	UINT32	tim_ie_pos)
{
#ifdef CONFIG_AP_SUPPORT
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	UCHAR  ID_1B, TimFirst, TimLast, *pTim, *ptr, New_Tim_Len;
	UINT  i;
		
	ptr = pBeaconFrame + tim_ie_pos; /* TIM LOCATION */
	*ptr = IE_TIM;
	*(ptr + 2) = pAd->ApCfg.DtimCount;
	*(ptr + 3) = pAd->ApCfg.DtimPeriod;	
	
	TimFirst = 0; /* record first TIM byte != 0x00 */
	TimLast = 0;  /* record last  TIM byte != 0x00 */
	
	pTim = pAd->ApCfg.MBSSID[MAIN_MBSSID].TimBitmaps;	
	
	for(ID_1B=0; ID_1B<WLAN_MAX_NUM_OF_TIM; ID_1B++)
	{
		/* get the TIM indicating PS packets for 8 stations */
		UCHAR tim_1B = pTim[ID_1B];

		if (ID_1B == 0)
			tim_1B &= 0xfe; /* skip bit0 bc/mc */

		if (tim_1B == 0)
			continue; /* find next 1B */

		if (TimFirst == 0)
			TimFirst = ID_1B;
			
		TimLast = ID_1B;
	} 
	
	/* fill TIM content to beacon buffer */
	if (TimFirst & 0x01)
		TimFirst --; /* find the even offset byte */		

	*(ptr + 1) = 3 + (TimLast - TimFirst + 1); /* TIM IE length */
	*(ptr + 4) = TimFirst;

	for(i=TimFirst; i<=TimLast; i++)
		*(ptr + 5 + i - TimFirst) = pTim[i];

	/* bit0 means backlogged mcast/bcast */
    	if (pAd->ApCfg.DtimCount == 0)
		*(ptr + 4) |= (pAd->ApCfg.MBSSID[MAIN_MBSSID].TimBitmaps[WLAN_CT_TIM_BCMC_OFFSET] & 0x01); 

	/* adjust BEACON length according to the new TIM */
	New_Tim_Len = (2 + *(ptr+1));
	
	return New_Tim_Len;
#else
	return 0;
#endif
}	

VOID CFG80211_UpdateBeacon(
	VOID		*pAdOrg,
	UCHAR 		*beacon_head_buf,
	UINT32		beacon_head_len,
	UCHAR 		*beacon_tail_buf,
	UINT32		beacon_tail_len,
	BOOLEAN		isAllUpdate)
{
#ifdef CONFIG_AP_SUPPORT
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	HTTRANSMIT_SETTING BeaconTransmit;   /* MGMT frame PHY rate setting when operatin at Ht rate. */
	TXWI_STRUC *pTxWI = &pAd->BeaconTxWI;
	UINT8 TXWISize = pAd->chipCap.TXWISize;
	UCHAR  *ptr, New_Tim_Len;
	UINT  i;
	UINT32 longValue, beacon_len;
	PUCHAR pBeaconFrame = (PUCHAR)pAd->ApCfg.MBSSID[MAIN_MBSSID].BeaconBuf;

	if (isAllUpdate) /* Invoke From CFG80211 OPS For setting Beacon buffer */
	{
		/* 1. Update the Before TIM IE */
		NdisCopyMemory(pBeaconFrame, beacon_head_buf, beacon_head_len);
		
		/* 2. Update the TIM IE */
		pAd->ApCfg.MBSSID[MAIN_MBSSID].TimIELocationInBeacon = beacon_head_len;
		
		/* 3. Store the Tail Part For appending later */
		if (pAd->beacon_tail_buf != NULL)
			 os_free_mem(NULL, pAd->beacon_tail_buf);
		
		os_alloc_mem(NULL, (UCHAR **)&pAd->beacon_tail_buf, beacon_tail_len);
		if (pAd->beacon_tail_buf != NULL)
		{
			NdisCopyMemory(pAd->beacon_tail_buf, beacon_tail_buf, beacon_tail_len);
			pAd->beacon_tail_len = beacon_tail_len;
		}		
		else
		{
			DBGPRINT(RT_DEBUG_ERROR, ("CFG80211 Beacon: MEM ALLOC ERROR\n"));
		}

		return;  	
	}
	else /* Invoke From Beacon Timer */
	{		
		if (pAd->ApCfg.DtimCount == 0)
			pAd->ApCfg.DtimCount = pAd->ApCfg.DtimPeriod - 1;
		else
			pAd->ApCfg.DtimCount -= 1;

		/* 1. Update the TIM IE */
		New_Tim_Len = CFG80211DRV_UpdateTimIE(pAd, pBeaconFrame, pAd->ApCfg.MBSSID[MAIN_MBSSID].TimIELocationInBeacon);
		
		/* 2. Update the AFTER TIM IE */
        if (pAd->beacon_tail_buf != NULL)
        {
		NdisCopyMemory(pAd->ApCfg.MBSSID[MAIN_MBSSID].BeaconBuf + pAd->ApCfg.MBSSID[MAIN_MBSSID].TimIELocationInBeacon + New_Tim_Len,
						pAd->beacon_tail_buf, pAd->beacon_tail_len);
		beacon_len = pAd->ApCfg.MBSSID[MAIN_MBSSID].TimIELocationInBeacon + pAd->beacon_tail_len + New_Tim_Len;
		}
		else
		{
				 printk("YF_BEACON ====> CFG80211_UpdateBeacon OOPS\n");
				 return;
		}	
	}
	
    	BeaconTransmit.word = 0;
	/* Should be Find the P2P IE Then Set Basic Rate to 6M */	
	BeaconTransmit.field.MODE = MODE_OFDM; /* Use 6Mbps *///Carter
	BeaconTransmit.field.MCS = MCS_RATE_6;//Carter

    	RTMPWriteTxWI(pAd, pTxWI, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE, 0, BSS0Mcast_WCID,
                beacon_len, PID_MGMT, 0, 0, IFS_HTTXOP, FALSE, &BeaconTransmit);

			
	//RT28xx_UpdateBeaconToAsic(pAd, MAIN_MBSSID, beacon_len, pAd->ApCfg.MBSSID[MAIN_MBSSID].TimIELocationInBeacon);		

	ptr = (PUCHAR)&pAd->BeaconTxWI;
#ifdef RT_BIG_ENDIAN
	RTMPWIEndianChange(ptr, TYPE_TXWI);
#endif

	for (i=0; i<TXWISize; i+=4)  /* 16-byte TXWI field */
	{
		longValue =  *ptr + (*(ptr+1)<<8) + (*(ptr+2)<<16) + (*(ptr+3)<<24);
		RTMP_IO_WRITE32(pAd, pAd->BeaconOffset[0] + i, longValue);
		ptr += 4;
	}

	/* update BEACON frame content. start right after the 16-byte TXWI field. */
	ptr = pAd->ApCfg.MBSSID[MAIN_MBSSID].BeaconBuf;
#ifdef RT_BIG_ENDIAN
	RTMPFrameEndianChange(pAd, ptr, DIR_WRITE, FALSE);
#endif

	for (i= 0; i< beacon_len; i+=4)
	{
		longValue =  *ptr + (*(ptr+1)<<8) + (*(ptr+2)<<16) + (*(ptr+3)<<24);
		RTMP_IO_WRITE32(pAd, pAd->BeaconOffset[0] + TXWISize + i, longValue);
		ptr += 4;
	}

	BEACON_SYNC_STRUCT	*pBeaconSync = pAd->CommonCfg.pBeaconSync;

	if (pBeaconSync == NULL)
	{
			CFG80211DBG(RT_DEBUG_ERROR, ("80211> UpdateBeacon - pBeaconSync == NULL\n"));
			return;
	}
	
	ptr = (PUCHAR) (pAd->ApCfg.MBSSID[MAIN_MBSSID].BeaconBuf + pAd->ApCfg.MBSSID[MAIN_MBSSID].TimIELocationInBeacon);
	if ((*(ptr + 4)) & 0x01)
		pBeaconSync->DtimBitOn |= (1 << MAIN_MBSSID);
	else
		pBeaconSync->DtimBitOn &= ~(1 << MAIN_MBSSID);
#endif

}

/* REF: ap_connect.c ApMakeBssBeacon */
BOOLEAN CFG80211DRV_OpsBeaconSet(
    VOID                                            *pAdOrg,
    VOID                                            *pData,
	BOOLEAN                                          isAdd)
{

#ifdef CONFIG_AP_SUPPORT
	CFG80211DBG(RT_DEBUG_TRACE, ("80211> CFG80211DRV_OpsBeaconSet ==> %d\n", isAdd));
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	CMD_RTPRIV_IOCTL_80211_BEACON *pBeacon;
	UINT32 rx_filter_flag;
	BOOLEAN TxPreamble, SpectrumMgmt = FALSE;
	BOOLEAN	bWmmCapable = FALSE;
	UCHAR	BBPR1 = 0, BBPR3 = 0;
	UINT  i;
	INT idx;
	ULONG offset;
	BCN_TIME_CFG_STRUC csr9;

	pBeacon = (CMD_RTPRIV_IOCTL_80211_BEACON *)pData;

#ifdef WFD_SUPPORT
	if (pAd->StaCfg.WfdCfg.bSuppInsertWfdIe)
	{
		ULONG TmpLen, WfdIeBitmap;

		ptr = pBeacon->beacon_tail + pBeacon->beacon_tail_len;
		WfdIeBitmap = (0x1 << SUBID_WFD_DEVICE_INFO) | (0x1 << SUBID_WFD_ASSOCIATED_BSSID) |
			(0x1 << SUBID_WFD_COUPLED_SINK_INFO);
		WfdMakeWfdIE(pAd, WfdIeBitmap, ptr, &TmpLen);
		pBeacon->beacon_tail_len += TmpLen;
	}
#endif /* WFD_SUPPORT */

	if (isAdd)
	{
		rx_filter_flag = APNORMAL;
		RTMP_IO_WRITE32(pAd, RX_FILTR_CFG, rx_filter_flag);     /* enable RX of DMA block */
	
		pAd->ApCfg.BssidNum = 1;
		pAd->MacTab.MsduLifeTime = 20; /* default 5 seconds */
		pAd->ApCfg.MBSSID[MAIN_MBSSID].bBcnSntReq = TRUE;

                /* For GO Timeout */
                pAd->ApCfg.StaIdleTimeout = 30;
                pAd->ApCfg.MBSSID[MAIN_MBSSID].StationKeepAliveTime = 10;

		AsicDisableSync(pAd);

		if (pAd->CommonCfg.PhyMode >= PHY_11ABGN_MIXED)
		{
			if (pAd->CommonCfg.Channel > 14)
				pAd->ApCfg.MBSSID[MAIN_MBSSID].PhyMode = PHY_11AN_MIXED;
			else
				pAd->ApCfg.MBSSID[MAIN_MBSSID].PhyMode = PHY_11BGN_MIXED;
		}
		else
		{
			if (pAd->CommonCfg.Channel > 14)
				pAd->ApCfg.MBSSID[MAIN_MBSSID].PhyMode = PHY_11A;
			else
				pAd->ApCfg.MBSSID[MAIN_MBSSID].PhyMode = PHY_11BG_MIXED;
		}

		TxPreamble = (pAd->CommonCfg.TxPreamble == Rt802_11PreambleLong ? 0 : 1);	
	}

	PMULTISSID_STRUCT pMbss = &pAd->ApCfg.MBSSID[MAIN_MBSSID];

	const UCHAR *ssid_ie = NULL;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
	ssid_ie = cfg80211_find_ie(WLAN_EID_SSID, pBeacon->beacon_head+36, pBeacon->beacon_head_len-36);
#endif
	NdisZeroMemory(pMbss->Ssid, pMbss->SsidLen);
	if (ssid_ie == NULL) 
	{
		DBGPRINT(RT_DEBUG_ERROR,("CFG: SSID Not Found In Packet\n"));
		NdisMoveMemory(pMbss->Ssid, "P2P_Linux_AP", 12);
		pMbss->SsidLen = 12;
	}
	else
	{
		pMbss->SsidLen = ssid_ie[1];
		NdisCopyMemory(pMbss->Ssid, ssid_ie+2, pMbss->SsidLen);
		DBGPRINT(RT_DEBUG_TRACE,("CFG : SSID: %s, %d\n", pMbss->Ssid, pMbss->SsidLen));
	}
	
	if (isAdd)
	{
		//if (pMbss->bWmmCapable)
		//{
        	bWmmCapable = FALSE;
			pMbss->bWmmCapable = FALSE;
		//}

		pMbss->MSSIDDev = pAd->net_dev;
		COPY_MAC_ADDR(pMbss->Bssid, pAd->CurrentAddress);
		DBGPRINT(RT_DEBUG_TRACE,("AP BSSID %02x:%02x:%02x:%02x:%02x:%02x\n", PRINT_MAC(pAd->CurrentAddress)));
		
		/* GO always use WPA2PSK / AES */
		pMbss->AuthMode = Ndis802_11AuthModeWPA2PSK;
 		pMbss->WepStatus = Ndis802_11Encryption3Enabled;
		pMbss->WscSecurityMode = WPA2PSKAES;
		pMbss->GroupKeyWepStatus = pMbss->WepStatus;
		pMbss->CapabilityInfo =
			CAP_GENERATE(1, 0, (pMbss->WepStatus != Ndis802_11EncryptionDisabled), TxPreamble, pAd->CommonCfg.bUseShortSlotTime, SpectrumMgmt);

		RTMPMakeRSNIE(pAd, Ndis802_11AuthModeWPA2PSK, Ndis802_11Encryption3Enabled, MAIN_MBSSID);

		/* Disable Driver-Internal Rekey */
		pMbss->WPAREKEY.ReKeyInterval = 0;
		pMbss->WPAREKEY.ReKeyMethod = DISABLE_REKEY;

#ifdef DOT11_N_SUPPORT
		RTMPSetPhyMode(pAd,  pAd->CommonCfg.PhyMode);
		SetCommonHT(pAd);

		if ((pAd->CommonCfg.PhyMode >= PHY_11ABGN_MIXED) && (pAd->Antenna.field.TxPath == 2))
		{
			RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R1, &BBPR1);
			BBPR1 &= (~0x18);
			BBPR1 |= 0x10;
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R1, BBPR1);
		}
		else
#endif /* DOT11_N_SUPPORT */
		{
			RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R1, &BBPR1);
			BBPR1 &= (~0x18);
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R1, BBPR1);
		}
	
		/* Receiver Antenna selection, write to BBP R3(bit4:3) */
		RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R3, &BBPR3);
		BBPR3 &= (~0x18);
		if(pAd->Antenna.field.RxPath == 3)
		{
			BBPR3 |= (0x10);
		}
		else if(pAd->Antenna.field.RxPath == 2)
		{
			BBPR3 |= (0x8);
		}
		else if(pAd->Antenna.field.RxPath == 1)
		{
			BBPR3 |= (0x0);
		}
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R3, BBPR3);

		if(!OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED))
		{
			if ((pAd->CommonCfg.PhyMode > PHY_11G) || bWmmCapable)
			{
				/* EDCA parameters used for AP's own transmission */
				pAd->CommonCfg.APEdcaParm.bValid = TRUE;
				pAd->CommonCfg.APEdcaParm.Aifsn[0] = 3;
				pAd->CommonCfg.APEdcaParm.Aifsn[1] = 7;
				pAd->CommonCfg.APEdcaParm.Aifsn[2] = 1;
				pAd->CommonCfg.APEdcaParm.Aifsn[3] = 1;

				pAd->CommonCfg.APEdcaParm.Cwmin[0] = 4;
				pAd->CommonCfg.APEdcaParm.Cwmin[1] = 4;
				pAd->CommonCfg.APEdcaParm.Cwmin[2] = 3;
				pAd->CommonCfg.APEdcaParm.Cwmin[3] = 2;

				pAd->CommonCfg.APEdcaParm.Cwmax[0] = 6;
				pAd->CommonCfg.APEdcaParm.Cwmax[1] = 10;
				pAd->CommonCfg.APEdcaParm.Cwmax[2] = 4;
				pAd->CommonCfg.APEdcaParm.Cwmax[3] = 3;

				pAd->CommonCfg.APEdcaParm.Txop[0]  = 0;
				pAd->CommonCfg.APEdcaParm.Txop[1]  = 0;
				pAd->CommonCfg.APEdcaParm.Txop[2]  = 94;	/*96; */
				pAd->CommonCfg.APEdcaParm.Txop[3]  = 47;	/*48; */
				AsicSetEdcaParm(pAd, &pAd->CommonCfg.APEdcaParm);

				/* EDCA parameters to be annouced in outgoing BEACON, used by WMM STA */
				pAd->ApCfg.BssEdcaParm.bValid = TRUE;
				pAd->ApCfg.BssEdcaParm.Aifsn[0] = 3;
				pAd->ApCfg.BssEdcaParm.Aifsn[1] = 7;
				pAd->ApCfg.BssEdcaParm.Aifsn[2] = 2;
				pAd->ApCfg.BssEdcaParm.Aifsn[3] = 2;

				pAd->ApCfg.BssEdcaParm.Cwmin[0] = 4;
				pAd->ApCfg.BssEdcaParm.Cwmin[1] = 4;
				pAd->ApCfg.BssEdcaParm.Cwmin[2] = 3;
				pAd->ApCfg.BssEdcaParm.Cwmin[3] = 2;

				pAd->ApCfg.BssEdcaParm.Cwmax[0] = 10;
				pAd->ApCfg.BssEdcaParm.Cwmax[1] = 10;
				pAd->ApCfg.BssEdcaParm.Cwmax[2] = 4;
				pAd->ApCfg.BssEdcaParm.Cwmax[3] = 3;
	
				pAd->ApCfg.BssEdcaParm.Txop[0]  = 0;
				pAd->ApCfg.BssEdcaParm.Txop[1]  = 0;
				pAd->ApCfg.BssEdcaParm.Txop[2]  = 94;	/*96; */
				pAd->ApCfg.BssEdcaParm.Txop[3]  = 47;	/*48; */
			}
			else
			{
				AsicSetEdcaParm(pAd, NULL);
			}
		}

#ifdef DOT11_N_SUPPORT
		if (pAd->CommonCfg.PhyMode < PHY_11ABGN_MIXED)
		{
			/* Patch UI */
			pAd->CommonCfg.HtCapability.HtCapInfo.ChannelWidth = BW_20;
		}

		/* init */
		if (pAd->CommonCfg.bRdg)
		{	
			RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_RDG_ACTIVE);
			AsicEnableRDG(pAd);
		}
		else	
		{
			RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_RDG_ACTIVE);
			AsicDisableRDG(pAd);
		}	
#endif /* DOT11_N_SUPPORT */

		//AsicSetBssid(pAd, pAd->CurrentAddress); 
		AsicSetMcastWC(pAd);
		
		/* In AP mode,  First WCID Table in ASIC will never be used. To prevent it's 0xff-ff-ff-ff-ff-ff, Write 0 here. */
		/* p.s ASIC use all 0xff as termination of WCID table search. */
		RTMP_IO_WRITE32(pAd, MAC_WCID_BASE, 0x00);
		RTMP_IO_WRITE32(pAd, MAC_WCID_BASE + 4, 0x0);

		/* reset WCID table */
		for (idx=2; idx<255; idx++)
		{
			offset = MAC_WCID_BASE + (idx * HW_WCID_ENTRY_SIZE);	
			RTMP_IO_WRITE32(pAd, offset, 0x0);
			RTMP_IO_WRITE32(pAd, offset+4, 0x0);
		}

		pAd->MacTab.Content[0].Addr[0] = 0x01;
		pAd->MacTab.Content[0].HTPhyMode.field.MODE = MODE_OFDM;
		pAd->MacTab.Content[0].HTPhyMode.field.MCS = 3;

		AsicBBPAdjust(pAd);
		//MlmeSetTxPreamble(pAd, (USHORT)pAd->CommonCfg.TxPreamble);	

		if (1)	
		{
			ULONG	Addr4;
			UINT32	regValue;
			PUCHAR pP2PBssid = &pAd->CurrentAddress[0];
		
			Addr4 = (ULONG)(pP2PBssid[0])	    | 
				(ULONG)(pP2PBssid[1] << 8)  | 
				(ULONG)(pP2PBssid[2] << 16) |
				(ULONG)(pP2PBssid[3] << 24);
			RTMP_IO_WRITE32(pAd, MAC_BSSID_DW0, Addr4);
	
			Addr4 = 0;

			Addr4 = (ULONG)(pP2PBssid[4]) | (ULONG)(pP2PBssid[5] << 8);
			RTMP_IO_WRITE32(pAd, MAC_BSSID_DW1, Addr4);
	
			RTMP_IO_READ32(pAd, MAC_BSSID_DW1, &regValue);
			regValue &= 0x0000FFFF;

			regValue |= (1 << 16);

			if (pAd->chipCap.MBSSIDMode == MBSSID_MODE1)
				regValue |= (1 << 21);
			RTMP_IO_WRITE32(pAd, MAC_BSSID_DW1, regValue);		
		}
	

#ifdef RTMP_MAC_USB
        RTUSBBssBeaconInit(pAd);
#endif /* RTMP_MAC_USB */
	}

	//pAd->ApCfg.MBSSID[MAIN_MBSSID].PhyMode = PHY_11BGN_MIXED;
	if (pBeacon->interval != 0)
	{
		DBGPRINT(RT_DEBUG_TRACE,("CFG_TIM New BI %d\n", pBeacon->interval));
		pAd->CommonCfg.BeaconPeriod = pBeacon->interval;
	}
	
	if (pBeacon->dtim_period != 0)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("CFG_TIM New DP %d\n", pBeacon->dtim_period));
		pAd->ApCfg.DtimPeriod = pBeacon->dtim_period;	
	}
	
	CFG80211_UpdateBeacon(pAd, pBeacon->beacon_head, pBeacon->beacon_head_len,
			  				   pBeacon->beacon_tail, pBeacon->beacon_tail_len,
							   TRUE);

	if (isAdd)
	{
		/* Enable Bss Sync*/
		RTMP_IO_READ32(pAd, BCN_TIME_CFG, &csr9.word);
		DBGPRINT(RT_DEBUG_TRACE, ("CFG_TIM Apply BI %d\n", pAd->CommonCfg.BeaconPeriod));
		csr9.field.BeaconInterval = pAd->CommonCfg.BeaconPeriod << 4; /* ASIC register in units of 1/16 TU*/
		csr9.field.bTsfTicking = 1;
		csr9.field.TsfSyncMode = 3;
		csr9.field.bTBTTEnable = 1;
		csr9.field.bBeaconGen = 1;
		RTMP_IO_WRITE32(pAd, BCN_TIME_CFG, csr9.word);

		pAd->P2pCfg.bSentProbeRSP = TRUE;

#ifdef RTMP_MAC_USB
		/*
		 * Support multiple BulkIn IRP,
	 	 * the value on pAd->CommonCfg.NumOfBulkInIRP may be large than 1.
		 */
	
		UCHAR num_idx;

		for(num_idx=0; num_idx < pAd->CommonCfg.NumOfBulkInIRP; num_idx++)
		{
			RTUSBBulkReceive(pAd);
		}
	
#endif /* RTMP_MAC_USB */

{
		BOOLEAN     Cancelled;
		RTMPCancelTimer(&pAd->CommonCfg.BeaconUpdateTimer, & Cancelled);
}

#ifdef RTMP_MAC_USB
		RTMPInitTimer(pAd, &pAd->CommonCfg.BeaconUpdateTimer, GET_TIMER_FUNCTION(BeaconUpdateExec), pAd, TRUE);
#endif /* RTMP_MAC_USB */

{
		RTMPSetTimer(&pAd->CommonCfg.BeaconUpdateTimer, 10 /*pAd->CommonCfg.BeaconPeriod*/);
}


	}

#ifdef WFD_SUPPORT
	pAd->StaCfg.WfdCfg.bSuppGoOn = TRUE;
#endif /* WFD_SUPPORT */
#endif		
	return TRUE;

}

BOOLEAN CFG80211DRV_OpsExtraIesSet(
	VOID						*pAdOrg)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	
	CFG80211_CB *pCfg80211_CB = pAd->pCfg80211_CB;
	UINT ie_len = pCfg80211_CB->pCfg80211_ScanReq->ie_len;
    CFG80211DBG(RT_DEBUG_INFO, ("80211> CFG80211DRV_OpsExtraIesSet ==> %d\n", ie_len)); 

	if (pAd->StaCfg.pWpsProbeReqIe)
	{	
		os_free_mem(NULL, pAd->StaCfg.pWpsProbeReqIe);
		pAd->StaCfg.pWpsProbeReqIe = NULL;
	}

	pAd->StaCfg.WpsProbeReqIeLen = 0;

	CFG80211DBG(RT_DEBUG_INFO, ("80211> is_wpa_supplicant_up ==> %d\n", pAd->StaCfg.WpaSupplicantUP)); 
	os_alloc_mem(pAd, (UCHAR **)&(pAd->StaCfg.pWpsProbeReqIe), ie_len);
	if (pAd->StaCfg.pWpsProbeReqIe)
	{
		memcpy(pAd->StaCfg.pWpsProbeReqIe, pCfg80211_CB->pCfg80211_ScanReq->ie, ie_len);
		pAd->StaCfg.WpsProbeReqIeLen = ie_len;
		//hex_dump("WpsProbeReqIe", pAd->StaCfg.pWpsProbeReqIe, pAd->StaCfg.WpsProbeReqIeLen);
		DBGPRINT(RT_DEBUG_INFO, ("Set::RT_OID_WPS_PROBE_REQ_IE, WpsProbeReqIeLen = %d!!\n",
					pAd->StaCfg.WpsProbeReqIeLen));
	}
	else
	{
		CFG80211DBG(RT_DEBUG_ERROR, ("80211> CFG80211DRV_OpsExtraIesSet ==> allocate fail. \n")); 
		return FALSE;
	}
	return TRUE;
}


BOOLEAN CFG80211DRV_OpsJoinIbss(
	VOID						*pAdOrg,
	VOID						*pData)
{
#ifdef CONFIG_STA_SUPPORT
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	CMD_RTPRIV_IOCTL_80211_IBSS *pIbssInfo;


	pIbssInfo = (CMD_RTPRIV_IOCTL_80211_IBSS *)pData;
	pAd->StaCfg.bAutoReconnect = TRUE;

	pAd->CommonCfg.BeaconPeriod = pIbssInfo->BeaconInterval;
	Set_SSID_Proc(pAd, (PSTRING)pIbssInfo->pSsid);
#endif /* CONFIG_STA_SUPPORT */
	return TRUE;
}


BOOLEAN CFG80211DRV_OpsLeave(
	VOID						*pAdOrg)
{
#ifdef CONFIG_STA_SUPPORT
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;

    	MLME_DEAUTH_REQ_STRUCT   DeAuthReq;
	MLME_QUEUE_ELEM *pMsgElem = NULL;
    
	if (pAd->FlgCfg80211Connecting)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("CFG80211_MLME Disconnect in Connecting, ORI ==> %d\n", pAd->Mlme.CntlMachine.CurrState)); 
		//CFG80211_LostApInform(pAd);
	}
	

	pAd->StaCfg.bAutoReconnect = FALSE;
	pAd->FlgCfg80211Connecting = FALSE;

        pAd->MlmeAux.AutoReconnectSsidLen= 32;
        NdisZeroMemory(pAd->MlmeAux.AutoReconnectSsid, pAd->MlmeAux.AutoReconnectSsidLen);

	os_alloc_mem(pAd, (UCHAR **)&pMsgElem, sizeof(MLME_QUEUE_ELEM));
	COPY_MAC_ADDR(DeAuthReq.Addr, pAd->CommonCfg.Bssid);
	DeAuthReq.Reason = REASON_DEAUTH_STA_LEAVING;
	pMsgElem->MsgLen = sizeof(MLME_DEAUTH_REQ_STRUCT);
	NdisMoveMemory(pMsgElem->Msg, &DeAuthReq, sizeof(MLME_DEAUTH_REQ_STRUCT));
	MlmeDeauthReqAction(pAd, pMsgElem);
	os_free_mem(NULL, pMsgElem);
	
	LinkDown(pAd, FALSE);
#endif /* CONFIG_STA_SUPPORT */
	return TRUE;
}


BOOLEAN CFG80211DRV_StaGet(
	VOID						*pAdOrg,
	VOID						*pData)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	CMD_RTPRIV_IOCTL_80211_STA *pIbssInfo;


	pIbssInfo = (CMD_RTPRIV_IOCTL_80211_STA *)pData;

#ifdef CONFIG_AP_SUPPORT
{
	MAC_TABLE_ENTRY *pEntry;
	ULONG DataRate = 0;
	UINT32 RSSI;


	pEntry = MacTableLookup(pAd, pIbssInfo->MAC);
	if (pEntry == NULL)
		return FALSE;
	/* End of if */

	/* fill tx rate */
	getRate(pEntry->HTPhyMode, &DataRate);

	if ((pEntry->HTPhyMode.field.MODE == MODE_HTMIX) ||
		(pEntry->HTPhyMode.field.MODE == MODE_HTGREENFIELD))
	{
		if (pEntry->HTPhyMode.field.BW)
			pIbssInfo->TxRateFlags |= RT_CMD_80211_TXRATE_BW_40;
		/* End of if */
		if (pEntry->HTPhyMode.field.ShortGI)
			pIbssInfo->TxRateFlags |= RT_CMD_80211_TXRATE_SHORT_GI;
		/* End of if */

		pIbssInfo->TxRateMCS = pEntry->HTPhyMode.field.MCS;
	}
	else
	{
		pIbssInfo->TxRateFlags = RT_CMD_80211_TXRATE_LEGACY;
		pIbssInfo->TxRateMCS = DataRate*1000; /* unit: 100kbps */
	} /* End of if */

	/* fill signal */
	RSSI = RTMPAvgRssi(pAd, &pEntry->RssiSample);
	pIbssInfo->Signal = RSSI;

	/* fill tx count */
	pIbssInfo->TxPacketCnt = pEntry->OneSecTxNoRetryOkCount + 
						pEntry->OneSecTxRetryOkCount + 
						pEntry->OneSecTxFailCount;

	/* fill inactive time */
	pIbssInfo->InactiveTime = pEntry->NoDataIdleCount * 1000; /* unit: ms */
	pIbssInfo->InactiveTime *= MLME_TASK_EXEC_MULTIPLE;
	pIbssInfo->InactiveTime /= 20;
}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
{
	HTTRANSMIT_SETTING PhyInfo;
	ULONG DataRate = 0;
	UINT32 RSSI;


	/* fill tx rate */
    if ((!WMODE_CAP_N(pAd->CommonCfg.PhyMode)) ||
	 (pAd->MacTab.Content[BSSID_WCID].HTPhyMode.field.MODE <= MODE_OFDM))
	{
		PhyInfo.word = pAd->StaCfg.HTPhyMode.word;
	}
    else
		PhyInfo.word = pAd->MacTab.Content[BSSID_WCID].HTPhyMode.word;
	/* End of if */

	getRate(PhyInfo, &DataRate);

	if ((PhyInfo.field.MODE == MODE_HTMIX) ||
		(PhyInfo.field.MODE == MODE_HTGREENFIELD))
	{
		if (PhyInfo.field.BW)
			pIbssInfo->TxRateFlags |= RT_CMD_80211_TXRATE_BW_40;
		/* End of if */
		if (PhyInfo.field.ShortGI)
			pIbssInfo->TxRateFlags |= RT_CMD_80211_TXRATE_SHORT_GI;
		/* End of if */

		pIbssInfo->TxRateMCS = PhyInfo.field.MCS;
	}
	else
	{
		pIbssInfo->TxRateFlags = RT_CMD_80211_TXRATE_LEGACY;
		pIbssInfo->TxRateMCS = DataRate*10; /* unit: 100kbps */
	} /* End of if */

	/* fill signal */
	RSSI = RTMPAvgRssi(pAd, &pAd->StaCfg.RssiSample);
	pIbssInfo->Signal = RSSI;
}
#endif /* CONFIG_STA_SUPPORT */

	return TRUE;
}

BOOLEAN CFG80211DRV_ApKeyDel(
        VOID                                            *pAdOrg,
        VOID                                            *pData)
{
        PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
        CMD_RTPRIV_IOCTL_80211_KEY *pKeyInfo;
	MAC_TABLE_ENTRY *pEntry;
	
	pKeyInfo = (CMD_RTPRIV_IOCTL_80211_KEY *)pData;
	
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
        if (pKeyInfo->bPairwise == FALSE )
#else
        if (pKeyInfo->KeyId > 0)
#endif
	{
		DBGPRINT(RT_DEBUG_TRACE,("CFG: AsicRemoveSharedKeyEntry %d\n", pKeyInfo->KeyId));	
		AsicRemoveSharedKeyEntry(pAd, MAIN_MBSSID, pKeyInfo->KeyId);
	}
	else
	{
		pEntry = MacTableLookup(pAd, pKeyInfo->MAC);
		
		if (pEntry && (pEntry->Aid != 0))
		{
			NdisZeroMemory(&pEntry->PairwiseKey, sizeof(CIPHER_KEY));
			AsicRemovePairwiseKeyEntry(pAd, (UCHAR)pEntry->Aid);
		}
	}

}

BOOLEAN CFG80211DRV_ApKeyAdd(
        VOID                                            *pAdOrg,
        VOID                                            *pData)
{
#ifdef CONFIG_AP_SUPPORT
        PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
        CMD_RTPRIV_IOCTL_80211_KEY *pKeyInfo;
	MAC_TABLE_ENTRY *pEntry;

        DBGPRINT(RT_DEBUG_TRACE,("CFG: CFG80211DRV_ApKeyAdd\n"));
        pKeyInfo = (CMD_RTPRIV_IOCTL_80211_KEY *)pData;

	if (pKeyInfo->KeyType == RT_CMD_80211_KEY_WEP)
	{
		pAd->ApCfg.MBSSID[MAIN_MBSSID].WepStatus = Ndis802_11WEPEnabled;
	}
	else
	{
		/* AES */
		pAd->ApCfg.MBSSID[MAIN_MBSSID].WepStatus = Ndis802_11Encryption3Enabled;
		pAd->ApCfg.MBSSID[MAIN_MBSSID].GroupKeyWepStatus = Ndis802_11Encryption3Enabled;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
                if (pKeyInfo->bPairwise == FALSE )
#else
                if (pKeyInfo->KeyId > 0)
#endif		
		{
			UINT8 Wcid;
			PMULTISSID_STRUCT pMbss = &pAd->ApCfg.MBSSID[MAIN_MBSSID];
			//pMbss->DefaultKeyId = pKeyInfo->KeyId;

			if (pAd->ApCfg.MBSSID[MAIN_MBSSID].GroupKeyWepStatus == Ndis802_11Encryption3Enabled)
			{
				DBGPRINT(RT_DEBUG_TRACE, ("CFG: Set AES Security Set. (GROUP) %d\n", pKeyInfo->KeyLen));
				pAd->SharedKey[MAIN_MBSSID][pKeyInfo->KeyId].KeyLen= LEN_TK;
				NdisMoveMemory(pAd->SharedKey[MAIN_MBSSID][pKeyInfo->KeyId].Key, pKeyInfo->KeyBuf, pKeyInfo->KeyLen);
				//NdisMoveMemory(pAd->SharedKey[MAIN_MBSSID][pMbss->DefaultKeyId].RxMic, (Key.ik_keydata+16+8), 8);
				//NdisMoveMemory(pAd->SharedKey[MAIN_MBSSID][pMbss->DefaultKeyId].TxMic, (Key.ik_keydata+16), 8);
				
				pAd->SharedKey[MAIN_MBSSID][pKeyInfo->KeyId].CipherAlg = CIPHER_AES;

				AsicAddSharedKeyEntry(pAd, MAIN_MBSSID, pKeyInfo->KeyId, 
						&pAd->SharedKey[MAIN_MBSSID][pKeyInfo->KeyId]);

				GET_GroupKey_WCID(pAd, Wcid, MAIN_MBSSID);
				RTMPSetWcidSecurityInfo(pAd, MAIN_MBSSID, (UINT8)(pKeyInfo->KeyId), 
						pAd->SharedKey[MAIN_MBSSID][pKeyInfo->KeyId].CipherAlg, Wcid, SHAREDKEYTABLE);
				
			}
		}
		else
		{
			if (pKeyInfo->MAC)
				pEntry = MacTableLookup(pAd, pKeyInfo->MAC);
			if(pEntry)
			{
				DBGPRINT(RT_DEBUG_TRACE, ("CFG: Set AES Security Set. (PAIRWISE) %d\n", pKeyInfo->KeyLen));
				pEntry->PairwiseKey.KeyLen = LEN_TK;
				NdisCopyMemory(&pEntry->PTK[OFFSET_OF_PTK_TK], pKeyInfo->KeyBuf, OFFSET_OF_PTK_TK);
				NdisMoveMemory(pEntry->PairwiseKey.Key, &pEntry->PTK[OFFSET_OF_PTK_TK], pKeyInfo->KeyLen);
				pEntry->PairwiseKey.CipherAlg = CIPHER_AES;
				
				AsicAddPairwiseKeyEntry(pAd, (UCHAR)pEntry->Aid, &pEntry->PairwiseKey);
				RTMPSetWcidSecurityInfo(pAd, pEntry->apidx, (UINT8)(pKeyInfo->KeyId & 0x0fff),
					pEntry->PairwiseKey.CipherAlg, pEntry->Aid, PAIRWISEKEYTABLE);
			}
			else	
			{
				DBGPRINT(RT_DEBUG_ERROR,("CFG: Set AES Security Set. (PAIRWISE) But pEntry NULL\n"));
			}
		
			
			
		}
	}
#endif
	return TRUE;

}

BOOLEAN CFG80211DRV_StaKeyAdd(
	VOID						*pAdOrg,
	VOID						*pData)
{
#ifdef CONFIG_STA_SUPPORT
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	CMD_RTPRIV_IOCTL_80211_KEY *pKeyInfo;

	
	pKeyInfo = (CMD_RTPRIV_IOCTL_80211_KEY *)pData;

	if (pKeyInfo->KeyType == RT_CMD_80211_KEY_WEP)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("RT_CMD_80211_KEY_WEP\n"));
#if 0
		switch(pKeyInfo->KeyId)
		{
			case 1:
			default:
				Set_Key1_Proc(pAd, (PSTRING)pKeyInfo->KeyBuf);
				break;

			case 2:
				Set_Key2_Proc(pAd, (PSTRING)pKeyInfo->KeyBuf);
				break;

			case 3:
				Set_Key3_Proc(pAd, (PSTRING)pKeyInfo->KeyBuf);
				break;

			case 4:
				Set_Key4_Proc(pAd, (PSTRING)pKeyInfo->KeyBuf);
				break;
		} /* End of switch */
#endif
	} 
	else 
	{
		DBGPRINT(RT_DEBUG_TRACE, ("Set_WPAPSK_Proc ==> %d, %d, %d...\n", pKeyInfo->KeyId, pKeyInfo->KeyType, strlen(pKeyInfo->KeyBuf)));
		
		RT_CMD_STA_IOCTL_SECURITY IoctlSec;
		
		IoctlSec.KeyIdx = pKeyInfo->KeyId;
		IoctlSec.pData = pKeyInfo->KeyBuf;
		IoctlSec.length = pKeyInfo->KeyLen;
		
		/* YF@20120327: Due to WepStatus will be set in the cfg connect function.*/
		if (pAd->StaCfg.WepStatus == Ndis802_11Encryption2Enabled)
			IoctlSec.Alg = RT_CMD_STA_IOCTL_SECURITY_ALG_TKIP;
		else if (pAd->StaCfg.WepStatus == Ndis802_11Encryption3Enabled)
			IoctlSec.Alg = RT_CMD_STA_IOCTL_SECURITY_ALG_CCMP;
		
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
		if (pKeyInfo->bPairwise == FALSE )
#else
		if (pKeyInfo->KeyId > 0)
#endif	
		{
			if (pAd->StaCfg.GroupCipher == Ndis802_11Encryption2Enabled)
				IoctlSec.Alg = RT_CMD_STA_IOCTL_SECURITY_ALG_TKIP;
			else if (pAd->StaCfg.GroupCipher == Ndis802_11Encryption3Enabled)
				IoctlSec.Alg = RT_CMD_STA_IOCTL_SECURITY_ALG_CCMP;
				
			DBGPRINT(RT_DEBUG_TRACE, ("Install GTK: %d\n", IoctlSec.Alg));
			IoctlSec.ext_flags = RT_CMD_STA_IOCTL_SECURTIY_EXT_GROUP_KEY;
		}	
		else
		{
			if (pAd->StaCfg.PairCipher == Ndis802_11Encryption2Enabled)
				IoctlSec.Alg = RT_CMD_STA_IOCTL_SECURITY_ALG_TKIP;
			else if (pAd->StaCfg.PairCipher == Ndis802_11Encryption3Enabled)
				IoctlSec.Alg = RT_CMD_STA_IOCTL_SECURITY_ALG_CCMP;
				
			DBGPRINT(RT_DEBUG_TRACE, ("Install PTK: %d\n", IoctlSec.Alg));
			IoctlSec.ext_flags = RT_CMD_STA_IOCTL_SECURTIY_EXT_SET_TX_KEY;
		}
		
		Set_GroupKey_Proc(pAd, &IoctlSec);	
	}


#endif /* CONFIG_STA_SUPPORT */

	return TRUE;
}

extern INT RtmpIoctl_rt_ioctl_siwauth(
	IN      RTMP_ADAPTER                    *pAd,
	IN      VOID                            *pData,
	IN      ULONG                            Data);

BOOLEAN CFG80211DRV_Connect(
	VOID						*pAdOrg,
	VOID						*pData)
{
#ifdef CONFIG_STA_SUPPORT
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	CMD_RTPRIV_IOCTL_80211_CONNECT *pConnInfo;
	UCHAR SSID[NDIS_802_11_LENGTH_SSID];
	UINT32 SSIDLen;
	RT_CMD_STA_IOCTL_SECURITY_ADV IoctlWpa;

	if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_INFRA_ON) && 
            OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("CFG80211: Connected, disconnect first !\n"));
	}
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("CFG80211: No Connection\n"));
	}

	pConnInfo = (CMD_RTPRIV_IOCTL_80211_CONNECT *)pData;

	/* change to infrastructure mode if we are in ADHOC mode */
	Set_NetworkType_Proc(pAd, "Infra");

	SSIDLen = pConnInfo->SsidLen;
	if (SSIDLen > NDIS_802_11_LENGTH_SSID)
	{
		SSIDLen = NDIS_802_11_LENGTH_SSID;
	}
	
	memset(&SSID, 0, sizeof(SSID));
	memcpy(SSID, pConnInfo->pSsid, SSIDLen);

	if (pConnInfo->bWpsConnection) 
	{
		DBGPRINT(RT_DEBUG_TRACE, ("WPS Connection onGoing.....\n"));
		/* YF@20120327: Trigger Driver to Enable WPS function. */	
		pAd->StaCfg.WpaSupplicantUP |= WPA_SUPPLICANT_ENABLE_WPS;  /* Set_Wpa_Support(pAd, "3") */
		Set_AuthMode_Proc(pAd, "OPEN");
		Set_EncrypType_Proc(pAd, "NONE");
		Set_SSID_Proc(pAd, (PSTRING)SSID);
		
		return TRUE;
	}
	else
	{
		pAd->StaCfg.WpaSupplicantUP = WPA_SUPPLICANT_ENABLE; /* Set_Wpa_Support(pAd, "1")*/
	}	
	
	/* set authentication mode */
	if (pConnInfo->WpaVer == 2)
	{
		if (pConnInfo->FlgIs8021x == TRUE) {
			DBGPRINT(RT_DEBUG_TRACE, ("WPA2\n"));
			Set_AuthMode_Proc(pAd, "WPA2");
		}
		else 
		{
			DBGPRINT(RT_DEBUG_TRACE, ("WPA2PSK\n"));
			Set_AuthMode_Proc(pAd, "WPA2PSK");
		}
		/* End of if */
	}
	else if (pConnInfo->WpaVer == 1)
	{
		if (pConnInfo->FlgIs8021x == TRUE) {
			DBGPRINT(RT_DEBUG_TRACE, ("WPA\n"));
			Set_AuthMode_Proc(pAd, "WPA");
		}
		else 
		{
			DBGPRINT(RT_DEBUG_TRACE, ("WPAPSK\n"));
			Set_AuthMode_Proc(pAd, "WPAPSK");
		}
		/* End of if */
	}
	else if (pConnInfo->AuthType == Ndis802_11AuthModeAutoSwitch)
		Set_AuthMode_Proc(pAd, "WEPAUTO");
    else if (pConnInfo->AuthType == Ndis802_11AuthModeShared)
		Set_AuthMode_Proc(pAd, "SHARED");
	else
		Set_AuthMode_Proc(pAd, "OPEN");
	/* End of if */

	CFG80211DBG(RT_DEBUG_TRACE,
				("80211> AuthMode = %d\n", pAd->StaCfg.AuthMode));

	/* set encryption mode */
	if (pConnInfo->PairwiseEncrypType & RT_CMD_80211_CONN_ENCRYPT_CCMP) 
	{
		DBGPRINT(RT_DEBUG_TRACE, ("AES\n"));
		Set_EncrypType_Proc(pAd, "AES");
	}
	else if (pConnInfo->PairwiseEncrypType & RT_CMD_80211_CONN_ENCRYPT_TKIP) 
	{
		DBGPRINT(RT_DEBUG_TRACE, ("TKIP\n"));
		Set_EncrypType_Proc(pAd, "TKIP");
	}
	else if (pConnInfo->PairwiseEncrypType & RT_CMD_80211_CONN_ENCRYPT_WEP)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("WEP\n"));
		Set_EncrypType_Proc(pAd, "WEP");
	}
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("NONE\n"));
		Set_EncrypType_Proc(pAd, "NONE");		
	}
	
	/* Groupwise Key Information Setting */
	IoctlWpa.flags = RT_CMD_STA_IOCTL_WPA_GROUP;    
	if (pConnInfo->GroupwiseEncrypType & RT_CMD_80211_CONN_ENCRYPT_CCMP)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("GTK AES\n"));
		IoctlWpa.value = RT_CMD_STA_IOCTL_WPA_GROUP_CCMP;
		RtmpIoctl_rt_ioctl_siwauth(pAd, &IoctlWpa, 0);
	}
	else if (pConnInfo->GroupwiseEncrypType & RT_CMD_80211_CONN_ENCRYPT_TKIP)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("GTK TKIP\n"));
		IoctlWpa.value = RT_CMD_STA_IOCTL_WPA_GROUP_TKIP;
		RtmpIoctl_rt_ioctl_siwauth(pAd, &IoctlWpa, 0);
	} 

	CFG80211DBG(RT_DEBUG_TRACE,
				("80211> EncrypType = %d\n", pAd->StaCfg.WepStatus));

	CFG80211DBG(RT_DEBUG_TRACE,
				("80211> Key = %x\n", pConnInfo->pKey));

	/* set channel: STATION will auto-scan */

	/* set WEP key */
	if (pConnInfo->pKey &&
		((pConnInfo->GroupwiseEncrypType | pConnInfo->PairwiseEncrypType) &
												RT_CMD_80211_CONN_ENCRYPT_WEP))
	{
		UCHAR KeyBuf[50];

		/* reset AuthMode and EncrypType */
		Set_EncrypType_Proc(pAd, "WEP");

		/* reset key */
#ifdef RT_CFG80211_DEBUG
		hex_dump("KeyBuf=", (UINT8 *)pConnInfo->pKey, pConnInfo->KeyLen);
#endif /* RT_CFG80211_DEBUG */

		pAd->StaCfg.DefaultKeyId = pConnInfo->KeyIdx; /* base 0 */
		if (pConnInfo->KeyLen >= sizeof(KeyBuf))
			return FALSE;
		/* End of if */
		memcpy(KeyBuf, pConnInfo->pKey, pConnInfo->KeyLen);
		KeyBuf[pConnInfo->KeyLen] = 0x00;

		CFG80211DBG(RT_DEBUG_ERROR,
					("80211> pAd->StaCfg.DefaultKeyId = %d\n",
					pAd->StaCfg.DefaultKeyId));

		Set_Wep_Key_Proc(pAd, (PSTRING)KeyBuf, (INT)pConnInfo->KeyLen, (INT)pConnInfo->KeyIdx);

	} /* End of if */

	/* TODO: We need to provide a command to set BSSID to associate a AP */

	/* re-set SSID */
	pAd->FlgCfg80211Connecting = TRUE;

	Set_SSID_Proc(pAd, (PSTRING)SSID);
	CFG80211DBG(RT_DEBUG_TRACE, ("80211> Connecting SSID = %s\n", SSID));
#endif /* CONFIG_STA_SUPPORT */

	return TRUE;
}


VOID CFG80211DRV_RegNotify(
	VOID						*pAdOrg,
	VOID						*pData)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	CMD_RTPRIV_IOCTL_80211_REG_NOTIFY *pRegInfo;


	pRegInfo = (CMD_RTPRIV_IOCTL_80211_REG_NOTIFY *)pData;

	/* keep Alpha2 and we can re-call the function when interface is up */
	pAd->Cfg80211_Alpha2[0] = pRegInfo->Alpha2[0];
	pAd->Cfg80211_Alpha2[1] = pRegInfo->Alpha2[1];

	/* apply the new regulatory rule */
	if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_START_UP))
	{
		/* interface is up */
		CFG80211_RegRuleApply(pAd, pRegInfo->pWiphy, (UCHAR *)pRegInfo->Alpha2);
	}
	else
	{
		CFG80211DBG(RT_DEBUG_ERROR, ("crda> interface is down!\n"));
	} /* End of if */
}


VOID CFG80211DRV_SurveyGet(
	VOID						*pAdOrg,
	VOID						*pData)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	CMD_RTPRIV_IOCTL_80211_SURVEY *pSurveyInfo;


	pSurveyInfo = (CMD_RTPRIV_IOCTL_80211_SURVEY *)pData;

	pSurveyInfo->pCfg80211 = pAd->pCfg80211_CB;

#ifdef AP_QLOAD_SUPPORT
	pSurveyInfo->ChannelTimeBusy = pAd->QloadLatestChannelBusyTimePri;
	pSurveyInfo->ChannelTimeExtBusy = pAd->QloadLatestChannelBusyTimeSec;
#endif /* AP_QLOAD_SUPPORT */
}


VOID CFG80211_UnRegister(
	IN VOID						*pAdOrg,
	IN VOID						*pNetDev)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;


	/* sanity check */
	if (pAd->pCfg80211_CB == NULL)
		return;
	/* End of if */

	CFG80211OS_UnRegister(pAd->pCfg80211_CB, pNetDev);
	pAd->pCfg80211_CB = NULL;
	pAd->CommonCfg.HT_Disable = 0;
}


/*
========================================================================
Routine Description:
	Parse and handle country region in beacon from associated AP.

Arguments:
	pAdCB			- WLAN control block pointer
	pVIE			- Beacon elements
	LenVIE			- Total length of Beacon elements

Return Value:
	NONE

Note:
========================================================================
*/
VOID CFG80211_BeaconCountryRegionParse(
	IN VOID						*pAdCB,
	IN NDIS_802_11_VARIABLE_IEs	*pVIE,
	IN UINT16					LenVIE)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;
	UCHAR *pElement = (UCHAR *)pVIE;
	UINT32 LenEmt;


	while(LenVIE > 0)
	{
		pVIE = (NDIS_802_11_VARIABLE_IEs *)pElement;

		if (pVIE->ElementID == IE_COUNTRY)
		{
			/* send command to do regulation hint only when associated */
			RTEnqueueInternalCmd(pAd, CMDTHREAD_REG_HINT_11D,
								pVIE->data, pVIE->Length);
			break;
		} /* End of if */

		LenEmt = pVIE->Length + 2;

		if (LenVIE <= LenEmt)
			break; /* length is not enough */
		/* End of if */

		pElement += LenEmt;
		LenVIE -= LenEmt;
	} /* End of while */
} /* End of CFG80211_BeaconCountryRegionParse */

/*
========================================================================
Routine Description:
	Re-Initialize wireless channel/PHY in 2.4GHZ and 5GHZ.

Arguments:
	pAdCB			- WLAN control block pointer

Return Value:
	NONE

Note:
	CFG80211_SupBandInit() is called in xx_probe().
========================================================================
*/
VOID CFG80211_LostApInform(
    IN VOID 					*pAdCB)
{

	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;
	CFG80211_CB *p80211CB = pAd->pCfg80211_CB;
	
	DBGPRINT(RT_DEBUG_TRACE, ("80211> CFG80211_LostApInform ==> %d\n", 
					p80211CB->pCfg80211_Wdev->sme_state));
	pAd->StaCfg.bAutoReconnect = FALSE;

	//cfg80211_disconnected(pAd->net_dev, 0, NULL, 0, GFP_KERNEL);
	if (p80211CB->pCfg80211_Wdev->sme_state == CFG80211_SME_CONNECTING)
	{
		   cfg80211_connect_result(pAd->net_dev, NULL, NULL, 0, NULL, 0,
								   WLAN_STATUS_UNSPECIFIED_FAILURE, GFP_KERNEL);
	}
	else if (p80211CB->pCfg80211_Wdev->sme_state == CFG80211_SME_CONNECTED)
	{
		   cfg80211_disconnected(pAd->net_dev, 0, NULL, 0, GFP_KERNEL);
	} 

}

BOOLEAN CFG80211DRV_OpsCancelRemainOnChannel(
	VOID                                            *pAdOrg,
	UINT32                                          cookie)
{	
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	BOOLEAN Cancelled;
	CFG80211DBG(RT_DEBUG_TRACE, ("%s\n", __FUNCTION__));

        if (pAd->Cfg80211RocTimerRunning == TRUE)
        {
                CFG80211DBG(RT_DEBUG_TRACE, ("CFG_ROC : CANCEL Cfg80211RemainOnChannelDurationTimer\n"));
                RTMPCancelTimer(&pAd->Cfg80211RemainOnChannelDurationTimer, &Cancelled);
                pAd->Cfg80211RocTimerRunning = FALSE;
        }

	//pAd->CommonCfg.Channel

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34))
//	cfg80211_remain_on_channel_expired(pAd->net_dev, cookie, pAd->CfgChanInfo.chan, 
//				pAd->CfgChanInfo.ChanType, GFP_KERNEL);
#endif
	return TRUE;	
}

BOOLEAN CFG80211DRV_OpsRemainOnChannel(	
	VOID						*pAdOrg,	
	VOID						*pData,	
	UINT32 						duration)
{
	CFG80211DBG(RT_DEBUG_INFO, ("%s\n", __FUNCTION__));
	
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdOrg;
	CMD_RTPRIV_IOCTL_80211_CHAN *pChanInfo;
	STRING ChStr[5] = { 0 };
	BOOLEAN     Cancelled;
	
	pChanInfo = (CMD_RTPRIV_IOCTL_80211_CHAN *) pData;

	//if (pAd->CommonCfg.Channel != pChanInfo->ChanId) 
	if(1)
	{
                pAd->CommonCfg.Channel= pChanInfo->ChanId;
                AsicSwitchChannel(pAd, pAd->CommonCfg.Channel, FALSE);
                AsicLockChannel(pAd, pAd->CommonCfg.Channel);
	}
	else
	{
		//CFG80211DBG(RT_DEBUG_ERROR, ("80211> Switch Channel But same with Now\n"));
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34))
	cfg80211_ready_on_channel(pAd->net_dev,  pChanInfo->cookie, pChanInfo->chan, pChanInfo->ChanType, duration, GFP_KERNEL);
#endif
	NdisCopyMemory(&pAd->CfgChanInfo, pChanInfo, sizeof(CMD_RTPRIV_IOCTL_80211_CHAN));

	//RTMPCancelTimer(&pAd->Cfg80211RemainOnChannelDurationTimer, &Cancelled);
	if (pAd->Cfg80211RocTimerInit == FALSE)
	{
		CFG80211DBG(RT_DEBUG_TRACE, ("CFG80211_ROC : INIT Cfg80211RemainOnChannelDurationTimer\n"));
		RTMPInitTimer(pAd, &pAd->Cfg80211RemainOnChannelDurationTimer, GET_TIMER_FUNCTION(RemainOnChannelTimeout), pAd, FALSE);
		pAd->Cfg80211RocTimerInit = TRUE;
	}
	
	if (pAd->Cfg80211RocTimerRunning == TRUE)
	{
		CFG80211DBG(RT_DEBUG_TRACE, ("CFG80211_ROC : CANCEL Cfg80211RemainOnChannelDurationTimer\n"));
		RTMPCancelTimer(&pAd->Cfg80211RemainOnChannelDurationTimer, &Cancelled);
		pAd->Cfg80211RocTimerRunning = FALSE;
	}

	RTMPSetTimer(&pAd->Cfg80211RemainOnChannelDurationTimer, duration + 10);
	pAd->Cfg80211RocTimerRunning = TRUE;

	return TRUE;	
}


/*
========================================================================
Routine Description:
	Hint to the wireless core a regulatory domain from driver.

Arguments:
	pAd				- WLAN control block pointer
	pCountryIe		- pointer to the country IE
	CountryIeLen	- length of the country IE

Return Value:
	NONE

Note:
	Must call the function in kernel thread.
========================================================================
*/
VOID CFG80211_RegHint(
	IN VOID						*pAdCB,
	IN UCHAR					*pCountryIe,
	IN ULONG					CountryIeLen)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;


	CFG80211OS_RegHint(CFG80211CB, pCountryIe, CountryIeLen);
} /* End of CFG80211_RegHint */


/*
========================================================================
Routine Description:
	Hint to the wireless core a regulatory domain from country element.

Arguments:
	pAdCB			- WLAN control block pointer
	pCountryIe		- pointer to the country IE
	CountryIeLen	- length of the country IE

Return Value:
	NONE

Note:
	Must call the function in kernel thread.
========================================================================
*/
VOID CFG80211_RegHint11D(
	IN VOID						*pAdCB,
	IN UCHAR					*pCountryIe,
	IN ULONG					CountryIeLen)
{
	/* no regulatory_hint_11d() in 2.6.32 */
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;


	CFG80211OS_RegHint11D(CFG80211CB, pCountryIe, CountryIeLen);
} /* End of CFG80211_RegHint11D */


/*
========================================================================
Routine Description:
	Apply new regulatory rule.

Arguments:
	pAdCB			- WLAN control block pointer
	pWiphy			- Wireless hardware description
	pAlpha2			- Regulation domain (2B)

Return Value:
	NONE

Note:
	Can only be called when interface is up.

	For general mac80211 device, it will be set to new power by Ops->config()
	In rt2x00/, the settings is done in rt2x00lib_config().
========================================================================
*/
VOID CFG80211_RegRuleApply(
	IN VOID						*pAdCB,
	IN VOID						*pWiphy,
	IN UCHAR					*pAlpha2)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;
	VOID *pBand24G, *pBand5G;
	UINT32 IdBand, IdChan, IdPwr;
	UINT32 ChanNum, ChanId, Power, RecId, DfsType;
	BOOLEAN FlgIsRadar;
	ULONG IrqFlags;
#ifdef DFS_SUPPORT	
	RADAR_DETECT_STRUCT	*pRadarDetect;
#endif /* DFS_SUPPORT */


	CFG80211DBG(RT_DEBUG_TRACE, ("crda> CFG80211_RegRuleApply ==>\n"));

	/* init */
	pBand24G = NULL;
	pBand5G = NULL;

	if (pAd == NULL)
		return;

	RTMP_IRQ_LOCK(&pAd->irq_lock, IrqFlags);

	/* zero first */
	NdisZeroMemory(pAd->ChannelList,
					MAX_NUM_OF_CHANNELS * sizeof(CHANNEL_TX_POWER));

	/* 2.4GHZ & 5GHz */
	RecId = 0;
#ifdef DFS_SUPPORT	
	pRadarDetect = &pAd->CommonCfg.RadarDetect;
#endif /* DFS_SUPPORT */

	/* find the DfsType */
	DfsType = CE;

	pBand24G = NULL;
	pBand5G = NULL;

	if (CFG80211OS_BandInfoGet(CFG80211CB, pWiphy, &pBand24G, &pBand5G) == FALSE)
		return;

#ifdef AUTO_CH_SELECT_ENHANCE
#ifdef EXT_BUILD_CHANNEL_LIST
	if ((pAlpha2[0] != '0') && (pAlpha2[1] != '0'))
	{
		UINT32 IdReg;

		if (pBand5G != NULL)
		{
			for(IdReg=0; ; IdReg++)
			{
				if (ChRegion[IdReg].CountReg[0] == 0x00)
					break;
				/* End of if */
	
				if ((pAlpha2[0] == ChRegion[IdReg].CountReg[0]) &&
					(pAlpha2[1] == ChRegion[IdReg].CountReg[1]))
				{
					if (pAd->CommonCfg.DfsType != MAX_RD_REGION)
						DfsType = pAd->CommonCfg.DfsType;
					else
						DfsType = ChRegion[IdReg].DfsType;
	
					CFG80211DBG(RT_DEBUG_TRACE,
								("crda> find region %c%c, DFS Type %d\n",
								pAlpha2[0], pAlpha2[1], DfsType));
					break;
				} /* End of if */
			} /* End of for */
		} /* End of if */
	} /* End of if */
#endif /* EXT_BUILD_CHANNEL_LIST */
#endif /* AUTO_CH_SELECT_ENHANCE */

	for(IdBand=0; IdBand<2; IdBand++)
	{
		if (((IdBand == 0) && (pBand24G == NULL)) ||
			((IdBand == 1) && (pBand5G == NULL)))
		{
			continue;
		} /* End of if */

		if (IdBand == 0)
		{
			CFG80211DBG(RT_DEBUG_TRACE, ("crda> reset chan/power for 2.4GHz\n"));
		}
		else
		{
			CFG80211DBG(RT_DEBUG_TRACE, ("crda> reset chan/power for 5GHz\n"));
		} /* End of if */

		ChanNum = CFG80211OS_ChanNumGet(CFG80211CB, pWiphy, IdBand);

		for(IdChan=0; IdChan<ChanNum; IdChan++)
		{
			if (CFG80211OS_ChanInfoGet(CFG80211CB, pWiphy, IdBand, IdChan,
									&ChanId, &Power, &FlgIsRadar) == FALSE)
			{
				/* the channel is not allowed in the regulatory domain */
				/* get next channel information */
				continue;
			} /* End of if */

			if ((pAd->CommonCfg.PhyMode == PHY_11A) ||
				(pAd->CommonCfg.PhyMode == PHY_11AN_MIXED))
			{
				/* 5G-only mode */
				if (ChanId <= CFG80211_NUM_OF_CHAN_2GHZ)
					continue; /* check next */
				/* End of if */
			} /* End of if */

			if ((pAd->CommonCfg.PhyMode != PHY_11A) &&
				(pAd->CommonCfg.PhyMode != PHY_11ABG_MIXED) &&
				(pAd->CommonCfg.PhyMode != PHY_11AN_MIXED) &&
				(pAd->CommonCfg.PhyMode != PHY_11ABGN_MIXED) &&
				(pAd->CommonCfg.PhyMode != PHY_11AGN_MIXED))
			{
				/* 2.5G-only mode */
				if (ChanId > CFG80211_NUM_OF_CHAN_2GHZ)
					continue; /* check next */
				/* End of if */
			} /* End of if */

			for(IdPwr=0; IdPwr<MAX_NUM_OF_CHANNELS; IdPwr++)
			{
				if (ChanId == pAd->TxPower[IdPwr].Channel)
				{
					/* init the channel info. */
					NdisMoveMemory(&pAd->ChannelList[RecId],
									&pAd->TxPower[IdPwr],
									sizeof(CHANNEL_TX_POWER));

					/* keep channel number */
					pAd->ChannelList[RecId].Channel = ChanId;

					/* keep maximum tranmission power */
					pAd->ChannelList[RecId].MaxTxPwr = Power;

					/* keep DFS flag */
					if (FlgIsRadar == TRUE)
						pAd->ChannelList[RecId].DfsReq = TRUE;
					else
						pAd->ChannelList[RecId].DfsReq = FALSE;
					/* End of if */

					/* keep DFS type */
					pAd->ChannelList[RecId].RegulatoryDomain = DfsType;

					/* re-set DFS info. */
					pAd->CommonCfg.RDDurRegion = DfsType;

					CFG80211DBG(RT_DEBUG_TRACE,
								("Chan %03d:\tpower %d dBm, "
								"DFS %d, DFS Type %d\n",
								ChanId, Power,
								((FlgIsRadar == TRUE)?1:0),
								DfsType));

					/* change to record next channel info. */
					RecId ++;
					break;
				} /* End of if */
			} /* End of for */
		} /* End of for */
	} /* End of for */

	pAd->ChannelListNum = RecId;
	RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);

	CFG80211DBG(RT_DEBUG_TRACE, ("crda> Number of channels = %d\n", RecId));
} /* End of CFG80211_RegRuleApply */


/*
========================================================================
Routine Description:
	Inform us that a scan is got.

Arguments:
	pAdCB				- WLAN control block pointer

Return Value:
	NONE

Note:
	Call RT_CFG80211_SCANNING_INFORM, not CFG80211_Scaning
========================================================================
*/
VOID CFG80211_Scaning(
	IN VOID							*pAdCB,
	IN UINT32						BssIdx,
	IN UINT32						ChanId,
	IN UCHAR						*pFrame,
	IN UINT32						FrameLen,
	IN INT32						RSSI)
{
#ifdef CONFIG_STA_SUPPORT
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;
	VOID *pCfg80211_CB = pAd->pCfg80211_CB;
	BOOLEAN FlgIsNMode;
	UINT8 BW;


	//CFG80211DBG(RT_DEBUG_ERROR, ("80211> CFG80211_Scaning ==>\n"));

	if (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("80211> Network is down!\n"));
		return;
	} /* End of if */

	/*
		In connect function, we also need to report BSS information to cfg80211;
		Not only scan function.
	*/
	if ((pAd->FlgCfg80211Scanning == FALSE) &&
		(pAd->FlgCfg80211Connecting == FALSE))
	{
		
		//CFG80211DBG(RT_DEBUG_ERROR, ("YF DEBUG: FlgCfg80211Scanning & FlgCfg80211Connecting ALL False\n"));
		return; /* no scan is running */
	} /* End of if */

	/* init */
	/* Note: Can not use local variable to do pChan */
	if (WMODE_CAP_N(pAd->CommonCfg.PhyMode))
		FlgIsNMode = TRUE;
	else
		FlgIsNMode = FALSE;

	if (pAd->CommonCfg.RegTransmitSetting.field.BW == BW_20)
		BW = 0;
	else
		BW = 1;

	CFG80211OS_Scaning(pCfg80211_CB,
						ChanId,
						pFrame,
						FrameLen,
						RSSI,
						FlgIsNMode,
						BW);
#endif /* CONFIG_STA_SUPPORT */
} /* End of CFG80211_Scaning */

static void CFG80211_UpdateBssAvgRssi(
	IN      PBSS_ENTRY                              pBssEntry)
{
        BOOLEAN bInitial = FALSE;

        if (!(pBssEntry->AvgRssiX8 | pBssEntry->AvgRssi))
        {
                bInitial = TRUE;
        }

        if (bInitial)
        {
                pBssEntry->AvgRssiX8 = pBssEntry->Rssi << 3;
                pBssEntry->AvgRssi  = pBssEntry->Rssi;
        }
        else
        {
                pBssEntry->AvgRssiX8 = (pBssEntry->AvgRssiX8 - pBssEntry->AvgRssi) + pBssEntry->Rssi;
        }

        pBssEntry->AvgRssi = pBssEntry->AvgRssiX8 >> 3;

}

/*
========================================================================
Routine Description:
	Inform us that scan ends.

Arguments:
	pAdCB			- WLAN control block pointer
	FlgIsAborted	- 1: scan is aborted

Return Value:
	NONE

Note:
========================================================================
*/
VOID CFG80211_ScanEnd(
	IN VOID						*pAdCB,
	IN BOOLEAN					FlgIsAborted)
{
#ifdef CONFIG_STA_SUPPORT
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;
	UINT32 index;
	PBSS_ENTRY pBssEntry;
	CFG80211_CB *pCfg80211_CB  = (CFG80211_CB *)pAd->pCfg80211_CB;
	struct wiphy *pWiphy = pCfg80211_CB->pCfg80211_Wdev->wiphy;
	struct ieee80211_channel *chan;
	UINT32 CenFreq;
	struct cfg80211_bss *bss;

	if (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("80211> Network is down!\n"));
		return;
	} /* End of if */

	if (pAd->FlgCfg80211Scanning == FALSE)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("80211> No scan is running!\n"));
		return; /* no scan is running */
	} /* End of if */

	if (FlgIsAborted == TRUE)
		FlgIsAborted = 1;
	else
	{
		FlgIsAborted = 0;
		for (index = 0; index < pAd->ScanTab.BssNr; index++) 
		{
			pBssEntry = &pAd->ScanTab.BssEntry[index];
			
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,39)) 
			if (pAd->ScanTab.BssEntry[index].Channel > 14) 
				CenFreq = ieee80211_channel_to_frequency(pAd->ScanTab.BssEntry[index].Channel , IEEE80211_BAND_5GHZ);
			else 
				CenFreq = ieee80211_channel_to_frequency(pAd->ScanTab.BssEntry[index].Channel , IEEE80211_BAND_2GHZ);
#else
			CenFreq = ieee80211_channel_to_frequency(pAd->ScanTab.BssEntry[index].Channel);
#endif

			chan = ieee80211_get_channel(pWiphy, CenFreq);			
			bss = cfg80211_get_bss(pWiphy, chan, pBssEntry->Bssid, pBssEntry->Ssid, pBssEntry->SsidLen, 
						WLAN_CAPABILITY_ESS, WLAN_CAPABILITY_ESS);
			if (bss == NULL)
			{
				/* ScanTable Entry not exist in kernel buffer */
			}
			else
			{
				/* HIt */
				CFG80211_UpdateBssAvgRssi(pBssEntry);
				bss->signal = pBssEntry->AvgRssi * 100; //UNIT: MdBm
				cfg80211_put_bss(bss);
			}
		}
	}
 
	CFG80211OS_ScanEnd(CFG80211CB, FlgIsAborted);

	pAd->FlgCfg80211Scanning = FALSE;
#endif /* CONFIG_STA_SUPPORT */
} /* End of CFG80211_ScanEnd */


/*
========================================================================
Routine Description:
	Inform CFG80211 about association status.

Arguments:
	pAdCB			- WLAN control block pointer
	pBSSID			- the BSSID of the AP
	pReqIe			- the element list in the association request frame
	ReqIeLen		- the request element length
	pRspIe			- the element list in the association response frame
	RspIeLen		- the response element length
	FlgIsSuccess	- 1: success; otherwise: fail

Return Value:
	None

Note:
========================================================================
*/
VOID CFG80211_ConnectResultInform(
	IN VOID						*pAdCB,
	IN UCHAR					*pBSSID,
	IN UCHAR					*pReqIe,
	IN UINT32					ReqIeLen,
	IN UCHAR					*pRspIe,
	IN UINT32					RspIeLen,
	IN UCHAR					FlgIsSuccess)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;


	CFG80211DBG(RT_DEBUG_TRACE, ("80211> CFG80211_ConnectResultInform ==>\n"));

	CFG80211OS_ConnectResultInform(CFG80211CB,
								pBSSID,
								pReqIe,
								ReqIeLen,
								pRspIe,
								RspIeLen,
								FlgIsSuccess);

	pAd->FlgCfg80211Connecting = FALSE;
} /* End of CFG80211_ConnectResultInform */


/*
========================================================================
Routine Description:
	Re-Initialize wireless channel/PHY in 2.4GHZ and 5GHZ.

Arguments:
	pAdCB			- WLAN control block pointer

Return Value:
	TRUE			- re-init successfully
	FALSE			- re-init fail

Note:
	CFG80211_SupBandInit() is called in xx_probe().
	But we do not have complete chip information in xx_probe() so we
	need to re-init bands in xx_open().
========================================================================
*/
BOOLEAN CFG80211_SupBandReInit(
	IN VOID						*pAdCB)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;
	CFG80211_BAND BandInfo;


	CFG80211DBG(RT_DEBUG_TRACE, ("80211> re-init bands...\n"));

	/* re-init bands */
	NdisZeroMemory(&BandInfo, sizeof(BandInfo));
	CFG80211_BANDINFO_FILL(pAd, &BandInfo);

	return CFG80211OS_SupBandReInit(CFG80211CB, &BandInfo);
} /* End of CFG80211_SupBandReInit */


INT CFG80211_StaPortSecured(
	IN VOID                                         *pAdCB,
	IN UCHAR 					*pMac,
	IN UINT						flag)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;
	MAC_TABLE_ENTRY *pEntry;

	pEntry = MacTableLookup(pAd, pMac);
	if (!pEntry)
	{
		printk("Can't find pEntry in CFG80211_StaPortSecured\n");
	}
	else
	{
		if (flag)
		{
			printk("AID:%d, PortSecured\n", pEntry->Aid);
			pEntry->PrivacyFilter = Ndis802_11PrivFilterAcceptAll;
			pEntry->WpaState = AS_PTKINITDONE;
			pEntry->PortSecured = WPA_802_1X_PORT_SECURED;	
		}
		else
		{
			printk("AID:%d, PortNotSecured\n", pEntry->Aid);
			pEntry->PrivacyFilter = Ndis802_11PrivFilter8021xWEP;
			pEntry->PortSecured = WPA_802_1X_PORT_NOT_SECURED;	
		}	
	}
	
	return 0;
}

INT CFG80211_ApStaDel(
	IN VOID                                         *pAdCB,
	IN UCHAR                                        *pMac)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;
	MAC_TABLE_ENTRY *pEntry;
	
	if (pMac == NULL)
	{
		MacTableReset(pAd);
	}
	else
	{
		pEntry = MacTableLookup(pAd, pMac);
		if (pEntry)
		{
			printk("DEAUTH with %02X:%02X:%02X:%02X:%02X:%02X\n", PRINT_MAC(pEntry->Addr));
			MlmeDeAuthAction(pAd, pEntry, 2, FALSE);
		}
		else
			printk("Can't find pEntry in ApStaDel\n");
	}
}

//CMD_RTPRIV_IOCTL_80211_KEY_DEFAULT_SET:
INT CFG80211_setDefaultKey(
	IN VOID                                         *pAdCB,
	IN UINT 					Data
)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;
	CFG80211_CB *p80211CB = pAd->pCfg80211_CB;
#ifdef CONFIG_AP_SUPPORT
        if (p80211CB->pCfg80211_Wdev->iftype == RT_CMD_80211_IFTYPE_AP)
	//if (pAd->VifNextMode == RT_CMD_80211_IFTYPE_AP)
        {
		 printk("Set Ap Default Key: %d\n", Data);
        	pAd->ApCfg.MBSSID[MAIN_MBSSID].DefaultKeyId = Data;
        }
        else
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
        {
		printk("Set Sta Default Key: %d\n", Data);
                pAd->StaCfg.DefaultKeyId = Data; /* base 0 */
        }
#endif /* CONFIG_STA_SUPPORT */
	return 0;	

}


INT CFG80211_reSetToDefault(
	IN VOID                                         *pAdCB)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;
	printk("==========> TYPE Reset CFG80211 Global Setting Here\n");
	
	pAd->Cfg80211RegisterProbeReqFrame = FALSE;
	pAd->Cfg80211RegisterActionFrame = FALSE;
	pAd->Cfg80211ProbeReqCount = 0;
	pAd->Cfg80211ActionCount = 0;

	pAd->Cfg80211RocTimerInit = FALSE;
	pAd->Cfg80211RocTimerRunning = FALSE;

	pAd->FlgCfg80211Scanning = FALSE;
}

VOID CFG80211_IndicateScanFail(
	IN VOID                                         *pAdCB)
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdCB;
	CFG80211_CB *p80211CB = pAd->pCfg80211_CB;
	
	CFG80211OS_ScanEnd(p80211CB, TRUE);
}	
#endif /* RT_CFG80211_SUPPORT */

/* End of cfg80211drv.c */
