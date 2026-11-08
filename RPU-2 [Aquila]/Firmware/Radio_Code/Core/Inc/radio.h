/*
 * radio.h
 *
 *  Created on: Oct 4, 2026
 *      Author: samsonk
 */

#ifndef INC_RADIO_H_
#define INC_RADIO_H_
#include "radio_driver.h"
#include "usart.h"

#include <string.h>

#ifndef CALLSIGN
#error  "You must provide a callsign at Compile Time"
#endif

#define RF_FREQUENCY                                433000000 /* Hz */
#define TX_OUTPUT_POWER                             14        /* dBm */
#define FSK_FDEV                                    25000     /* Hz */
#define FSK_DATARATE                                50000     /* bps */
#define FSK_BANDWIDTH                               50000     /* Hz */
#define FSK_PREAMBLE_LENGTH                         2         /* Same for Tx and Rx */
#define FSK_SYNCWORD_LENGTH                         6
//#define FSK_FIX_LENGTH_PAYLOAD_ON                   false
//#define PAYLOAD_LEN                                 64

#define PACKET_START (0b111110 << 2)
#define PACKET_START_LEN 8


typedef enum {
	CONTROL_BOARD = 0,
	FILL_CONTROL = 1,
	DAQ_CONTROL = 2,
	RESERVED = 3
} board;

typedef enum
{
  STATE_NULL,
  STATE_MASTER,
  STATE_SLAVE
} state_t;

typedef enum
{
  SSTATE_NULL,
  SSTATE_RX,
  SSTATE_TX
} substate_t;

typedef struct
{
  state_t state;
  substate_t subState;
  uint32_t rxTimeout;
  uint32_t rxMargin;
  uint32_t randomDelay;
  char rxBuffer[RX_BUFFER_SIZE];
  uint8_t rxSize;
} pingPongFSM_t;


extern void (*volatile eventReceptor)(pingPongFSM_t *const fsm);
extern PacketParams_t packetParams;  // TODO: this is lazy...


void radioInit(void);
void RadioOnDioIrq(RadioIrqMasks_t radioIrq);
void eventTxDone(pingPongFSM_t *const fsm);
void eventRxDone(pingPongFSM_t *const fsm);
void eventTxTimeout(pingPongFSM_t *const fsm);
void eventRxTimeout(pingPongFSM_t *const fsm);
void eventRxError(pingPongFSM_t *const fsm);
void enterMasterRx(pingPongFSM_t *const fsm);
void enterSlaveRx(pingPongFSM_t *const fsm);
void enterMasterTx(pingPongFSM_t *const fsm);
void enterSlaveTx(pingPongFSM_t *const fsm);
void transitionRxDone(pingPongFSM_t *const fsm);

void setNodeAddress(uint8_t node_address);
#endif /* INC_RADIO_H_ */
