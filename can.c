/*
 * The MIT License (MIT)
 * 
 * Copyright (c) [2015] [Marco Russi]
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

/*
 * This file can.c represents the only source file of the CAN bus driver for 
 * PIC18 microcontrollers.
 * 
 * Author : Marco Russi
 * 
 * Evolution of the file:
 * 06/08/2015 - File created - Marco Russi
 * 
*/


/* -------------- System and library files included ------------- */
#include "common.h"
#include "can.h"




/* ---------------- Other files included ---------------------- */
/* None */




/*-------------- Definition of local constants ---------------- */

/* Length of TX messages buffer */
#define TX_BUFFER_LENGTH_MESSAGES     			UC_5
/* Length of RX messages buffer */
#define RX_BUFFER_LENGTH_MESSAGES     			UC_5
/* Max number of acceptance filter */
#define UC_MAX_NUM_OF_ACCEPTANCE_FILTERS		UC_16




/* -------------- Definition of local macros ---------------- */
/* None */
     
     
     
                 
/* --------------- Definition of local types ---------------- */

/* Module info structure */
typedef struct
{
   Boolean  fgInitialised;
   uchar    ucTxErrorCounter;
   uchar    ucRxErrorCounter;
   uchar    ucWriteRequestIndex;
   uchar    ucWriteExecutionIndex;
   uchar    ucReadRequestIndex;
   uchar    ucReadExecutionIndex;
   uchar    ucWriteBufferCounter;
   uchar    ucReadBufferCounter;
} stInfo;




/* ------------ Definition of local constant data ------------- */ 
/* None */




/* ------------- Declaration of local variables  -------------- */

/* Message FIFO write buffer. Range: 0 - TX_BUFFER_LENGTH_MESSAGES */ 
CAN_stCanMessage stTxBuffer[TX_BUFFER_LENGTH_MESSAGES]; 

/* Message FIFO read buffer. Range: 0 - RX_BUFFER_LENGTH_MESSAGES */ 
CAN_stCanMessage stRxBuffer[RX_BUFFER_LENGTH_MESSAGES]; 

/* Store module informations */ 
stInfo stModuleInfo;        

/* Array of first register addres of acceptance filters */ 
uchar *apucFilterPointer[UC_MAX_NUM_OF_ACCEPTANCE_FILTERS] =
{ 
	 &RXF0SIDH
	,&RXF1SIDH
	,&RXF2SIDH
	,&RXF3SIDH
	,&RXF4SIDH
	,&RXF5SIDH
	,&RXF6SIDH
	,&RXF7SIDH
	,&RXF8SIDH
	,&RXF9SIDH
	,&RXF10SIDH
	,&RXF11SIDH
	,&RXF12SIDH
	,&RXF13SIDH
	,&RXF14SIDH
	,&RXF15SIDH
}



          
/* ------------------ Functions prototypes ------------------ */
LOCAL Boolean receiveCanMessage( CAN_stCanMessage * );
LOCAL Boolean sendCanMessage( CAN_stCanMessage * );
LOCAL void setCanConfigurationMode( void );
LOCAL void setCanNormalMode( void );




/* ------------------- Exported Functions -------------------- */

