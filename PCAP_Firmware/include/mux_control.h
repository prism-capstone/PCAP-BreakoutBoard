/**
 * @file mux_control.h
 * @brief CD74HC4067 multiplexer control for PCAP04 chip selection
 *
 * This module provides control for the multiplexer that routes SPI communication
 * to individual PCAP04 capacitive sensing chips on the breakout board.
 */

#ifndef MUX_CONTROL_H
#define MUX_CONTROL_H

#include <Arduino.h>

/**
 * @defgroup MuxPins Multiplexer Control Pins
 * @brief GPIO pins connected to CD74HC4067 multiplexer select lines
 * Based on Seeeduino XIAO ESP32C3 pinout and your schematic
 * The multiplexer routes the shared CS signal to individual PCAP chips
 * @{
 */

// TODO: These pin assignments are based on the old schematic. Need to change for the new board.
#define MUX_S0_PIN  D3   ///< PCAP_SEL0 - Connects to multiplexer S0 (pin 3)
#define MUX_S1_PIN  D2   ///< PCAP_SEL1 - Connects to multiplexer S1 (pin 2)
#define MUX_S2_PIN  D1   ///< PCAP_SEL2 - Connects to multiplexer S2 (pin 1)
#define MUX_S3_PIN  D0   ///< PCAP_SEL3 - Connects to multiplexer S3 (pin 0)
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
    PCAP_CHIP_5 = 3,        ///< PCAP chip 5 (mux channel 3)
    PCAP_CHIP_6 = 4,        ///< PCAP chip 6 (mux channel 4)
    PCAP_CHIP_7 = 5,        ///< PCAP chip 7 (mux channel 5)
    PCAP_CHIP_8 = 6,        ///< PCAP chip 8 (mux channel 6)
    PCAP_CHIP_NONE = 15     ///< No chip selected / invalid selection
} pcap_chip_select_t;

/**
 * @class MuxControl
 * @brief Controls the CD74HC4067 multiplexer for PCAP chip routing
 *
 * This class manages the GPIO pins that control which PCAP04 chip is connected
 * to the shared SPI bus through the multiplexer.
 */
class MuxControl {
public:
    /**
     * @brief Initialize the multiplexer control pins
     *
     * Configures the GPIO pins as outputs and deselects all chips initially.
     */
    void begin();

    void deselectChip();

    /**
     * @brief Select a specific PCAP chip through the multiplexer
     * @param chip The PCAP chip to select (or PCAP_CHIP_NONE to deselect all)
     *
     * Sets the multiplexer select pins to route SPI communication to the
     * specified chip. Includes settling delay for stable communication.
     */
    void selectChip(pcap_chip_select_t chip);

    /**
     * @brief Get the currently selected chip
     * @return The currently selected PCAP chip identifier
     */
    pcap_chip_select_t getCurrentChip();

private:
    pcap_chip_select_t current_chip = PCAP_CHIP_NONE; ///< Currently selected chip
};

#endif // MUX_CONTROL_H
