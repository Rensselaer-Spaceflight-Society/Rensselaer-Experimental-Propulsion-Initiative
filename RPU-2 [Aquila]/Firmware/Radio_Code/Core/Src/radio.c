/*
 * radio.c
 *
 *  Created on: Oct 4, 2026
 *      Author: samsonk
 */

#include "radio.h"
#include "usart.h"

#ifndef BOARD_ADDR
#error  "You must provide an address for this board at Compile Time"
#endif


void (*volatile eventReceptor)(pingPongFSM_t *const fsm);
PacketParams_t packetParams;  // TODO: this is lazy...

void radioInit(void)
{
  // Initialize the hardware (SPI bus, TCXO control, RF switch)
  SUBGRF_Init(RadioOnDioIrq);

  // Use DCDC converter if `DCDC_ENABLE` is defined in radio_conf.h
  // "By default, the SMPS clock detection is disabled and must be enabled before enabling the SMPS." (6.1 in RM0453)
  SUBGRF_WriteRegister(SUBGHZ_SMPSC0R, (SUBGRF_ReadRegister(SUBGHZ_SMPSC0R) | SMPS_CLK_DET_ENABLE));
  SUBGRF_SetRegulatorMode();

  // Use the whole 256-byte buffer for both TX and RX
  SUBGRF_SetBufferBaseAddress(0x00, 0x00);

  SUBGRF_SetRfFrequency(RF_FREQUENCY);
  SUBGRF_SetRfTxPower(TX_OUTPUT_POWER);
  SUBGRF_SetStopRxTimerOnPreambleDetect(false);

  SUBGRF_SetPacketType(PACKET_TYPE_GFSK);

  ModulationParams_t modulationParams;
  modulationParams.PacketType = PACKET_TYPE_GFSK;
//  modulationParams.Params.Gfsk.Bandwidth = SUBGRF_GetFskBandwidthRegValue(FSK_BANDWIDTH); We just use the register directly
  modulationParams.Params.Gfsk.Bandwidth = RX_BW_234300;
  modulationParams.Params.Gfsk.BitRate = FSK_DATARATE;
  modulationParams.Params.Gfsk.Fdev = FSK_FDEV;
  modulationParams.Params.Gfsk.ModulationShaping = MOD_SHAPING_G_BT_1;
  SUBGRF_SetModulationParams(&modulationParams);

  packetParams.PacketType = PACKET_TYPE_GFSK;
  packetParams.Params.Gfsk.AddrComp = RADIO_ADDRESSCOMP_FILT_NODE;
  packetParams.Params.Gfsk.CrcLength = RADIO_CRC_2_BYTES_CCIT;
  packetParams.Params.Gfsk.DcFree = RADIO_DC_FREEWHITENING;
  packetParams.Params.Gfsk.HeaderType = RADIO_PACKET_VARIABLE_LENGTH;
  packetParams.Params.Gfsk.PayloadLength = 0xFE;
  packetParams.Params.Gfsk.PreambleLength = (FSK_PREAMBLE_LENGTH << 3); // bytes to bits
  packetParams.Params.Gfsk.PreambleMinDetect = RADIO_PREAMBLE_DETECTOR_08_BITS; // 8 bits of preamble to look for syncword.
  packetParams.Params.Gfsk.SyncWordLength = (FSK_SYNCWORD_LENGTH << 3); // bytes to bits
  SUBGRF_SetPacketParams(&packetParams);

  SUBGRF_SetSyncWord(CALLSIGN);

  SUBGRF_SetWhiteningSeed(0x01FF);
  setNodeAddress(BOARD_ADDR);
//  setBroadcastAddress(0x01);
}


/**
  * @brief  Receive data trough SUBGHZSPI peripheral
  * @param  radioIrq  interrupt pending status information
  * @retval None
  */
void RadioOnDioIrq(RadioIrqMasks_t radioIrq)
{
  switch (radioIrq)
  {
    case IRQ_TX_DONE:
      eventReceptor = eventTxDone;
      break;
    case IRQ_RX_DONE:
      eventReceptor = eventRxDone;
      break;
    case IRQ_RX_TX_TIMEOUT:
      if (SUBGRF_GetOperatingMode() == MODE_TX)
      {
        eventReceptor = eventTxTimeout;
      }
      else if (SUBGRF_GetOperatingMode() == MODE_RX)
      {
        eventReceptor = eventRxTimeout;
      }
      break;
    case IRQ_CRC_ERROR:
      eventReceptor = eventRxError;
      break;
    default:
      break;
  }
}


/**
  * @brief  Process the TX Done event
  * @param  fsm pointer to FSM context
  * @retval None
  */