/* Initialise CAN module */
EXPORTED CAN_eCanResponseCodes CAN_InitCanModule( CAN_stCanInitParameters stParameters )
{
   CAN_eCanResponseCodes eResponseCode;

   setCanConfigurationMode();
   
   /* Reset status flags */
   COMSTAT = 0x00;
   
   /* Select Enhanced Legacy mode (Mode 1) */
   ECANCON |= 0b01000000;
   
   /* Set channels priority levels */
   /* TXB0 priority  = 2 */
   TXB0CONbits.TXPRI0 = 0;
   TXB0CONbits.TXPRI1 = 1;
   /* TXB0 priority  = 1 */
   TXB1CONbits.TXPRI0 = 1;
   TXB1CONbits.TXPRI1 = 0;
   /* TXB0 priority  = 0 */
   TXB2CONbits.TXPRI0 = 0;
   TXB2CONbits.TXPRI1 = 0;
   
   /* Configuration */
   BRGCON1 |= ((uchar)(stParameters.eSynchJumpWidth) & 0x03) << UC_6;
   BRGCON1 |= (uchar)(stParameters.ucBaudRate) & 0x3F;

   if(stParameters.ePhaseSeg2 == CAN_PHASE_SEGMENT_2_MAXIMUM_OF_PHEG1)
   {
      BRGCON2bits.SEG2PHTS = 0;
   }
   else
   {
      BRGCON2bits.SEG2PHTS = 1;
      BRGCON3 |= (uchar)stParameters.ePhaseSeg2 & 0x07;
   }  
   BRGCON2bits.SAM = stParameters.eBusSample; 
   BRGCON2 |= ((uchar)stParameters.ePhaseSeg1 & 0x07) << UC_3; 
   BRGCON2 |= (uchar)stParameters.ePropagation & 0x07;
   BRGCON3bits.WAKFIL = stParameters.eWakeUpFilter; 
   BRGCON3bits.WAKDIS = 0;	/* Enable CAN bus activity wake-up feature */ 
   /*
   sync jump   : 1 tq
   phase seg 1 : 8 tq
   phase seg 2 : 8 tq
   prog seg    : 3 tq
   bit time = ( 1 + 8 + 8 + 3 ) * TQ = 20 TQ
   TQ @ 100 Kbit/s = 10 us / 20 = 500 ns
   BRP = 0
   */
   BRGCON1 = 0x00;
   BRGCON2 = 0xBA;
   BRGCON3 = 0x07;
   
   /* Reset rxb0 control register. Receive all valid messages as per acceptance filters */
   RXB0CON = 0b00000000;
   /* Reset rxb1 control register. Receive all valid messages as per acceptance filters */
   RXB1CON = 0b00000000;
   
   /* No bits will be compared with incoming data bits */
   SDFLC = 0x00;
   
   /* All acceptance filters are associated with RXB0 */
   RXFBCON0 = 0x00;
   RXFBCON1 = 0x00;
   RXFBCON2 = 0x00;
   RXFBCON3 = 0x00;
   RXFBCON4 = 0x00;
   RXFBCON5 = 0x00;
   RXFBCON6 = 0x00;
   RXFBCON7 = 0x00;
   
   /* No mask are used */
   MSEL0 = 0xFF;
   MSEL1 = 0xFF;
   MSEL2 = 0xFF;
   MSEL3 = 0xFF;
   
   CIOCON = 0b00100000; 
   /*         ||||||||__   -   : Unimplemented (read as 0) */
   /*         |||||||___   -   : Unimplemented (read as 0) */
   /*         ||||||____   -   : Unimplemented (read as 0) */
   /*         |||||_____   -   : Unimplemented (read as 0) */
   /*         ||||______ CANCAP: Disable CAN capture, RC2/CCP1 input to CCP1 module */
   /*         |||_______ ENDRHI: CANTX pin will drive VDD when recessive */
   /*         ||________   -   : Unimplemented (read as 0) */
   /*         |_________   -   : Unimplemented (read as 0) */
   
   setCanNormalMode();
   
   IPR3 = 0x00;      /* All interrupts have low priority */
   PIR3 = 0x00;      /* Reset all interrupts flags */
   PIE3 = 0x01;      /* Enable rxb0 interrupts only */  
   
   /* Reset module status */
   stModuleInfo.fgInitialised = FG_TRUE;
   stModuleInfo.ucTxErrorCounter = UC_NULL;
   stModuleInfo.ucRxErrorCounter = UC_NULL;
   stModuleInfo.ucWriteRequestIndex = UC_NULL;
   stModuleInfo.ucWriteExecutionIndex = UC_NULL;
   stModuleInfo.ucReadRequestIndex = UC_NULL;
   stModuleInfo.ucReadExecutionIndex = UC_NULL;
   stModuleInfo.ucWriteBufferCounter = UC_NULL;
   stModuleInfo.ucReadBufferCounter = UC_NULL;
   
   eResponseCode = CAN_MODULE_INITIALISED;
   
   return eResponseCode;
}


