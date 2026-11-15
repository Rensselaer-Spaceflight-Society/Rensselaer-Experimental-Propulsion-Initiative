/*
 * radio.c
 *
 *  Created on: Oct 4, 2026
 *      Author: samsonk
 */

#include "radio.h"
#include "packet.h"



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
  packetParams.Params.Gfsk.AddrComp = RADIO_ADDRESSCOMP_FILT_OFF;
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
//  setNodeAddress(BOARD_ADDR);
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
  switch (fsm->subState)
  {
    case SSTATE_TX:
      enterRx(fsm);
      fsm->subState = SSTATE_RX;
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
	// Todo: Remove outter switch statement. Maybe use a def flag, if def mastered ifndef slave.
	// Todo: Access char buffer and maybe use that directly. pass this to packet.c.
  Command desiredCommand = NULL;
  int8_t packetCode = parsePacket(&desiredCommand, &fsm->rxBuffer[1], fsm->rxSize - 1);
  HAL_UART_Transmit(&huart2, (uint8_t *)"Event RX Done\r\n", 15, HAL_MAX_DELAY);
  switch (fsm->subState) {
    case SSTATE_RX:
      transitionRxDone(fsm);
      switch(packetCode) {
        case -1:
        case -2:
        case -3:
        // -3 = Packet not intended for us, go back to listening
          // Any other = error, wait for packet to be re-sent
          // TODO: log error for debugging?
          enterRx(fsm);
          HAL_UART_Transmit(&huart2, (uint8_t *)fsm->rxBuffer, strlen(fsm->rxBuffer), HAL_MAX_DELAY);
          break;
        default:
          // Packet intended for us, TODO: acknowledge and execute command. After rewriting parsePacket
          uint8_t *packet = NULL;
          uint8_t packetSize = createPacket(packet, CONTROL_BOARD, packetCode, 0, 0);
          enterTx(fsm, packet, packetSize);
          fsm->subState = SSTATE_TX;
          desiredCommand(&fsm->rxBuffer[2]);
          break;
      }
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

  switch (fsm->subState)
  {
    case SSTATE_TX:
      enterRx(fsm);
      fsm->subState = SSTATE_RX;
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
    switch (fsm->subState)
    {
      case SSTATE_RX:
        enterRx(fsm);
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
  switch (fsm->subState)
  {
    case SSTATE_RX:
      enterRx(fsm);
      break;
    default:
      break;
  }
}


/**
  * @brief  Entry actions for the RX sub-state
  * @param  fsm pointer to FSM context
  * @retval None
  */
void enterRx(pingPongFSM_t *const fsm)
{
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
void enterTx(pingPongFSM_t *const fsm, uint8_t* packet, uint8_t packetSize)
{
  HAL_Delay(fsm->rxMargin);

  HAL_UART_Transmit(&huart2, (uint8_t *)"Tx start\r\n", 17, HAL_MAX_DELAY);
  SUBGRF_SetDioIrqParams( IRQ_TX_DONE | IRQ_RX_TX_TIMEOUT,
                          IRQ_TX_DONE | IRQ_RX_TX_TIMEOUT,
                          IRQ_RADIO_NONE,
                          IRQ_RADIO_NONE );
  SUBGRF_SetSwitch(RFO_LP, RFSWITCH_TX);
  // Workaround 5.1 in DS.SX1261-2.W.APP (before each packet transmission)
  SUBGRF_WriteRegister(0x0889, (SUBGRF_ReadRegister(0x0889) | 0x04));
  packetParams.Params.Gfsk.PayloadLength = packetSize + 1; // TODO: Should +1? check if works
  SUBGRF_SetPacketParams(&packetParams);
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
