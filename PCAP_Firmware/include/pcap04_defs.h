/**
 * @file pcap04_defs.h
 * @brief PCAP04 chip definitions, commands, and data structures
 *
 * This file contains all constant definitions for the PCAP04 capacitive sensor chip
 * including SPI command codes, system configuration parameters, and data structures
 * for storing measurement results.
 */

#ifndef PCAP04_DEFS_H
#define PCAP04_DEFS_H

#include <stdint.h>

/**
 * @defgroup PCAPCommands PCAP04 SPI Command Codes
 * @brief Command byte definitions for PCAP04 SPI communication
 * @{
 */

/** @defgroup MemoryCommands Memory Access Commands */
/** @{ */
#define PCAP_WR_MEM         0xA0    ///< Write to internal memory
#define PCAP_RD_MEM         0x20    ///< Read from internal memory
/** @} */

/** @defgroup ConfigCommands Configuration Access Commands */
/** @{ */
#define PCAP_WR_CONFIG      0xA3C0  ///< Write configuration registers (byte-wise)
#define PCAP_RD_CONFIG      0x23C0  ///< Read configuration registers (byte-wise)
/** @} */

/** @defgroup ResultCommands Measurement Result Commands */
/** @{ */
#define PCAP_RD_RESULT      0x40    ///< Read measurement results
/** @} */

/** @defgroup ControlCommands Control and Initialization Commands */
/** @{ */
#define PCAP_POR            0x88    ///< Power-On Reset - resets chip to default state
#define PCAP_INIT           0x8A    ///< Initialize - starts chip operation
#define PCAP_CDC_START      0x8C    ///< Start capacitance-to-digital conversion
#define PCAP_RDC_START      0x8E    ///< Start resistance-to-digital conversion
#define PCAP_DSP_TRIG       0x8D    ///< Trigger DSP processing
/** @} */

/** @defgroup NVMemCommands Non-Volatile Memory Commands */
/** @{ */
#define PCAP_NV_STORE       0x96    ///< Store current config to non-volatile memory
#define PCAP_NV_RECALL      0x99    ///< Recall config from non-volatile memory
#define PCAP_NV_ERASE       0x9C    ///< Erase non-volatile memory
/** @} */

/** @defgroup TestCommands Test and Diagnostic Commands */
/** @{ */
#define PCAP_TEST_READ      0x7E    ///< Test read operation
/** @} */

/** @} */ // End of PCAPCommands group

/**
 * @defgroup SystemConfig System Configuration Parameters
 * @brief Hardware configuration constants for the PCAP sensor system
 * @{
 */
#define NUM_PCAP_CHIPS       8      ///< Total number of PCAP04 chips in the system
#define NUM_SENSORS_PER_CHIP 6      ///< Number of sensor inputs per PCAP04 chip
#define PCAP_CONFIG_SIZE     52     ///< Size of configuration data in bytes
#define PCAP_FW_SIZE         1024   ///< Size of firmware data in bytes
/** @} */

/**
 * @struct pcap_data_t
 * @brief Data structure for storing PCAP04 measurement results
 *
 * This structure holds raw measurement values, calibrated final values,
 * and offset corrections for all sensors on a single PCAP04 chip.
 */
typedef struct {
    uint32_t raw[NUM_SENSORS_PER_CHIP];     ///< Raw 24-bit measurement values from ADC
    float final[NUM_SENSORS_PER_CHIP];      ///< Calibrated/processed measurement values
    float offset[NUM_SENSORS_PER_CHIP];     ///< Offset correction values for calibration
} pcap_data_t;

#endif // PCAP04_DEFS_H
