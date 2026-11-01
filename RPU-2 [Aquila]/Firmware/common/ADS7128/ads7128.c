/**
 * @file ads7128.c
 * @author Cooper Werner (cooperwerner824@gmail.com)
 * @brief A Simple I2C driver for the ADS7128 from Texas Instruments
 * @version 0.1
 * @date 2026-09-20
 *
 * @copyright Copyright (c) 2026
 *
 * MIT License
 *
 * Copyright (c) 2026 Cooper Werner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "ads7128.h"
#include <stdint.h>

#define ADS7128_OPCODE_SINGLE_READ 0x10
#define ADS7128_OPCODE_SINGLE_WRITE 0x08
#define ADS7128_OPCODE_SET_BIT 0x18
#define ADS7128_OPCODE_CLEAR_BIT 0x20
#define ADS7128_OPCODE_BLOCK_READ 0x30
#define ADS7128_OPCODE_BLOCK_WRITE 0x28

#define ADS7128_REG_ALERT_CH_SEL 0x14
#define ADS7128_REG_ALERT_PIN_CFG 0x17
#define ADS7128_REG_GENERAL_CFG 0x01
#define ADS7128_REG_GPIO_CFG 0x07
#define ADS7128_REG_GPO_DRIVE_CFG 0x09
#define ADS7128_REG_GPO_VALUE 0x0B
#define ADS7128_REG_GPI_VALUE 0x0D
#define ADS7128_REG_CHANNEL_SEL 0x11
#define ADS7128_REG_EVENT_HIGH_FLAG 0x1A
#define ADS7128_REG_EVENT_LOW_FLAG 0x1C
#define ADS7128_REG_EVENT_RGN 0x1E
#define ADS7128_REG_HYSTERESIS_CH0 0x20
#define ADS7128_REG_HIGH_TH_CH0 0x21
#define ADS7128_REG_EVENT_COUNT_CH0 0x22
#define ADS7128_REG_LOW_TH_CH0 0x23
#define ADS7128_REG_SEQUENCE_CFG 0x25
#define AD7128_REG_MAX_LSB_CH0 0x60
#define AD7128_REG_MIN_LSB_CH0 0x80
#define AD7128_REG_PREV_LSB_CH0 0xA0

static HAL_StatusTypeDef
ADS7128_WriteRegister(const ADS7128_HandleTypeDef *hadc, uint8_t reg,
                      uint8_t value) {
  return HAL_I2C_Mem_Write(hadc->i2c, hadc->address << 1,
                           ((ADS7128_OPCODE_SINGLE_WRITE << 8) | reg),
                           I2C_MEMADD_SIZE_16BIT, &value, 1, hadc->max_delay);
}

static HAL_StatusTypeDef ADS7128_ReadRegister(const ADS7128_HandleTypeDef *hadc,
                                              uint8_t reg, uint8_t *value) {
  return HAL_I2C_Mem_Read(hadc->i2c, hadc->address << 1,
                          ((ADS7128_OPCODE_SINGLE_READ << 8) | reg),
                          I2C_MEMADD_SIZE_16BIT, value, 1, hadc->max_delay);
}

static HAL_StatusTypeDef
ADS7128_UpdateRegisterBits(const ADS7128_HandleTypeDef *hadc, uint8_t reg,
                           uint8_t mask, uint8_t set_bits) {
  uint8_t opcode = set_bits ? ADS7128_OPCODE_SET_BIT : ADS7128_OPCODE_CLEAR_BIT;
  return HAL_I2C_Mem_Write(hadc->i2c, hadc->address << 1, ((opcode << 8) | reg),
                           I2C_MEMADD_SIZE_16BIT, &mask, 1, hadc->max_delay);
}

HAL_StatusTypeDef ADS7128_Init(const ADS7128_HandleTypeDef *hadc) {
  uint8_t data[7] = {0, 0, 0, 0, 0, 0, 0};

  // GENERAL_CFG
  data[0] = (hadc->Init.digital_window_comparator.enable << 4) |
            (hadc->Init.statistics_enable << 5);

  // DATA_CFG
  // Append Channel ID to conversion
  data[1] = (0b01 << 4);

  // OSR_CFG
  // Set Oversampling
  data[2] = hadc->Init.oversampling;

  // OPMODE_CFG
  // Set Operating Mode
  data[3] = ((hadc->Init.mode == ADS7128_ADC_AUTONOMOUS_MODE) ? (1 << 5) : 0) |
            (hadc->Init.clk_div | (hadc->Init.osc_select << 4));

  // PIN_CFG
  // Set Pin Configuration (Analog / Digital)
  for (int i = 0; i < 8; i++) {
    uint8_t is_digital = hadc->Init.pin_cfg[i] != 0 ? 1 : 0;
    data[4] |= ((is_digital) << i);
  }

  HAL_StatusTypeDef status = HAL_I2C_Mem_Write(
      hadc->i2c, hadc->address << 1, ((ADS7128_OPCODE_BLOCK_WRITE << 8) | 0x01),
      I2C_MEMADD_SIZE_16BIT, data, 5, hadc->max_delay);

  if (status != HAL_OK)
    return status;

  // Configure ALERT pin drive and assertion logic.
  data[0] =
      (uint8_t)((hadc->Init.digital_window_comparator.alert_drive << 2) |
                (hadc->Init.digital_window_comparator.alert_logic & 0x03));
  status = ADS7128_WriteRegister(hadc, ADS7128_REG_ALERT_PIN_CFG, data[0]);
  if (status != HAL_OK)
    return status;

  // Set Digital Input / Output
  for (int i = 0; i < 8; i++) {
    uint8_t is_output = (hadc->Init.pin_cfg[i] & 0b10) != 0 ? 1 : 0;
    data[5] |= (is_output << i);
  }

  status = ADS7128_WriteRegister(hadc, ADS7128_REG_GPIO_CFG, data[4]);

  if (status != HAL_OK)
    return status;

  // Set Push-Pull or Open-Drain
  for (int i = 0; i < 8; i++) {
    uint8_t is_push_pull = (hadc->Init.pin_cfg[i] & 0b01);
    data[6] |= (is_push_pull << i);
  }

  status = ADS7128_WriteRegister(hadc, ADS7128_REG_GPO_DRIVE_CFG, data[6]);

  if (status != HAL_OK)
    return status;

  if (hadc->Init.mode == ADS7128_ADC_MANUAL_MODE)
    return status;

  // If in sequential or autonomous mode, we need to do a bit more setup

  // SEQUENCE_CFG
  // Set the sequence mode and enable sequencing
  data[0] = (1 << 4) | (0b01 << 0);

  // CHANNEL_SEL
  // Only used in manual mode, can ignore
  data[1] = 0;

  // AUTO_SEQ_CH_SEL
  // Configure all analog inputs to be included in the channel
  data[2] = (uint8_t)~data[3];

  status = HAL_I2C_Mem_Write(hadc->i2c, hadc->address << 1,
                             ((ADS7128_OPCODE_BLOCK_WRITE << 8) | 0x10),
                             I2C_MEMADD_SIZE_16BIT, data, 3, hadc->max_delay);

  return status;
}

HAL_StatusTypeDef ADS7128_Calibrate(const ADS7128_HandleTypeDef *hadc) {
  uint8_t data = 0b10;

  // Write to the calibration bit
  HAL_StatusTypeDef status =
      ADS7128_UpdateRegisterBits(hadc, ADS7128_REG_GENERAL_CFG, data, 1);

  if (status != HAL_OK)
    return status;

  // Blocking wait until calibration bit is unset
  while (data) {
    status = ADS7128_ReadRegister(hadc, 0x01, &data);
    if (status != HAL_OK)
      return status;

    data &= 0b10;
  }

  return HAL_OK;
}

HAL_StatusTypeDef
ADS7128_ManualSetAnalogChannel(const ADS7128_HandleTypeDef *hadc,
                               ADS7128_PinTypeDef channel) {
  // ! If we ever end up using the Zero-Crossing-Detection (ZCD) this will screw
  // ! it up and will need to be changed. Ok for now tho.
  return ADS7128_WriteRegister(hadc, ADS7128_REG_CHANNEL_SEL, channel);
}

HAL_StatusTypeDef
ADS7128_AnalogGetConversion(const ADS7128_HandleTypeDef *hadc, uint16_t *value,
                            ADS7128_PinTypeDef *channel_read) {
  uint8_t data[3] = {0, 0, 0};
  uint8_t oversampling =
      hadc->Init.oversampling != ADS7128_OVERSAMPLING_DISABLED;

  HAL_StatusTypeDef status =
      HAL_I2C_Master_Receive(hadc->i2c, hadc->address << 1, data,
                             oversampling ? 3 : 2, hadc->max_delay);

  if (status != HAL_OK)
    return status;

  if (!oversampling) {
    if (value != NULL) {
      *value = (uint16_t)data[0];
      *value = *value << 4;
      *value += (data[1] >> 4);
    }

    if (channel_read != NULL) {
      *channel_read = data[1] & 0x0F;
    }
  } else {
    if (value != NULL) {
      *value = ((uint16_t)data[0] << 8) + data[1];
    }

    if (channel_read != NULL) {
      *channel_read = data[2] >> 4;
    }
  }

  return status;
}


HAL_StatusTypeDef ADS7128_SetAutonomousMode(const ADS7128_HandleTypeDef *hadc,
                                            ADS7128_AutoModeTypeDef on_off) {
  return ADS7128_UpdateRegisterBits(hadc, ADS7128_REG_SEQUENCE_CFG, (1 << 4),
                                    on_off);
}


HAL_StatusTypeDef
ADS7128_AnalogGetPreviousConversion(const ADS7128_HandleTypeDef *hadc,
                                    ADS7128_PinTypeDef channel_to_read,
                                    uint16_t *value) {
  if (channel_to_read > ADS7128_ADC_CHANNEL_7)
    return HAL_ERROR;

  uint8_t reg_offset = (uint8_t)(channel_to_read * 2U);
  uint8_t data[2] = {0, 0};
  HAL_StatusTypeDef status =
      HAL_I2C_Mem_Read(hadc->i2c, hadc->address << 1,
                       ((ADS7128_OPCODE_BLOCK_READ << 8) |
                        (AD7128_REG_PREV_LSB_CH0 + reg_offset)),
                       I2C_MEMADD_SIZE_16BIT, data, 2, hadc->max_delay);
  return status;
}


HAL_StatusTypeDef
ADS7128_AnalogGetMaximumValue(const ADS7128_HandleTypeDef *hadc,
                              ADS7128_PinTypeDef channel_to_read,
                              uint16_t *value) {
  if (channel_to_read > ADS7128_ADC_CHANNEL_7)
    return HAL_ERROR;

  uint8_t reg_offset = (uint8_t)(channel_to_read * 2U);
  uint8_t data[2] = {0, 0};
  HAL_StatusTypeDef status =
      HAL_I2C_Mem_Read(hadc->i2c, hadc->address << 1,
                       ((ADS7128_OPCODE_BLOCK_READ << 8) |
                        (AD7128_REG_MAX_LSB_CH0 + reg_offset)),
                       I2C_MEMADD_SIZE_16BIT, data, 2, hadc->max_delay);
  return status;
}


HAL_StatusTypeDef
ADS7128_AnalogGetMinimumValue(const ADS7128_HandleTypeDef *hadc,
                              ADS7128_PinTypeDef channel_to_read,
                              uint16_t *value) {
  if (channel_to_read > ADS7128_ADC_CHANNEL_7)
    return HAL_ERROR;

  uint8_t reg_offset = (uint8_t)(channel_to_read * 2U);
  uint8_t data[2] = {0, 0};
  HAL_StatusTypeDef status =
      HAL_I2C_Mem_Read(hadc->i2c, hadc->address << 1,
                       ((ADS7128_OPCODE_BLOCK_READ << 8) |
                        (AD7128_REG_MIN_LSB_CH0 + reg_offset)),
                       I2C_MEMADD_SIZE_16BIT, data, 2, hadc->max_delay);
  return status;
}

ADS7128_PinStateTypeDef
ADS7128_ReadDigitalInput(const ADS7128_HandleTypeDef *hadc,
                         ADS7128_PinTypeDef pin) {
  uint8_t data = 0;
  HAL_StatusTypeDef status =
      ADS7128_ReadRegister(hadc, ADS7128_REG_GPI_VALUE, &data);

  if (status != HAL_OK)
    return 255;

  if ((data & (1 << pin)) != 0)
    return 1;
  else
    return 0;
}

HAL_StatusTypeDef ADS7128_WriteDigitalOutput(const ADS7128_HandleTypeDef *hadc,
                                             ADS7128_PinTypeDef pin,
                                             ADS7128_PinStateTypeDef state) {
  if (pin > ADS7128_ADC_CHANNEL_7)
    return HAL_ERROR;

  return ADS7128_UpdateRegisterBits(hadc, ADS7128_REG_GPO_VALUE,
                                    (uint8_t)(1U << pin),
                                    state != ADS7128_GPIO_RESET);
}

HAL_StatusTypeDef ADS7128_EnableChannelAlert(const ADS7128_HandleTypeDef *hadc,
                                             ADS7128_PinTypeDef channel,
                                             uint8_t enable) {
  if (channel > ADS7128_ADC_CHANNEL_7)
    return HAL_ERROR;

  return ADS7128_UpdateRegisterBits(hadc, ADS7128_REG_ALERT_CH_SEL,
                                    (uint8_t)(1U << channel), enable != 0);
}

HAL_StatusTypeDef ADS7128_ConfigureChannelComparator(
    const ADS7128_HandleTypeDef *hadc, ADS7128_PinTypeDef channel,
    uint16_t low_threshold, uint16_t high_threshold, uint8_t hysteresis,
    uint8_t event_region, uint8_t event_count) {
  if ((channel > ADS7128_ADC_CHANNEL_7) ||
      (low_threshold > ADS7128_MAX_THRESHOLD) ||
      (high_threshold > ADS7128_MAX_THRESHOLD) || (hysteresis > 0x0F) ||
      (low_threshold > high_threshold) ||
      (event_region > ADS7128_EVENT_REGION_INSIDE) ||
      (event_count < ADS7128_EVENT_COUNT_MIN) ||
      (event_count > ADS7128_EVENT_COUNT_MAX))
    return HAL_ERROR;

  uint8_t reg_offset = (uint8_t)(channel * 4U);
  uint8_t data = (uint8_t)(((high_threshold & 0x0F) << 4) | hysteresis);
  HAL_StatusTypeDef status = ADS7128_WriteRegister(
      hadc, (uint8_t)(ADS7128_REG_HYSTERESIS_CH0 + reg_offset), data);
  if (status != HAL_OK)
    return status;

  data = (uint8_t)(high_threshold >> 4);
  status = ADS7128_WriteRegister(
      hadc, (uint8_t)(ADS7128_REG_HIGH_TH_CH0 + reg_offset), data);
  if (status != HAL_OK)
    return status;

  // Hardware stores the number of additional samples after the first one.
  data = (uint8_t)(((low_threshold & 0x0F) << 4) | (event_count - 1U));
  status = ADS7128_WriteRegister(
      hadc, (uint8_t)(ADS7128_REG_EVENT_COUNT_CH0 + reg_offset), data);
  if (status != HAL_OK)
    return status;

  data = (uint8_t)(low_threshold >> 4);
  status = ADS7128_WriteRegister(
      hadc, (uint8_t)(ADS7128_REG_LOW_TH_CH0 + reg_offset), data);
  if (status != HAL_OK)
    return status;

  uint8_t event_regions = 0;
  status = ADS7128_ReadRegister(hadc, ADS7128_REG_EVENT_RGN, &event_regions);
  if (status != HAL_OK)
    return status;

  uint8_t channel_mask = (uint8_t)(1U << channel);
  if (event_region == ADS7128_EVENT_REGION_INSIDE)
    event_regions |= channel_mask;
  else
    event_regions &= (uint8_t)~channel_mask;

  return ADS7128_WriteRegister(hadc, ADS7128_REG_EVENT_RGN, event_regions);
}

HAL_StatusTypeDef ADS7128_GetAlertEvents(const ADS7128_HandleTypeDef *hadc,
                                         uint16_t *events) {
  uint8_t high_flags = 0;
  uint8_t low_flags = 0;
  HAL_StatusTypeDef status;

  if (events == NULL)
    return HAL_ERROR;

  status = ADS7128_ReadRegister(hadc, ADS7128_REG_EVENT_HIGH_FLAG, &high_flags);
  if (status != HAL_OK)
    return status;

  status = ADS7128_ReadRegister(hadc, ADS7128_REG_EVENT_LOW_FLAG, &low_flags);
  if (status != HAL_OK)
    return status;

  *events = 0;
  for (uint8_t channel = 0; channel < 8; channel++) {
    uint8_t channel_flags = (uint8_t)(1U << channel);

    *events |= ((high_flags & channel_flags) << (channel * 2 + 1));
    *events |= ((low_flags & channel_flags) << (channel * 2));
  }

  return HAL_OK;
}

HAL_StatusTypeDef ADS7128_ClearAlertEvents(const ADS7128_HandleTypeDef *hadc,
                                           uint8_t high_channel_mask,
                                           uint8_t low_channel_mask) {
  HAL_StatusTypeDef status;

  if (high_channel_mask != 0) {
    status = ADS7128_WriteRegister(hadc, ADS7128_REG_EVENT_HIGH_FLAG,
                                   high_channel_mask);
    if (status != HAL_OK)
      return status;
  }

  if (low_channel_mask != 0)
    return ADS7128_WriteRegister(hadc, ADS7128_REG_EVENT_LOW_FLAG,
                                 low_channel_mask);

  return HAL_OK;
}