/* Check the module status */
EXPORTED CAN_eCanResponseCodes CAN_CheckModuleStatus( void )
{
   CAN_eCanResponseCodes eResponseCode;
   
   /* If the module is initialised */
   if(stModuleInfo.fgInitialised == FG_TRUE)
   {
      /* Errors check */
      if((stModuleInfo.ucTxErrorCounter == UC_NULL) && (stModuleInfo.ucRxErrorCounter == UC_NULL))
      {
         eResponseCode = CAN_MODULE_INITIALISED;
      }
      else
      {
         eResponseCode = CAN_ERRORS;
      }
   }
   else
   {
      eResponseCode = CAN_MODULE_NOT_INITIALISED;
   }
   
   return eResponseCode;
}


/* Send a message */
EXPORTED CAN_eCanResponseCodes CAN_SendMessage( CAN_stCanMessage *pstMessage )
{
   CAN_eCanResponseCodes eResponseCode;
   
   /* If write buffer is not full */
   if(stModuleInfo.ucWriteBufferCounter < TX_BUFFER_LENGTH_MESSAGES)
   {
      /* Copy the message */
      stTxBuffer[stModuleInfo.ucWriteRequestIndex] = *pstMessage;
      
      /* Increment write buffer index */
      stModuleInfo.ucWriteRequestIndex++;
      if(stModuleInfo.ucWriteRequestIndex == TX_BUFFER_LENGTH_MESSAGES)
      {
         stModuleInfo.ucWriteRequestIndex = UC_NULL;
      }
      else
      {
      }
      
      /* Increment write requests counter */
      stModuleInfo.ucWriteBufferCounter++;
      
      eResponseCode = CAN_NO_ERRORS;
   }
   else
   {
      eResponseCode = CAN_TX_BUFFER_FULL;
   }   
   
   return eResponseCode;
}


/* Read a message */
EXPORTED CAN_eCanResponseCodes CAN_ReceiveMessage( CAN_stCanMessage *pstMessage )
{
   CAN_eCanResponseCodes eResponseCode;
   
   /* If read buffer is not empty */
   if(stModuleInfo.ucReadBufferCounter > UC_NULL)
   {
      /* Copy the message */
      *pstMessage = stRxBuffer[stModuleInfo.ucReadExecutionIndex];
      
      /* Increment read buffer index */
      stModuleInfo.ucReadExecutionIndex++;
      if(stModuleInfo.ucReadExecutionIndex == RX_BUFFER_LENGTH_MESSAGES)
      {
         stModuleInfo.ucReadExecutionIndex = UC_NULL;
      }
      else
      {
      }
      
      /* Decrement read requests counter */
      stModuleInfo.ucReadBufferCounter--;
      
      eResponseCode = CAN_NO_ERRORS;
   }
   else
   {
      eResponseCode = CAN_RX_BUFFER_EMPTY;
   }
   
   return eResponseCode;
}


/* Manage the can module queue */
EXPORTED CAN_eCanResponseCodes CAN_ManageQueue( void )
{
   CAN_eCanResponseCodes eResponseCode;
   Boolean fgSendSuccess = FG_FALSE;

   /* If there are not tx errors */
   if(stModuleInfo.ucTxErrorCounter == UC_NULL)
   {
      /* If there are write requests to be executed */
      if(stModuleInfo.ucWriteBufferCounter > UC_NULL)
      {
         /* Send the message pointed by current write execution index */
         fgSendSuccess = sendCanMessage(&stTxBuffer[stModuleInfo.ucWriteExecutionIndex]);
         
         /* If one can channel was free */
         if(fgSendSuccess == FG_TRUE)
         {
            /* Increment write execution index */
            stModuleInfo.ucWriteExecutionIndex++;
            if(stModuleInfo.ucWriteExecutionIndex == TX_BUFFER_LENGTH_MESSAGES)
            {
               stModuleInfo.ucWriteExecutionIndex = UC_NULL;
            }
            else
            {
            }
            
            /* Decrement write requests counter */
            stModuleInfo.ucWriteBufferCounter--;
         }
         else
         {
            /* Do nothing */
         }
      }
      else
      {
         /* Do nothing */
      }
      eResponseCode = CAN_NO_ERRORS;
   }
   else
   {
      eResponseCode = CAN_ERRORS;
   }
   
   return eResponseCode;
}


