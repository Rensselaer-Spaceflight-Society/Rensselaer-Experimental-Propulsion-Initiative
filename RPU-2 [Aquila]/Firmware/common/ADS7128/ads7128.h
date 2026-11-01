/**
 * @file ads7128.h
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

#ifndef ADS7128_H
#define ADS7128_H

#ifndef ADS7128_H
#define ADS7128_H

#include <stdint.h>
#include <stm32wlxx_hal.h>
#include "stm32wlxx_hal_def.h"
#include "stm32wlxx_hal_i2c.h"

#define ADS7128_GPIO_ANALOG_INPUT 0b00
#define ADS7128_GPIO_DIGITAL_INPUT 0b01
#define ADS7128_GPIO_DIGITAL_OUT_OD 0b10
#define ADS7128_GPIO_DIGITAL_OUT_PP 0b11

#define ADS7128_OSC_HIGH_SPEED 0
#define ADS7128_OSC_LOW_SPEED 1

#define ADS7128_CLK_DIV_1_1 0
#define ADS7128_CLK_DIV_2_3 1
#define ADS7128_CLK_DIV_1_2 2
#define ADS7128_CLK_DIV_1_3 3
#define ADS7128_CLK_DIV_1_4 4
#define ADS7128_CLK_DIV_1_6 5
#define ADS7128_CLK_DIV_1_8 6
#define ADS7128_CLK_DIV_83_1000 7
#define ADS7128_CLK_DIV_1_16 8
#define ADS7128_CLK_DIV_42_1000 9
#define ADS7128_CLK_DIV_31_1000 10
#define ADS7128_CLK_DIV_21_1000 11
#define ADS7128_CLK_DIV_16_1000 12
#define ADS7128_CLK_DIV_10_1000 13
#define ADS7128_CLK_DIV_8_1000 14
#define ADS7128_CLK_DIV_5_1000 15

#define ADS7128_ADC_MANUAL_MODE 0
#define ADS7128_ADC_SEQUENTIAL_MODE 1
#define ADS7128_ADC_AUTONOMOUS_MODE 2

#define ADS7128_OVERSAMPLING_DISABLED 0
#define ADS7128_OVERSAMPLING_2_SAMPLES 1
#define ADS7128_OVERSAMPLING_4_SAMPLES 2
#define ADS7128_OVERSAMPLING_8_SAMPLES 3
#define ADS7128_OVERSAMPLING_16_SAMPLES 4
#define ADS7128_OVERSAMPLING_32_SAMPLES 5
#define ADS7128_OVERSAMPLING_64_SAMPLES 6
#define ADS7128_OVERSAMPLING_128_SAMPLES 7

#define ADS7128_DWC_DISABLE 0
#define ADS7128_DWC_ENABLE 1

#define ADS7128_STATISTICS_DISABLE 0
#define ADS7128_STATISTICS_ENABLE 1

#define ADS7128_ALERT_OPEN_DRAIN 0
#define ADS7128_ALERT_PUSH_PULL 1

#define ADS7128_ALERT_ACTIVE_LOW 0
#define ADS7128_ALERT_ACTIVE_HIGH 1
#define ADS7128_ALERT_PULSED_LOW 2
#define ADS7128_ALERT_PULSED_HIGH 3

#define ADS7128_EVENT_REGION_OUTSIDE 0
#define ADS7128_EVENT_REGION_INSIDE 1

enum ADS7128_EventType {
  ADS7128_EVENT_NONE = 0,
  ADS7128_EVENT_LOW = 1,
  ADS7128_EVENT_HIGH = 2,
  ADS7128_EVENT_BOTH = 3
};

/**
 * @brief Gets the event from the event mask and returns the event type.
 *
 */
#define ADS7128_GET_EVENT(events, channel)                                     \
  ((events & (0b11 << 2 * channel)) >> (2 * channel))

#define ADS7128_MAX_THRESHOLD 0x0FFF
#define ADS7128_EVENT_COUNT_MIN 1
#define ADS7128_EVENT_COUNT_MAX 16

#define ADS7128_ADC_CHANNEL_0 0
#define ADS7128_ADC_CHANNEL_1 1
#define ADS7128_ADC_CHANNEL_2 2
#define ADS7128_ADC_CHANNEL_3 3
#define ADS7128_ADC_CHANNEL_4 4
#define ADS7128_ADC_CHANNEL_5 5
#define ADS7128_ADC_CHANNEL_6 6
#define ADS7128_ADC_CHANNEL_7 7

