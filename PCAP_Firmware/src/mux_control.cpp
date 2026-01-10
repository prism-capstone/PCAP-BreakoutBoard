#include "mux_control.h"

void MuxControl::begin() {
    pinMode(MUX_S0_PIN, OUTPUT);
    pinMode(MUX_S1_PIN, OUTPUT);
    pinMode(MUX_S2_PIN, OUTPUT);
    pinMode(MUX_S3_PIN, OUTPUT);

    // Deselect all initially
    selectChip(PCAP_CHIP_NONE);
}

void MuxControl::selectChip(pcap_chip_select_t chip) {
    if (chip == PCAP_CHIP_NONE) {
        return;
    }

    // Set the channel select pins according to the chip select pins
    digitalWrite(MUX_S0_PIN, (chip & 0x01) ? HIGH : LOW);
    digitalWrite(MUX_S1_PIN, (chip & 0x02) ? HIGH : LOW);
    digitalWrite(MUX_S2_PIN, (chip & 0x04) ? HIGH : LOW);
    digitalWrite(MUX_S3_PIN, (chip & 0x08) ? HIGH : LOW);

    // Small delay to allow mux to settle
    delayMicroseconds(10);
}

pcap_chip_select_t MuxControl::getCurrentChip() {
    return current_chip;
}