/* Manage RX message interrupt */
EXPORTED CAN_eCanResponseCodes CAN_ManageRxMessageInterrupt( void )
{
   CAN_eCanResponseCodes eResponseCode;
   Boolean fgReceiveSuccess = FG_FALSE;

   /* If there are not tx errors */
   if(stModuleInfo.ucRxErrorCounter == UC_NULL)
   {
      /* If the read buffer is not full */
      if(stModuleInfo.ucReadBufferCounter < RX_BUFFER_LENGTH_MESSAGES)
      {
         /* Read the message pointed by current read request index */
         fgReceiveSuccess = receiveCanMessage(&stRxBuffer[stModuleInfo.ucReadRequestIndex]);
         
         /* If a channel was read successfully */
         if(fgReceiveSuccess == FG_TRUE)
         {
            /* Increment read request index */
            stModuleInfo.ucReadRequestIndex++;
            if(stModuleInfo.ucReadRequestIndex == RX_BUFFER_LENGTH_MESSAGES)
            {
               stModuleInfo.ucReadRequestIndex = UC_NULL;
            }
            else
            {
            }
            
            /* Increment read requests counter */
            stModuleInfo.ucReadBufferCounter++;
         }
         else
         {
            /* Do nothing */
         }
      }
      else
      {
			/* Overwrite the message pointed by current read request index */
         fgReceiveSuccess = receiveCanMessage(&stRxBuffer[stModuleInfo.ucReadRequestIndex]);  
      }
      eResponseCode = CAN_NO_ERRORS;
   }
   else
   {
      eResponseCode = CAN_ERRORS;
   }
   
   return eResponseCode;
}


/* Set a ID message to receive */
EXPORTED CAN_eCanResponseCodes CAN_SetIDMessageToReceive( uchar ucIDIndex, ulong ulIdentifier, Boolean fgExide)
{
	CAN_eCanResponseCodes eResponseCode = CAN_NO_ERRORS;
	uchar *pucFilterPointer;

	/* Copy the first register address of the required acceptance filter */
	pucFilterPointer = apucFilterPointer[ucIDIndex];
	
	/* Set configuration mode */
	setCanConfigurationMode();

	/* Copy identifier */
	if(fgExide == B_TRUE)
	{
		 *pucFilterPointer = (uchar)(ulIdentifier >> UL_SHIFT_21);
		 pucFilterPointer++;
		 *pucFilterPointer = (uchar)(ulIdentifier >> UL_SHIFT_16);
		 (*pucFilterPointer) &= 0x03;
		 (*pucFilterPointer) |= (uchar)(ulIdentifier >> UL_SHIFT_13);
		 /* Set EXIDE bit */
		 (*pucFilterPointer) |= 0x08;                                  
		 pucFilterPointer++;
		 *pucFilterPointer = (uchar)(ulIdentifier >> UL_SHIFT_8);
		 pucFilterPointer++;
		 *pucFilterPointer = (uchar)(ulIdentifier);   
	}
	else
	{
	     *pucFilterPointer = (uchar)(ulIdentifier >> UL_SHIFT_3);
		 pucFilterPointer++;
		 *pucFilterPointer = (uchar)(ulIdentifier << UL_SHIFT_5);
		 (*pucFilterPointer) &= 0xE0;
		 pucFilterPointer++;
		 pucFilterPointer++;
	}
	
	/* Set corrispective filter enable bit */
	switch(ucIDIndex)
	{
		case UC_0:
		{
			RXFCON0bits.RXF0EN = 1;
			
			break;
		}
		case UC_1:
		{
			RXFCON0bits.RXF1EN = 1;
			
			break;
		}
		case UC_2:
		{
			RXFCON0bits.RXF2EN = 1;
			
			break;
		}
		case UC_3:
		{
			RXFCON0bits.RXF3EN = 1;
			
			break;
		}
		case UC_4:
		{
			RXFCON0bits.RXF4EN = 1;
			
			break;
		}
		case UC_5:
		{
			RXFCON0bits.RXF5EN = 1;
			
			break;
		}
		case UC_6:
		{
			RXFCON0bits.RXF6EN = 1;
			
			break;
		}
		case UC_7:
		{
			RXFCON0bits.RXF7EN = 1;
			
			break;
		}
		case UC_8:
		{
			RXFCON1bits.RXF8EN = 1;
			
			break;
		}
		case UC_9:
		{
			RXFCON1bits.RXF9EN = 1;
			
			break;
		}
		case UC_10:
		{
			RXFCON1bits.RXF10EN = 1;
			
			break;
		}
		case UC_11:
		{
			RXFCON1bits.RXF11EN = 1;
			
			break;
		}
		case UC_12:
		{
			RXFCON1bits.RXF12EN = 1;
			
			break;
		}
		case UC_13:
		{
			RXFCON1bits.RXF13EN = 1;
			
			break;
		}
		case UC_14:
		{
			RXFCON1bits.RXF14EN = 1;
			
			break;
		}
		case UC_15:
		{
			RXFCON1bits.RXF15EN = 1;
			
			break;
		}
		default:
		{
			break;
		}
	}
	  
	/* Set normal mode */
	setCanNormalMode();

	return eResponseCode;
}




