/**
 * @file mux_control.h
 * @brief CD74HC4067 multiplexer control for PCAP04 chip selection (ESP-IDF)
 *
 * This module provides control for the multiplexer that routes SPI communication
 * to individual PCAP04 capacitive sensing chips on the breakout board.
 */

#ifndef MUX_CONTROL_H
#define MUX_CONTROL_H

#include <stdint.h>
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

// Uncomment the below to enable the newer iteration of mux pins
#define OLD_PCAP_BOARD 1

/**
 * @defgroup MuxPins Multiplexer Control Pins
 * @brief GPIO pins connected to CD74HC4067 multiplexer select lines
 * Based on Seeeduino XIAO ESP32C3 pinout and the schematic
 * The multiplexer routes the shared CS signal to individual PCAP chips
 * @{
 */

#if OLD_PCAP_BOARD
// Pin assignments for old board revision
// XIAO ESP32C3: D0=GPIO2, D1=GPIO3, D2=GPIO4, D3=GPIO5
    #define MUX_S0_PIN  GPIO_NUM_5   ///< PCAP_SEL0 - D3
    #define MUX_S1_PIN  GPIO_NUM_4   ///< PCAP_SEL1 - D2
    #define MUX_S2_PIN  GPIO_NUM_3   ///< PCAP_SEL2 - D1
    #define MUX_S3_PIN  GPIO_NUM_2   ///< PCAP_SEL3 - D0
#else
// Pin assignments for new board revision
    #define MUX_S0_PIN  GPIO_NUM_6   ///< PCAP_SEL0 - D4
    #define MUX_S1_PIN  GPIO_NUM_5   ///< PCAP_SEL1 - D3
    #define MUX_S2_PIN  GPIO_NUM_4   ///< PCAP_SEL2 - D2
    #define MUX_S3_PIN  GPIO_NUM_3   ///< PCAP_SEL3 - D1
#endif

/** @} */

/**
 * @brief PCAP chip selection identifiers
 *
 * Enumeration of available PCAP04 chips connected through the multiplexer.
 * Each value corresponds to a multiplexer channel number.
 */
typedef enum {
    PCAP_CHIP_1 = 0,        ///< PCAP chip 1 (mux channel 0)
    PCAP_CHIP_2 = 1,        ///< PCAP chip 2 (mux channel 1)
    PCAP_CHIP_3 = 2,        ///< PCAP chip 3 (mux channel 2)
    PCAP_CHIP_4 = 3,        ///< PCAP chip 4 (mux channel 3)
    PCAP_CHIP_5 = 4,        ///< PCAP chip 5 (mux channel 4)
    PCAP_CHIP_6 = 5,        ///< PCAP chip 6 (mux channel 5)
    PCAP_CHIP_7 = 6,        ///< PCAP chip 7 (mux channel 6)
    PCAP_CHIP_8 = 7,        ///< PCAP chip 8 (mux channel 7)
    PCAP_CHIP_NONE = 15     ///< No chip selected / invalid selection
} pcap_chip_select_t;

/**
 * @brief Initialize the multiplexer control pins
 *
 * Configures the GPIO pins as outputs and deselects all chips initially.
 */
void mux_init(void);

/**
 * @brief Select a specific PCAP chip through the multiplexer
 * @param chip The PCAP chip to select (or PCAP_CHIP_NONE to deselect all)
 *
 * Sets the multiplexer select pins to route SPI communication to the
 * specified chip. Includes settling delay for stable communication.
 */
void mux_select_chip(pcap_chip_select_t chip);

/**
 * @brief Get the currently selected chip
 * @return The currently selected PCAP chip identifier
 */
pcap_chip_select_t mux_get_current_chip(void);

#ifdef __cplusplus
}
#endif

#endif // MUX_CONTROL_H
