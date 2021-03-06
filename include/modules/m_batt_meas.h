/*
  Copyright (c) 2010 - 2017, Nordic Semiconductor ASA
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice, this
     list of conditions and the following disclaimer.

  2. Redistributions in binary form, except as embedded into a Nordic
     Semiconductor ASA integrated circuit in a product or a software update for
     such product, must reproduce the above copyright notice, this list of
     conditions and the following disclaimer in the documentation and/or other
     materials provided with the distribution.

  3. Neither the name of Nordic Semiconductor ASA nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission.

  4. This software, with or without modification, must only be used with a
     Nordic Semiconductor ASA integrated circuit.

  5. Any software provided in binary form under this license must not be reverse
     engineered, decompiled, modified and/or disassembled.

  THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
  OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

 /** @file Battery measurement module
 *
 * @defgroup m_batt_meas Battery measurement
 * @{
 * @ingroup modules
 * @brief Battery measurement module API.
 *
 */

#ifndef __M_BATT_MEAS_H__
#define __M_BATT_MEAS_H__

#include "m_ble.h"
#include "nrf_saadc.h"
#include <stdint.h>
#include <stdbool.h>

#define MEAS_INTERVAL_LOW_LIMIT_MS 50
/* The SAADC is configured for oversampling for improved noise immunity.
corresponding settings can be found in sdk_config.h */

/** @brief The m_batt_meas return status codes.
 */
enum
{
    M_BATT_STATUS_CODE_SUCCESS,                     ///< Successfull
    M_BATT_STATUS_CODE_INVALID_PARAM,               ///< Invalid parameters
};

/** @brief Battery and charge event codes.
 */
typedef enum
{
    M_BATT_MEAS_EVENT_DATA,                         ///< Is called back at the desired frequency.
    M_BATT_MEAS_EVENT_LOW,                          ///< Low battery event  (<= set by user).
    M_BATT_MEAS_EVENT_FULL,                         ///< Full battery event (>= value set by user).
    M_BATT_MEAS_EVENT_USB_CONN_CHARGING,            ///< Battery main og tricle charging active.
    M_BATT_MEAS_EVENT_USB_CONN_CHARGING_FINISHED,   ///< Battery charging finished/not charging.
    M_BATT_MEAS_EVENT_USB_DISCONN,                  ///< USB disconnected, battery not charging.
    M_BATT_MEAS_EVENT_ERROR,                        ///< Error state detected signalled by the charger (not implemeted, CHG and CHG finished will toggle in case of error).
}m_batt_meas_event_type_t;

/** @brief The struct passed to the handler with relevant battery information.
 */
typedef struct
{
    m_batt_meas_event_type_t    type;               ///< Given event type.
    uint16_t                    voltage_mv;         ///< Battery voltage given in millivolts.
    uint8_t                     level_percent;      ///< Remaining battery capacity percent.
    bool                        valid_voltage;      ///< True if an event is generated by a ADC conversion.
}m_batt_meas_event_t;

/** @brief m_batt sensor event handler type. Should be implemented by user e.g. in main.c()
 */
typedef void (*m_batt_meas_event_handler_t)(m_batt_meas_event_t const * p_event);

/** @brief Struct for providing m_batt_meas with physical voltage divider information.
 *
 *@note r_1 is situated between the battery and adc_pin, r_2 connects adc_pin and GND.
 */
typedef struct
{
    uint32_t r_1_ohm;
    uint32_t r_2_ohm;
}voltage_divider_t;

/** @brief Struct for providing m_batt_meas with information on converting voltage to state of charge (Remaining battery capacity).
 */
typedef struct
{
    uint16_t            num_elements;                   ///< Number of elements in the voltage to state of charge vector.
    uint16_t            first_element_mv;               ///< Voltage of the first element in the vector [mV];
    uint8_t             delta_mv;                       ///< Distance in voltage between elements in the SoC vector [mV].
    uint8_t const *     voltage_to_soc;                 ///< Pointer to vector mapping from voltage to state of charge.
}state_of_charge_t;


/** @brief Input parameters for m_batt_meas.
 */
typedef struct
{
    uint16_t            app_timer_prescaler;            ///< App timer prescaler. See APP_TIMER_PRESCALER.
    uint8_t             adc_pin_no;                     ///< Analog in pin connected to battery (with possible voltage divider).
    nrf_saadc_input_t   adc_pin_no_ain;                 ///< Same as above, but given as nrf_saadc_input_t. These two values must correspond.            
    uint8_t             usb_detect_pin_no;              ///< Pin = high when USB is connected.
    uint8_t             batt_chg_stat_pin_no;           ///< Pin connected to "Charging status output pin" (CSO) of the battery charger.
    bool                batt_mon_en_pin_used;           ///< Indicates if a pin is used to enable battery monitoring. (E.g. activates a voltage divider).
    uint8_t             batt_mon_en_pin_no;             ///< Pin for enabling battery monitoring.
    uint16_t            batt_voltage_limit_low;         ///< Low voltage limit [milliVolts].
    uint16_t            batt_voltage_limit_full;        ///< Full voltage limit [milliVolts].
    voltage_divider_t   voltage_divider;                ///< Voltage divider type containing resistor information.
    state_of_charge_t   state_of_charge;                ///< Information on mapping from voltage to state of charge.
}batt_meas_param_t;

/** @brief Init parameters for m_batt_meas.
 */
typedef struct
{
    m_batt_meas_event_handler_t     evt_handler;        ///< Function pointer to the event handler (executed in main context).
    batt_meas_param_t               batt_meas_param;    ///< Input parameters.
}batt_meas_init_t;

/**@brief Function for initializing the battery driver.
 *
 * @param[out] p_handle             Pointer to the location to store the service handle.
 * @param[in]  p_batt_meas_init     Struct containing the configuration parameters.
 *
 * @return M_BATT_STATUS_CODE_SUCCESS
 * @return M_BATT_STATUS_CODE_INVALID_PARAM
 * @return NRF_ERROR_NULL
 * @return Other codes from the underlying driver.
 */
uint32_t m_batt_meas_init(m_ble_service_handle_t * p_handle, batt_meas_init_t const * const p_batt_meas_init);

/**@brief Function for enabling battery measurement at the given interval.
 *
 * @param meas_interval_ms  Sampling interval given in milliseconds.
 *
 * @note This will call the handler supplied in p_batt_meas_init at the given interval
 * which is supplied with a m_batt_meas_event_t struct containing the information.
 *
 * @return M_BATT_STATUS_CODE_SUCCESS
 * @return Other codes from the underlying driver.
 */
uint32_t m_batt_meas_enable(uint32_t meas_interval_ms);

/**@brief Function for stopping the battery measurement.
 *
 * @return M_BATT_STATUS_CODE_SUCCESS
 * @return Other codes from the underlying driver.
 */
uint32_t m_batt_meas_disable(void);

#endif

/** @} */