/* ------------------- Local Functions --------------------- */

/* Receive a message from CAN channels */
LOCAL Boolean receiveCanMessage( CAN_stCanMessage *pstMessage )
{
   uchar * pucChannelControlRegister;
   uchar * pucChannelPointer;
   uchar ucDataIndex;
   
   Boolean fgSuccess = FG_TRUE;
   
   /* Check the full channel */
   if(RXB0CONbits.RXFUL == 1)
   {
      pucChannelControlRegister = &RXB0CON; 
   }
   else if(RXB1CONbits.RXFUL == 1)
   {
      pucChannelControlRegister = &RXB1CON;
   }
   else
   {
      /* There are not full channels */
      fgSuccess = FG_FALSE;
   }
   
   if(fgSuccess == FG_TRUE)
   {
      /* The control register is the first register order by address */
      pucChannelPointer = pucChannelControlRegister;                 
      pucChannelPointer++;
      pucChannelPointer++;
      
      /* Copy identifier */
      /* if extended identifier */
      if(((*pucChannelPointer) & 0x08) == 0x08)
      {
         pucChannelPointer--;
         pstMessage->ulIdentifier = ((ulong)(*pucChannelPointer) & 0x000000FF) << UL_SHIFT_21;
         pucChannelPointer++;
         pstMessage->ulIdentifier |= ((ulong)(*pucChannelPointer) & 0x000000E0) << UL_SHIFT_13;
         pstMessage->ulIdentifier |= ((ulong)(*pucChannelPointer) & 0x00000003) << UL_SHIFT_16;
         pucChannelPointer++;
         pstMessage->ulIdentifier |= (*pucChannelPointer) << UL_SHIFT_8;
         pucChannelPointer++;
         pstMessage->ulIdentifier |= (*pucChannelPointer);
         /* Set EXIDE bit */
         pstMessage->fgExide = FG_TRUE;
      }
      else
      {
         pucChannelPointer--;
         pstMessage->ulIdentifier = ((ulong)(*pucChannelPointer) & 0x000000FF) << UL_SHIFT_3;
         pucChannelPointer++;
         pstMessage->ulIdentifier |= ((ulong)(*pucChannelPointer) & 0x000000E0) >> UL_SHIFT_5;
         pucChannelPointer++;
         pucChannelPointer++;
         /* Clear EXIDE bit */
         pstMessage->fgExide = FG_FALSE;
      }
      pucChannelPointer++;
      
      /* Data Length Code update */
      pstMessage->ucDataLength = (*pucChannelPointer) & 0x0F;
      
      /* Remote flag update */
      if(((*pucChannelControlRegister) & 0x04) == 0x04)
      {
         pstMessage->fgRemote = FG_TRUE;
      }
      else
      {
         pstMessage->fgRemote = FG_FALSE;
      }
      
      /* Copy data */
      for(ucDataIndex = UC_NULL; ucDataIndex < pstMessage->ucDataLength; ucDataIndex++)
      {
         pucChannelPointer++;
         pstMessage->ucvData[ucDataIndex] = (*pucChannelPointer);
      }
      
      /* Clear buffer full flag */
      (*pucChannelControlRegister) &= 0x7F;
   }
   else
   {
      /* Do nothing */
   }
   
   return fgSuccess;
}


