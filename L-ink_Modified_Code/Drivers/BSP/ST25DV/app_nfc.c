/**
  ******************************************************************************
  * File Name          :  stmicroelectronics_x-cube-nfc4_1_5_2.c
  * Description        : This file provides code for the configuration
  *                      of the STMicroelectronics.X-CUBE-NFC4.1.5.2 instances.
  ******************************************************************************
  *
  * COPYRIGHT 2020 STMicroelectronics
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  ******************************************************************************
  */

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "main.h"

/* Includes ------------------------------------------------------------------*/
#include "app_nfc.h"
#include "nfc04a1_nfctag.h"
#include "stm32l0xx_hal.h"
#include "stdio.h"
#include "string.h"	
#include "epd_w21.h"
/** @defgroup ST25_Nucleo
  * @{
  */

/** @defgroup Main
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/

extern unsigned char nfcBuffer[];

char uartmsg[80];

uint32_t st25dvbmsize = 0;
ST25DV_PASSWD passwd;
ST25DV_I2CSSO_STATUS i2csso;
ST25DV_MEM_SIZE st25dvmemsize;
uint32_t ret;

uint8_t readdata=0; //从eeprom取出的值
uint8_t i = 1;
uint8_t flag1 = 1;  //屏幕刷新标志位
uint8_t cir = 0;    //第几次循环

int time = 0;
int num = 0; 
/* Private functions ---------------------------------------------------------*/

void MX_NFC4_I2C_RW_DATA_Init(void);
void MX_NFC4_I2C_R_DATA_Process(uint32_t adr);
void MX_NFC4_I2C_W_DATA_Process(uint32_t adr,uint8_t wdata);
int16_t checkdatainzonex(	uint32_t memindex, ST25DV_I2C_PROT_ZONE pProtZone);

void MX_NFC_Init(void)
{
  /* USER CODE BEGIN SV */ 

  /* USER CODE END SV */

  /* USER CODE BEGIN NFC4_Library_Init_PreTreatment */
  
  /* USER CODE END NFC4_Library_Init_PreTreatment */

  /* Initialize the peripherals and the NFC4 components */

  MX_NFC4_I2C_RW_DATA_Init();
	
  //MX_NFC4_I2C_RW_DATA_Process();
  

  /* USER CODE BEGIN SV */ 

  /* USER CODE END SV */
  
  /* USER CODE BEGIN NFC4_Library_Init_PostTreatment */
  
  /* USER CODE END NFC4_Library_Init_PostTreatment */
}
/*
 * LM background task
 */
void MX_NFC_Process(void)
{
  /* USER CODE BEGIN NFC4_Library_Process */
  MX_NFC4_I2C_R_DATA_Process(500);//查询第500字节 
  if(readdata==0xaa)
	{
		HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_2);
		time = 0;
    for(uint16_t j=0;j<500;j++)
	  {
	    MX_NFC4_I2C_R_DATA_Process(j);//查询第一个块
			//nfcBuffer[(cir*500)+j] = (char)readdata;
			nfcBuffer[num] = readdata;
			num++;
	  }
		MX_NFC4_I2C_W_DATA_Process(500,0);//读完这次后 复位标志位
		HAL_Delay(100);
		
		if(num >= 4999)
		{ 
		  EpdDisFull((unsigned char *)nfcBuffer, 1);
			num = 0;
		}
		
	}
	HAL_Delay(400);
	
	time+=400;
	if(time>=4000)
	{
	  num = 0;
		time = 0;
	}
  /* USER CODE END NFC4_Library_Process */
}


  /**
  * @brief  Initialize the NFC4 I2C data read/ write Example
  * @retval None
  */
void MX_NFC4_I2C_RW_DATA_Init(void)
{
  /* Init ST25DV driver */
  while( NFC04A1_NFCTAG_Init(NFC04A1_NFCTAG_INSTANCE) != NFCTAG_OK );
	/* Reset Mailbox enable to allow write to EEPROM */
	NFC04A1_NFCTAG_ResetMBEN_Dyn(NFC04A1_NFCTAG_INSTANCE);
	/* Get ST25DV EEPROM size */
	NFC04A1_NFCTAG_ReadMemSize(NFC04A1_NFCTAG_INSTANCE, &st25dvmemsize);
	 /* st25dvmemsize is composed of Mem_Size (number of blocks) and BlockSize (size of each blocks in bytes) */
  st25dvbmsize = (st25dvmemsize.Mem_Size + 1) * (st25dvmemsize.BlockSize + 1);
}

   /**
  * @brief  Process of the NFC4 I2C data read/ write Example
  * @retval None
  */