void eventTxDone(pingPongFSM_t *const fsm)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)"Event TX Done\r\n", 15, HAL_MAX_DELAY);
  switch (fsm->state)
  {
    case STATE_MASTER:
      switch (fsm->subState)
      {
        case SSTATE_TX:
          enterMasterRx(fsm);
          fsm->subState = SSTATE_RX;
          break;
        default:
          break;
      }
      break;
    case STATE_SLAVE:
      switch (fsm->subState)
      {
        case SSTATE_TX:
          enterSlaveRx(fsm);
          fsm->subState = SSTATE_RX;
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}


/**
  * @brief  Process the RX Done event
  * @param  fsm pointer to FSM context
  * @retval None
  */
void eventRxDone(pingPongFSM_t *const fsm)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)"Event RX Done\r\n", 15, HAL_MAX_DELAY);
  switch(fsm->state)
  {
    case STATE_MASTER:
      switch (fsm->subState)
      {
        case SSTATE_RX:
          transitionRxDone(fsm);
          if (strncmp(&fsm->rxBuffer[1], "PONG", 4) == 0)
          {
            BSP_LED_Off(LED_GREEN);
            BSP_LED_Toggle(LED_RED);
            enterMasterTx(fsm);
            fsm->subState = SSTATE_TX;
          }
          else if (strncmp(&fsm->rxBuffer[1], "PING", 4) == 0)
          {
            enterSlaveRx(fsm);
            fsm->state = STATE_SLAVE;
          }
          else
          {
            enterMasterRx(fsm);
            HAL_UART_Transmit(&huart2, (uint8_t *)fsm->rxBuffer, strlen(fsm->rxBuffer), HAL_MAX_DELAY);
          }
          break;
        default:
          break;
      }
      break;
    case STATE_SLAVE:
      switch (fsm->subState)
      {
        case SSTATE_RX:
          transitionRxDone(fsm);
          if (strncmp(&fsm->rxBuffer[1], "PING", 4) == 0)
          {
            BSP_LED_Off(LED_RED);
            BSP_LED_Toggle(LED_GREEN);
            enterSlaveTx(fsm);
            fsm->subState = SSTATE_TX;
          }
          else
          {
            enterMasterRx(fsm);
            fsm->state = STATE_MASTER;
          }
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}


/**
  * @brief  Process the TX Timeout event
  * @param  fsm pointer to FSM context
  * @retval None
  */
void eventTxTimeout(pingPongFSM_t *const fsm)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)"Event TX Timeout\r\n", 18, HAL_MAX_DELAY);
  switch (fsm->state)
  {
    case STATE_MASTER:
      switch (fsm->subState)
      {
        case SSTATE_TX:
          enterMasterRx(fsm);
          fsm->subState = SSTATE_RX;
          break;
        default:
          break;
      }
      break;
    case STATE_SLAVE:
      switch (fsm->subState)
      {
        case SSTATE_TX:
          enterSlaveRx(fsm);
          fsm->subState = SSTATE_RX;
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}


/**
  * @brief  Process the RX Timeout event
  * @param  fsm pointer to FSM context
  * @retval None
  */
void eventRxTimeout(pingPongFSM_t *const fsm)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)"Event RX Timeout\r\n", 18, HAL_MAX_DELAY);
  switch (fsm->state)
  {
    case STATE_MASTER:
      switch (fsm->subState)
      {
        case SSTATE_RX:
          HAL_Delay(fsm->randomDelay);
          enterMasterTx(fsm);
          fsm->subState = SSTATE_TX;
          break;
        default:
          break;
      }
      break;
    case STATE_SLAVE:
      switch (fsm->subState)
      {
        case SSTATE_RX:
          enterSlaveRx(fsm);
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}


/**
  * @brief  Process the RX Error event
  * @param  fsm pointer to FSM context
  * @retval None
  */
void eventRxError(pingPongFSM_t *const fsm)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)"Event Rx Error\r\n", 16, HAL_MAX_DELAY);
  switch (fsm->state)
  {
    case STATE_MASTER:
      switch (fsm->subState)
      {
        case SSTATE_RX:
          HAL_Delay(fsm->randomDelay);
          enterMasterTx(fsm);
          fsm->subState = SSTATE_TX;
          break;
        default:
          break;
      }
      break;
    case STATE_SLAVE:
      switch (fsm->subState)
      {
        case SSTATE_RX:
          enterSlaveRx(fsm);
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}


/**
  * @brief  Entry actions for the RX sub-state of the Master state
  * @param  fsm pointer to FSM context
  * @retval None
  */
void enterMasterRx(pingPongFSM_t *const fsm)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)"Master Rx start\r\n", 17, HAL_MAX_DELAY);
  SUBGRF_SetDioIrqParams( IRQ_RX_DONE | IRQ_RX_TX_TIMEOUT | IRQ_CRC_ERROR,
                          IRQ_RX_DONE | IRQ_RX_TX_TIMEOUT | IRQ_CRC_ERROR,
                          IRQ_RADIO_NONE,
                          IRQ_RADIO_NONE );
  SUBGRF_SetSwitch(RFO_LP, RFSWITCH_RX);
  packetParams.Params.Gfsk.PayloadLength = 0xFE;
  SUBGRF_SetPacketParams(&packetParams);
  SUBGRF_SetRx(fsm->rxTimeout << 6);
}


/**
  * @brief  Entry actions for the RX sub-state of the Slave state
  * @param  fsm pointer to FSM context
  * @retval None
  */