/* Send a message via CAN channels */
LOCAL Boolean sendCanMessage( CAN_stCanMessage *pstMessage )
{
   /* Save pointed message indentifier and DLC */
   ulong ulIdentifier = pstMessage->ulIdentifier;
   uchar ucDataLength = pstMessage->ucDataLength; 
   
   uchar *pucChannelControlRegister;
   uchar *pucChannelPointer;
   uchar ucDataIndex;
   
   Boolean fgSuccess = FG_TRUE;
   
   /* Data length control */
   if(ucDataLength > CAN_MESSAGE_LENGTH_BYTES)
   {
      ucDataLength = CAN_MESSAGE_LENGTH_BYTES;
   }
   else
   {
      /* Do nothing */
   }
   
   /* Check a free channel for send a message according to priority level order */
   if(TXB0CONbits.TXREQ == 0)
   {
      pucChannelControlRegister = &TXB0CON; 
   }
   else if(TXB1CONbits.TXREQ == 0)
   {
      pucChannelControlRegister = &TXB1CON;
   }
   else if(TXB2CONbits.TXREQ == 0)
   {
      pucChannelControlRegister = &TXB2CON;
   }
   else
   {
      /* There are not free channels */
      fgSuccess = FG_FALSE;
   }
   
   if(fgSuccess == FG_TRUE)
   {
      /* The control register is the first register order by address */
      pucChannelPointer = pucChannelControlRegister;                 
      pucChannelPointer++;
      
      /* Copy identifier */
      if(pstMessage->fgExide == B_TRUE)
      {
         *pucChannelPointer = (uchar)(ulIdentifier >> UL_SHIFT_21);
         pucChannelPointer++;
         *pucChannelPointer = (uchar)(ulIdentifier >> UL_SHIFT_16);
         (*pucChannelPointer) &= 0x03;
         (*pucChannelPointer) |= (uchar)(ulIdentifier >> UL_SHIFT_13);
         /* Set EXIDE bit */
         (*pucChannelPointer) |= 0x08;                                  
         pucChannelPointer++;
         *pucChannelPointer = (uchar)(ulIdentifier >> UL_SHIFT_8);
         pucChannelPointer++;
         *pucChannelPointer = (uchar)(ulIdentifier);   
      }
      else
      {
         *pucChannelPointer = (uchar)(ulIdentifier >> UL_SHIFT_3);
         pucChannelPointer++;
         *pucChannelPointer = (uchar)(ulIdentifier << UL_SHIFT_5);
         (*pucChannelPointer) &= 0xE0;
         pucChannelPointer++;
         pucChannelPointer++;
      }
      pucChannelPointer++;
      
      /* Data Length Code update */
      *pucChannelPointer = ucDataLength;

      /* Remote flag update */
      if(pstMessage->fgRemote == FG_FALSE)
      {
         (*pucChannelPointer) |= 0x40;
      }
      else
      {
         (*pucChannelPointer) &= 0x0F;
      }
      
      /* Copy data */
      for(ucDataIndex = UC_NULL; ucDataIndex < ucDataLength; ucDataIndex++)
      {
         pucChannelPointer++;
         *pucChannelPointer = pstMessage->ucvData[ucDataIndex];
      }
      
      /* Send request */
      (*pucChannelControlRegister) |= 0x08;
   }
   else
   {
      /* Do nothing */
   }
   
   return fgSuccess;
}


/* Set CAN registers in configuration mode */
LOCAL void setCanConfigurationMode( void )
{
   /* Configuration mode request */
   CANCONbits.REQOP0 = 0;                
   CANCONbits.REQOP1 = 0;
   CANCONbits.REQOP2 = 1;

   /* wait configuration mode request */
   while((CANSTAT & 0xE0) != 0b10000000);
}


/* Set CAN registers in normal mode */
LOCAL void setCanNormalMode( void )
{
   /* Normal mode request */
   CANCONbits.REQOP0 = 0;                
   CANCONbits.REQOP1 = 0;
   CANCONbits.REQOP2 = 0;
   
   /* wait normal mode request */
   while((CANSTAT & 0xE0) != 0b00000000);
}




/* End of file */