void MX_NFC4_I2C_R_DATA_Process(uint32_t adr)
{
	uint16_t nbbytes=1;
	uint32_t memindex = adr;
	ST25DV_I2C_PROT_ZONE pProtZone;
	int16_t zonexstatus;

   /* Get ST25DV protection configuration */	
	NFC04A1_NFCTAG_ReadI2CProtectZone( NFC04A1_NFCTAG_INSTANCE, &pProtZone );
	
	zonexstatus=checkdatainzonex(memindex,pProtZone);
	
//	/* Read EEPROM */	
	if((zonexstatus==ST25DV_NO_PROT)||(zonexstatus==ST25DV_WRITE_PROT))
	{
		ret = NFC04A1_NFCTAG_ReadData(NFC04A1_NFCTAG_INSTANCE, &readdata, memindex, nbbytes );
	}
	else if((zonexstatus==ST25DV_READ_PROT)||(zonexstatus==ST25DV_READWRITE_PROT))
	{
/* if I2C session is closed, present password to open session */
		passwd.MsbPasswd = 0;
		passwd.LsbPasswd = 0;
		NFC04A1_NFCTAG_PresentI2CPassword(NFC04A1_NFCTAG_INSTANCE, passwd);
		
		ret = NFC04A1_NFCTAG_ReadData(NFC04A1_NFCTAG_INSTANCE, &readdata, memindex, nbbytes );
	}	
}

void MX_NFC4_I2C_W_DATA_Process(uint32_t adr,uint8_t wdata)
{
	uint32_t memindex = adr;

	uint8_t writedata = wdata;
	ST25DV_I2C_PROT_ZONE pProtZone;
	int16_t zonexstatus;

  /* Get ST25DV protection configuration */	
	NFC04A1_NFCTAG_ReadI2CProtectZone( NFC04A1_NFCTAG_INSTANCE, &pProtZone );
	
	zonexstatus=checkdatainzonex(memindex,pProtZone);

	/* Write EEPROM */		
	if((zonexstatus==ST25DV_NO_PROT)||(zonexstatus==ST25DV_READ_PROT))
	{
    ret = NFC04A1_NFCTAG_WriteData(NFC04A1_NFCTAG_INSTANCE, &writedata, memindex, 1 );	
	}
	else if((zonexstatus==ST25DV_WRITE_PROT)||(zonexstatus==ST25DV_READWRITE_PROT))
	{
/* if I2C session is closed, present password to open session */
		passwd.MsbPasswd = 0;
		passwd.LsbPasswd = 0;
		NFC04A1_NFCTAG_PresentI2CPassword(NFC04A1_NFCTAG_INSTANCE, passwd);
    ret = NFC04A1_NFCTAG_WriteData(NFC04A1_NFCTAG_INSTANCE, &writedata, memindex, 1 );	
	}	
}

int16_t checkdatainzonex(uint32_t memindex, ST25DV_I2C_PROT_ZONE pProtZone)
{
	uint16_t zonexstatus;
	uint8_t end1_addr; 
	uint8_t end2_addr;
	uint8_t end3_addr;
	uint32_t lastbyte_zone1 = 0;
	uint32_t lastbyte_zone2 = 0;
	uint32_t lastbyte_zone3 = 0;
	uint32_t lastbyte_zone4 = 0;	
	
	NFC04A1_NFCTAG_ReadEndZonex(NFC04A1_NFCTAG_INSTANCE, ST25DV_ZONE_END1, &end1_addr);
	lastbyte_zone1 = 32*end1_addr+31;	
	NFC04A1_NFCTAG_ReadEndZonex(NFC04A1_NFCTAG_INSTANCE, ST25DV_ZONE_END2, &end2_addr);	
	lastbyte_zone2 = 32*end2_addr+31;
	NFC04A1_NFCTAG_ReadEndZonex(NFC04A1_NFCTAG_INSTANCE, ST25DV_ZONE_END3, &end3_addr);
	lastbyte_zone3 = 32*end3_addr+31;
	/* Get ST25DV EEPROM size */
	NFC04A1_NFCTAG_ReadMemSize(NFC04A1_NFCTAG_INSTANCE, &st25dvmemsize); 
	/* st25dvmemsize is composed of Mem_Size (number of blocks) and BlockSize (size of each blocks in bytes) */
	st25dvbmsize = (st25dvmemsize.Mem_Size + 1) * (st25dvmemsize.BlockSize + 1);	
	lastbyte_zone4 = st25dvbmsize -1;	
	
	if(memindex <= lastbyte_zone1)
	{
		zonexstatus = pProtZone.ProtectZone1;
	}		
	if((memindex > lastbyte_zone1) && (memindex <= lastbyte_zone2))
	{ 
		zonexstatus = pProtZone.ProtectZone2;		
	} 
	if((memindex > lastbyte_zone2) && (memindex <= lastbyte_zone3))
	{ 
		zonexstatus = pProtZone.ProtectZone3;		
	} 	
	if((memindex > lastbyte_zone3) && (memindex <= lastbyte_zone4))
	{ 
		zonexstatus = pProtZone.ProtectZone4;		
	} 
	if(memindex > lastbyte_zone4)
	{ 
		//UARTConsolePrint( "\n\r\n\rInvalid address!" );
		return NFCTAG_ERROR;
	} 
	return zonexstatus;
}

#ifdef __cplusplus
}
#endif


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