#define ADS7128_GPIO_PIN_0 ADS7128_ADC_CHANNEL_0
#define ADS7128_GPIO_PIN_1 ADS7128_ADC_CHANNEL_1
#define ADS7128_GPIO_PIN_2 ADS7128_ADC_CHANNEL_2
#define ADS7128_GPIO_PIN_3 ADS7128_ADC_CHANNEL_3
#define ADS7128_GPIO_PIN_4 ADS7128_ADC_CHANNEL_4
#define ADS7128_GPIO_PIN_5 ADS7128_ADC_CHANNEL_5
#define ADS7128_GPIO_PIN_6 ADS7128_ADC_CHANNEL_6
#define ADS7128_GPIO_PIN_7 ADS7128_ADC_CHANNEL_7

#define ADS7128_GPIO_RESET 0
#define ADS7128_GPIO_SET 1

typedef struct {
  /**
   * @brief Digital window comparator and ALERT pin configuration.
   * - enable: ADS7128_DWC_DISABLE or ADS7128_DWC_ENABLE
   * - alert_drive: ADS7128_ALERT_OPEN_DRAIN or ADS7128_ALERT_PUSH_PULL
   * - alert_logic: ADS7128_ALERT_ACTIVE_LOW, ADS7128_ALERT_ACTIVE_HIGH,
   *   ADS7128_ALERT_PULSED_LOW, or ADS7128_ALERT_PULSED_HIGH
   */
  uint8_t enable;
  uint8_t alert_drive;
  uint8_t alert_logic;
} ADS7128_DWCConfigTypeDef;

typedef struct {
  /**
   * @brief Pins can be configured as:
   * - Analog Input (ADS7128_GPIO_ANALOG_INPUT)
   * - Digital Input (ADS7128_GPIO_DIGITAL_INPUT)
   * - Open-Drain Output (ADS7128_GPIO_DIGITAL_OUT_OD)
   * - Push-Pull Output (ADS7128_GPIO_DIGITAL_OUT_PP)
   */
  uint8_t pin_cfg[8];

  /**
  Select the oscilator speed (ADS7128_OSC_HIGH_SPEED \ ADS7128_OSC_LOW_SPEED)
  High Speed Osc: 1000 KSPS (Kilo Samples Per Second)
  Low Speed Osc: 31.25 KSPS (Kilo Samples Per Second)
  */
  uint8_t osc_select;

  /* Select the sampling rate for the ADC (See
  https://www.ti.com/lit/ds/symlink/ads7128.pdf section 8.3.9) Options:
  - ADS7128_SAMPLE_DIV_1_1     ==> 1/1
  - ADS7128_SAMPLE_DIV_2_3     ==> 2/3
  - ADS7128_SAMPLE_DIV_1_2     ==> 1/2
  - ADS7128_SAMPLE_DIV_1_3     ==> 1/3
  - ADS7128_SAMPLE_DIV_1_4     ==> 1/4
  - ADS7128_SAMPLE_DIV_1_6     ==> 1/6
  - ADS7128_SAMPLE_DIV_1_8     ==> 1/8
  - ADS7128_SAMPLE_DIV_83_1000 ==> 83/1000
  - ADS7128_SAMPLE_DIV_1_16    ==> 1/16
  - ADS7128_SAMPLE_DIV_42_1000 ==> 42/1000
  - ADS7128_SAMPLE_DIV_31_1000 ==> 31/1000
  - ADS7128_SAMPLE_DIV_21_1000 ==> 21/1000
  - ADS7128_SAMPLE_DIV_16_1000 ==> 16/1000
  - ADS7128_SAMPLE_DIV_10_1000 ==> 10/1000
  - ADS7128_SAMPLE_DIV_8_1000  ==> 8/1000
  - ADS7128_SAMPLE_DIV_5_1000  ==> 5/1000
  */
  uint8_t clk_div;

  /**
   * @brief The operating mode for the ADC Sampler
   * - ADS7128_ADC_MANUAL_MODE
   * - ADS7128_ADC_SEQUENTIAL_MODE
   * - ADS7128_ADC_AUTONOMOUS_MODE automatically sample, sample after an alert
   * is triggered
   */
  uint8_t mode;

  /**
   * @brief Enable or disable statistics collection on the ADC. When enabled,
   * the ADC will keep track of the minimum and maximum values for each channel
   * since the last read. When a channel is read, the min and max values are
   * reset.
   * - ADS7128_STATISTICS_DISABLE
   * - ADS7128_STATISTICS_ENABLE
   */
  uint8_t statistics_enable;

  /**
   * @brief Defines how many samples to take on each conversion
   * - ADS7128_OVERSAMPLING_DISABLED
   * - ADS7128_OVERSAMPLING_2_SAMPLES
   * - ADS7128_OVERSAMPLING_4_SAMPLES
   * - ADS7128_OVERSAMPLING_8_SAMPLES
   * - ADS7128_OVERSAMPLING_16_SAMPLES
   * - ADS7128_OVERSAMPLING_32_SAMPLES
   * - ADS7128_OVERSAMPLING_64_SAMPLES
   * - ADS7128_OVERSAMPLING_128_SAMPLES
   */
  uint8_t oversampling;

  ADS7128_DWCConfigTypeDef digital_window_comparator;

} ADS7128_InitTypeDef;