void enterSlaveRx(pingPongFSM_t *const fsm)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)"Slave Rx start\r\n", 16, HAL_MAX_DELAY);
  SUBGRF_SetDioIrqParams( IRQ_RX_DONE | IRQ_RX_TX_TIMEOUT | IRQ_CRC_ERROR,
                          IRQ_RX_DONE | IRQ_RX_TX_TIMEOUT | IRQ_CRC_ERROR,
                          IRQ_RADIO_NONE,
                          IRQ_RADIO_NONE );
  SUBGRF_SetSwitch(RFO_LP, RFSWITCH_RX);
  packetParams.Params.Gfsk.PayloadLength = 0xFE;
  SUBGRF_SetPacketParams(&packetParams);
  SUBGRF_SetRx(fsm->rxTimeout << 6);
}


/**
  * @brief  Entry actions for the TX sub-state of the Master state
  * @param  fsm pointer to FSM context
  * @retval None
  */
void enterMasterTx(pingPongFSM_t *const fsm)
{
  HAL_Delay(fsm->rxMargin);

  HAL_UART_Transmit(&huart2, (uint8_t *)"...PING\r\n", 9, HAL_MAX_DELAY);
  HAL_UART_Transmit(&huart2, (uint8_t *)"Master Tx start\r\n", 17, HAL_MAX_DELAY);
  SUBGRF_SetDioIrqParams( IRQ_TX_DONE | IRQ_RX_TX_TIMEOUT,
                          IRQ_TX_DONE | IRQ_RX_TX_TIMEOUT,
                          IRQ_RADIO_NONE,
                          IRQ_RADIO_NONE );
  SUBGRF_SetSwitch(RFO_LP, RFSWITCH_TX);
  // Workaround 5.1 in DS.SX1261-2.W.APP (before each packet transmission)
  SUBGRF_WriteRegister(0x0889, (SUBGRF_ReadRegister(0x0889) | 0x04));
  packetParams.Params.Gfsk.PayloadLength = 0x5;
  SUBGRF_SetPacketParams(&packetParams);
  uint8_t packet[] = { 0x00, 'P', 'I', 'N', 'G' };
  SUBGRF_SendPayload(packet, packetParams.Params.Gfsk.PayloadLength, 0);
}


/**
  * @brief  Entry actions for the TX sub-state of the Slave state
  * @param  fsm pointer to FSM context
  * @retval None
  */
void enterSlaveTx(pingPongFSM_t *const fsm)
{
  HAL_Delay(fsm->rxMargin);

  HAL_UART_Transmit(&huart2, (uint8_t *)"...PONG\r\n", 9, HAL_MAX_DELAY);
  HAL_UART_Transmit(&huart2, (uint8_t *)"Slave Tx start\r\n", 16, HAL_MAX_DELAY);
  SUBGRF_SetDioIrqParams( IRQ_TX_DONE | IRQ_RX_TX_TIMEOUT,
                          IRQ_TX_DONE | IRQ_RX_TX_TIMEOUT,
                          IRQ_RADIO_NONE,
                          IRQ_RADIO_NONE );
  SUBGRF_SetSwitch(RFO_LP, RFSWITCH_TX);
  // Workaround 5.1 in DS.SX1261-2.W.APP (before each packet transmission)
  SUBGRF_WriteRegister(0x0889, (SUBGRF_ReadRegister(0x0889) | 0x04));
  packetParams.Params.Gfsk.PayloadLength = 0x5;
  SUBGRF_SetPacketParams(&packetParams);

  uint8_t packet[] = { 0x00, 'P', 'O', 'N', 'G' };
  SUBGRF_SendPayload(packet, packetParams.Params.Gfsk.PayloadLength, 0);
}


/**
  * @brief  Transition actions executed on every RX Done event (helper function)
  * @param  fsm pointer to FSM context
  * @retval None
  */
void transitionRxDone(pingPongFSM_t *const fsm)
{
  PacketStatus_t packetStatus;
  int32_t cfo;
  char uartBuff[50];

  // Workaround 15.3 in DS.SX1261-2.W.APP (because following RX w/ timeout sequence)
  SUBGRF_WriteRegister(0x0920, 0x00);
  SUBGRF_WriteRegister(0x0944, (SUBGRF_ReadRegister(0x0944) | 0x02));

  SUBGRF_GetPayload((uint8_t *)fsm->rxBuffer, &fsm->rxSize, 0xFE);
  SUBGRF_GetPacketStatus(&packetStatus);
  SUBGRF_GetCFO(FSK_DATARATE, &cfo);

  sprintf(uartBuff, "RssiValue=%d dBm, Cfo=%ld Hz\r\n", packetStatus.Params.Gfsk.RssiAvg, cfo);
  HAL_UART_Transmit(&huart2, (uint8_t *)uartBuff, strlen(uartBuff), HAL_MAX_DELAY);
}

void setNodeAddress(uint8_t node_address) {
    SUBGRF_WriteRegisters(SUBGHZ_GNODEADDR, &node_address, 1 );
}
