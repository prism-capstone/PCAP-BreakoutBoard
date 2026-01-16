/**
 * @file mux_control.c
 * @brief CD74HC4067 multiplexer control implementation (ESP-IDF)
 */

#include "mux_control.h"
#include "esp_rom_sys.h"

static pcap_chip_select_t current_chip = PCAP_CHIP_NONE;

void mux_init(void)
{
    // Configure GPIO pins as outputs
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << MUX_S0_PIN) | (1ULL << MUX_S1_PIN) |
                        (1ULL << MUX_S2_PIN) | (1ULL << MUX_S3_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // Deselect all initially
    mux_select_chip(PCAP_CHIP_NONE);
}

void mux_select_chip(pcap_chip_select_t chip)
{
    // Set the channel select pins according to the chip select value
    gpio_set_level(MUX_S0_PIN, (chip & 0x01) ? 1 : 0);
    gpio_set_level(MUX_S1_PIN, (chip & 0x02) ? 1 : 0);
    gpio_set_level(MUX_S2_PIN, (chip & 0x04) ? 1 : 0);
    gpio_set_level(MUX_S3_PIN, (chip & 0x08) ? 1 : 0);

    current_chip = chip;

    // Small delay to allow mux to settle (10 microseconds)
    esp_rom_delay_us(10);
}

pcap_chip_select_t mux_get_current_chip(void)
{
    return current_chip;
}