typedef struct {

  // The handle to the I2C peripheral responsible for communicating with the
  // device
  I2C_HandleTypeDef *i2c;
  ADS7128_InitTypeDef Init;

  // 7-Bit Address for the device
  uint8_t address;

  // Maximum Delay (ms) for I2C comms
  uint32_t max_delay;
} ADS7128_HandleTypeDef;

typedef uint8_t ADS7128_PinTypeDef;
typedef uint8_t ADS7128_PinStateTypeDef;

/**
 * @brief Initializes the ADC with the provided configuration in the handle.
 * Once the ADC is initialized, the handle Init fields should not be modified.
 * If they are, the ADC must be reinitialized.
 *
 * @param hadc The ADC Handle to initialize
 * @return status, 0 if ok
 */
HAL_StatusTypeDef ADS7128_Init(const ADS7128_HandleTypeDef *hadc);

/**
 * @brief Tells the ADC to calibrate and blocks execution until the calibration
 * is done.
 *
 * @param hadc The ADC Handle
 * @return status, 0 if ok
 */
HAL_StatusTypeDef ADS7128_Calibrate(const ADS7128_HandleTypeDef *hadc);

/**
 * @brief Set the specific channel on the ADC to read from in Manual Mode. Does
 * nothing in sequential mode.
 *
 * @param hadc The ADC handle
 * @param channel The channel to read from on next conversion
 * @return status, 0 if ok
 */
HAL_StatusTypeDef
ADS7128_ManualSetAnalogChannel(const ADS7128_HandleTypeDef *hadc,
                               ADS7128_PinTypeDef channel);

/**
 * @brief Performs an analog to digital conversion on the channel to be read
 * from. This is either the on in manual mode, or the next channel in sequential
 * mode. Stores the result in `value`, stores which channel was read in
 * `channel_read`.
 *
 * @param hadc The ADC handle
 * @param value Pointer to store the value in
 * @param channel_read Pointer to store the last read channel in (can be NULL)
 * @return status, 0 if ok
 */
HAL_StatusTypeDef ADS7128_AnalogGetConversion(const ADS7128_HandleTypeDef *hadc,
                                              uint16_t *value,
                                              ADS7128_PinTypeDef *channel_read);

typedef enum {
  ADS7128_AUTO_MODE_DISABLED = 0,
  ADS7128_AUTO_MODE_ENABLED = 1,
} ADS7128_AutoModeTypeDef;

/**
 * @brief Enable or disable autonomous mode. In autonomous mode, the ADC will
 automatically each analog trigger, and will fire an alert if the value is
 outside the configured window comparator thresholds. In autonomous mode, the
 ADC will not respond to manual or sequential mode commands.
 *
 * @param hadc the ADC handle
 * @param on_off enable or disable autonomous mode
 * @return HAL_StatusTypeDef status, 0 if ok
 */
HAL_StatusTypeDef ADS7128_SetAutonomousMode(const ADS7128_HandleTypeDef *hadc,
                                            ADS7128_AutoModeTypeDef on_off);

/**
 * @brief Gets the most recent conversion value for a specific channel.
 * 
 * @param hadc the ADC handle
 * @param channel_to_read the channel to read the value for
 * @param value pointer to store the value in
 * @return HAL_StatusTypeDef status, 0 if ok
 */
HAL_StatusTypeDef
ADS7128_AnalogGetPreviousConversion(const ADS7128_HandleTypeDef *hadc,
                                    ADS7128_PinTypeDef channel_to_read,
                                    uint16_t *value);

