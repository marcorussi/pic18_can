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
 * This file can.h represents the header file of the CAN bus driver for 
 * PIC18 microcontrollers.
 * Author : Marco Russi
 * 
 * Evolution of the file:
 * 06/08/2015 - File created - Marco Russi
 * 
*/


/* ------------- Definition for mono-inclusion ---------------- */ 
#ifdef I_CAN
#else 
#define I_CAN 1




/* ----------------- Inclusions ------------------ */
#include "common.h"




/* -------------- Definition of exported constants -------------- */ 
#define CAN_MESSAGE_LENGTH_BYTES       8




/* -------------- Definition of exported types -------------- */ 

/* CAN responses codes enum */
typedef enum
{
   CAN_NO_ERRORS               
  ,CAN_ERRORS 
  ,CAN_MODULE_NOT_INITIALISED 
  ,CAN_MODULE_INITIALISED 
  ,CAN_TX_BUFFER_FULL
  ,CAN_RX_BUFFER_EMPTY
} CAN_eCanResponseCodes;

/* CAN Synch Jump With value enum */
typedef enum
{
   CAN_SYNCH_JUMP_WIDTH_1_TQ               
  ,CAN_SYNCH_JUMP_WIDTH_2_TQ
  ,CAN_SYNCH_JUMP_WIDTH_3_TQ
  ,CAN_SYNCH_JUMP_WIDTH_4_TQ  
} CAN_eCanSynchJumpWidth;

/* CAN Phase Segment 1 value enum */
typedef enum
{
   CAN_PHASE_SEGMENT_1_1_TQ              
  ,CAN_PHASE_SEGMENT_1_2_TQ 
  ,CAN_PHASE_SEGMENT_1_3_TQ 
  ,CAN_PHASE_SEGMENT_1_4_TQ 
  ,CAN_PHASE_SEGMENT_1_5_TQ 
  ,CAN_PHASE_SEGMENT_1_6_TQ 
  ,CAN_PHASE_SEGMENT_1_7_TQ 
  ,CAN_PHASE_SEGMENT_1_8_TQ 
} CAN_eCanPhaseSeg1;

/* CAN Phase Segment 2 value enum */
typedef enum
{
   CAN_PHASE_SEGMENT_2_1_TQ              
  ,CAN_PHASE_SEGMENT_2_2_TQ 
  ,CAN_PHASE_SEGMENT_2_3_TQ 
  ,CAN_PHASE_SEGMENT_2_4_TQ 
  ,CAN_PHASE_SEGMENT_2_5_TQ 
  ,CAN_PHASE_SEGMENT_2_6_TQ 
  ,CAN_PHASE_SEGMENT_2_7_TQ 
  ,CAN_PHASE_SEGMENT_2_8_TQ 
  ,CAN_PHASE_SEGMENT_2_MAXIMUM_OF_PHEG1
} CAN_eCanPhaseSeg2;

/* CAN Bus Line Sampling type enum */
typedef enum
{
   CAN_BUS_LINE_IS_SAMPLED_ONCE               
  ,CAN_BUS_LINE_IS_SAMPLED_THREE_TIMES
} CAN_eCanBusSample;

/* CAN Propagation value enum */
typedef enum
{
   CAN_PROPAGATION_1_TQ              
  ,CAN_PROPAGATION_2_TQ 
  ,CAN_PROPAGATION_3_TQ 
  ,CAN_PROPAGATION_4_TQ 
  ,CAN_PROPAGATION_5_TQ 
  ,CAN_PROPAGATION_6_TQ 
  ,CAN_PROPAGATION_7_TQ 
  ,CAN_PROPAGATION_8_TQ 
} CAN_eCanPropagation;

/* CAN Bus Line filtering enabling enum */
typedef enum
{
   CAN_BUS_LINE_FILTER_DISABLED   
  ,CAN_BUS_LINE_FILTER_ENABLED 
} CAN_eCanWakeUpFilter;




/* ------------- Definition of exported macros --------------- */ 

/* CAN interrupt flag */
#define CAN_INTERRUPT_FLAG 				PIR3bits.RXB0IF




/* -------------- Definition of exported types -------------- */ 

/* CAN init parameters structure */
typedef struct
{
  uchar                    ucBaudRate;
  CAN_eCanSynchJumpWidth   eSynchJumpWidth;
  CAN_eCanPhaseSeg1        ePhaseSeg1;
  CAN_eCanPhaseSeg2        ePhaseSeg2;
  CAN_eCanBusSample        eBusSample;
  CAN_eCanPropagation      ePropagation;
  CAN_eCanWakeUpFilter     eWakeUpFilter;
} CAN_stCanInitParameters;

/* CAN message structure */
typedef struct
{
   ulong    ulIdentifier;
   Boolean  fgExide;
   Boolean  fgRemote;
   uchar    ucDataLength;
   uchar    ucvData[CAN_MESSAGE_LENGTH_BYTES];
} CAN_stCanMessage;




/* ------------ Definition of exported constant data ------------- */ 
/* None */




/* --------------- Definition of exported variables --------------- */ 
/* None */




/* ------------------ Exported Functions -------------------- */ 
extern CAN_eCanResponseCodes CAN_InitCanModule( CAN_stCanInitParameters );
extern CAN_eCanResponseCodes CAN_CheckModuleStatus( void );
extern CAN_eCanResponseCodes CAN_SendMessage( CAN_stCanMessage * );
extern CAN_eCanResponseCodes CAN_ReceiveMessage( CAN_stCanMessage * );
extern CAN_eCanResponseCodes CAN_ManageQueue( void );
extern CAN_eCanResponseCodes CAN_ManageRxMessageInterrupt( void );
extern CAN_eCanResponseCodes CAN_SetIDMessageToReceive( uchar, ulong, Boolean);




#endif   




/* End of file */