/**
 * @brief Gets the maximum value recorded for a specific channel since the last read.
 * Maximum is reset when the channel is read.
 * 
 * @param hadc the ADC handle
 * @param channel_to_read the channel to read the value for
 * @param value pointer to store the value in
 * @return HAL_StatusTypeDef status, 0 if ok
 */
HAL_StatusTypeDef
ADS7128_AnalogGetMaximumValue(const ADS7128_HandleTypeDef *hadc,
                              ADS7128_PinTypeDef channel_to_read,
                              uint16_t *value);

/**
 * @brief Gets the minimum value recorded for a specific channel since the last read.
 * Minimum is reset when the channel is read.
 * 
 * @param hadc the ADC handle
 * @param channel_to_read the channel to read the value for
 * @param value pointer to store the value in
 * @return HAL_StatusTypeDef status, 0 if ok
 */
HAL_StatusTypeDef
ADS7128_AnalogGetMinimumValue(const ADS7128_HandleTypeDef *hadc,
                              ADS7128_PinTypeDef channel_to_read,
                              uint16_t *value);

/**
 * @brief Read the digital input value from a pin configured as a digital input.
 *
 * @param hadc The ADC handle
 * @param pin The specific pin to read from
 * @return State of the pin, 1 if high, 0 if low, 255 if the read failed
 */
ADS7128_PinStateTypeDef
ADS7128_ReadDigitalInput(const ADS7128_HandleTypeDef *hadc,
                         ADS7128_PinTypeDef pin);

/**
 * @brief Write a digital value to a pin configured as a digital output.
 *
 * @param hadc The ADC handle
 * @param pin The specific pin to write to
 * @param state The state to set for the pin. 1 if high, 0 if low
 * @return status, 0 if ok.
 */
HAL_StatusTypeDef ADS7128_WriteDigitalOutput(const ADS7128_HandleTypeDef *hadc,
                                             ADS7128_PinTypeDef pin,
                                             ADS7128_PinStateTypeDef state);

/**
 * @brief Select whether comparator events on a channel assert the ALERT pin.
 * @param hadc The ADC handle
 * @param channel ADC channel (ADS7128_ADC_CHANNEL_0 through _7)
 * @param enable Non-zero to route this channel's event flags to ALERT
 * @return HAL status
 */
HAL_StatusTypeDef ADS7128_EnableChannelAlert(const ADS7128_HandleTypeDef *hadc,
                                             ADS7128_PinTypeDef channel,
                                             uint8_t enable);

/**
 * @brief Configure the window comparator for one channel.
 * @param hadc The ADC handle
 * @param channel ADC channel (ADS7128_ADC_CHANNEL_0 through _7)
 * @param low_threshold 12-bit low threshold (0x000 through 0xFFF)
 * @param high_threshold 12-bit high threshold (0x000 through 0xFFF)
 * @param hysteresis Four-bit hysteresis (0 through 15)
 * @param event_region ADS7128_EVENT_REGION_OUTSIDE or _INSIDE
 * @param event_count Consecutive violating samples required (1 through 16)
 * @return HAL status; HAL_ERROR for invalid parameters
 */
HAL_StatusTypeDef ADS7128_ConfigureChannelComparator(
    const ADS7128_HandleTypeDef *hadc, ADS7128_PinTypeDef channel,
    uint16_t low_threshold, uint16_t high_threshold, uint8_t hysteresis,
    uint8_t event_region, uint8_t event_count);

/**
 * @brief Read the comparator events for all channels.
 * Each channel occupies two bits, with channel 7 in bits 15:14:
 * 00 = none, 01 = low event, 10 = high event, 11 = both events.
 * @param hadc The ADC handle
 * @param events Pointer to receive the packed event codes
 * @return HAL status
 */
HAL_StatusTypeDef ADS7128_GetAlertEvents(const ADS7128_HandleTypeDef *hadc,
                                         uint16_t *events);

/**
 * @brief Clear selected sticky high and/or low event flags.
 * A set bit in either mask selects the corresponding channel for clearing.
 * @param hadc The ADC handle
 * @param high_channel_mask Channels whose high-event flags should be cleared
 * @param low_channel_mask Channels whose low-event flags should be cleared
 * @return HAL status
 */
HAL_StatusTypeDef ADS7128_ClearAlertEvents(const ADS7128_HandleTypeDef *hadc,
                                           uint8_t high_channel_mask,
                                           uint8_t low_channel_mask);

#endif /* ADS7128_H */


#endif /* ADS7128_H */
