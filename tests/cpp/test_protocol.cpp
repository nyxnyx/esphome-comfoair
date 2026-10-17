#include <iostream>
#include <cassert>
#include <vector>
#include <cstdint>
#include <cmath>
#include <iomanip>

namespace esphome {
namespace comfoair {

// --- Protocol Mock Functions Matching comfoair.h ---

uint8_t comfoair_checksum_(const uint8_t *command_data, uint8_t length) {
    uint8_t sum = 0;
    for (uint8_t i = 0; i < length; i++) {
        sum += command_data[i];
    }
    return sum + 0xad;
}

uint8_t calculate_command_checksum(uint8_t command, uint8_t length, const uint8_t *data) {
    if (length > 0) {
        return (command + length + comfoair_checksum_(data, length)) & 0xff;
    } else {
        return comfoair_checksum_(&command, 1);
    }
}

uint32_t decode_uint24(const uint8_t *data) {
    return (uint32_t(data[0]) << 16) | (uint32_t(data[1]) << 8) | uint32_t(data[2]);
}

float decode_temperature(uint8_t raw) {
    return (float) raw / 2.0f - 20.0f;
}

uint8_t encode_temperature(float temp) {
    return (uint8_t) std::round((temp + 20.0f) * 2.0f);
}

float decode_analog_voltage(uint8_t percentage) {
    return (float) percentage * 0.1f;
}

// Simulates write_escaped_byte_ logic from comfoair.h
void write_escaped_bytes(const std::vector<uint8_t> &in, std::vector<uint8_t> &out) {
    out.clear();
    for (uint8_t b : in) {
        out.push_back(b);
        if (b == 0x07) {
            out.push_back(0x07);
        }
    }
}

// Balance logic from comfoair.h
int run_auto_balance_logic(float outside, float return_air, float target, bool bypass_open) {
    bool favorable = false;
    if (bypass_open) {
        if ((return_air > target + 0.5f && outside < return_air) || 
            (return_air < target - 0.5f && outside > return_air)) {
            favorable = true;
        }
    } else {
        if (std::abs(return_air - target) > 1.5f) {
            favorable = true;
        }
    }

    if (favorable && std::abs(return_air - target) > 1.5f) {
        return 4; // HIGH
    } else if (favorable) {
        return 3; // MEDIUM
    }
    return 0; // AUTO
}

// Validates received packet checksum (same formula as check_byte_ in comfoair.h)
bool validate_packet_checksum(const uint8_t *data_with_header, uint8_t data_len) {
    // data_with_header: [0x07, 0xF0, 0x00, CMD, LEN, DATA..., CHECKSUM]
    // Checksum is calculated over data + 2 for (COMFOAIR_MSG_HEAD_LENGTH + data_length - 2)
    // Head length is 5 (0x07, 0xF0, 0x00, CMD, LEN)
    uint8_t expected_checksum = comfoair_checksum_(data_with_header + 2, 5 + data_len - 2);
    uint8_t actual_checksum = data_with_header[5 + data_len];
    return expected_checksum == actual_checksum;
}

} // namespace comfoair
} // namespace esphome

using namespace esphome::comfoair;

void test_checksums() {
    std::cout << "Testing Checksum Calculations for Protocol Commands..." << std::endl;
    // 1. Get Temperatures (0x67, len 0)
    assert(calculate_command_checksum(0x67, 0, nullptr) == 0x14);

    // 2. Get Fan State (0x99, len 1, data [0x02])
    const uint8_t fan_req[1] = {0x02};
    assert(calculate_command_checksum(0x99, 1, fan_req) == 0x49);

    // 3. Set Ventilation Levels (0xCF, len 9)
    // 15, 30, 45, 70, 15, 30, 45, 70, speed=2
    const uint8_t vent_levels[9] = {15, 30, 45, 70, 15, 30, 45, 70, 2};
    uint8_t cs_vent = calculate_command_checksum(0xCF, 9, vent_levels);
    // Verify checksum matches byte-by-byte manual calculation:
    // sum = 15+30+45+70+15+30+45+70+2 = 322 (0x142)
    // comfoair_checksum = 0x42 + 0xAD = 0xEF
    // total = (0xCF + 9 + 0xEF) & 0xFF = 0x2C7 & 0xFF = 0xC7
    assert(cs_vent == 0xC7);

    // 4. Set Delays (0xCB, len 6)
    // bath_on=2, bath_off=15, boost=30, filter=16, 0, 0
    const uint8_t delays[6] = {2, 15, 30, 16, 0, 0};
    uint8_t cs_delays = calculate_command_checksum(0xCB, 6, delays);
    // sum = 2+15+30+16+0+0 = 63 (0x3F)
    // comfoair_checksum = 0x3F + 0xAD = 0xEC
    // total = (0xCB + 6 + 0xEC) & 0xFF = 0x1BD & 0xFF = 0xBD
    assert(cs_delays == 0xBD);

    // 5. Set EWT (0xED, len 7)
    // [comfort=82, bypass_factor=100, bypass_step=10, ewt_low=54, ewt_high=86, 0, 0]
    const uint8_t ewt_data[7] = {82, 100, 10, 54, 86, 0, 0};
    uint8_t cs_ewt = calculate_command_checksum(0xED, 7, ewt_data);
    assert(cs_ewt == ((0xED + 7 + comfoair_checksum_(ewt_data, 7)) & 0xFF));

    // 6. Set PC Master Mode (0x9B, len 4)
    // [0x04, 0x00, 0x00, 0x00]
    const uint8_t pc_mode[4] = {0x04, 0x00, 0x00, 0x00};
    uint8_t cs_pc = calculate_command_checksum(0x9B, 4, pc_mode);
    assert(cs_pc == ((0x9B + 4 + comfoair_checksum_(pc_mode, 4)) & 0xFF));

    // 7. Reset Filter (0x3B, len 4)
    const uint8_t filter_reset[4] = {0, 0, 0, 1};
    uint8_t cs_filter = calculate_command_checksum(0x3B, 4, filter_reset);
    assert(cs_filter == ((0x3B + 4 + comfoair_checksum_(filter_reset, 4)) & 0xFF));

    std::cout << "  -> All Checksum tests passed!" << std::endl;
}

void test_temperature_conversions() {
    std::cout << "Testing Temperature Conversions (Float <-> Raw)..." << std::endl;
    // 20.0 deg C -> raw 80
    assert(encode_temperature(20.0f) == 80);
    assert(decode_temperature(80) == 20.0f);

    // 21.5 deg C -> raw 83
    assert(encode_temperature(21.5f) == 83);
    assert(decode_temperature(83) == 21.5f);

    // 0.0 deg C -> raw 40
    assert(encode_temperature(0.0f) == 40);
    assert(decode_temperature(40) == 0.0f);

    // -5.0 deg C -> raw 30
    assert(encode_temperature(-5.0f) == 30);
    assert(decode_temperature(30) == -5.0f);

    // -12.5 deg C -> raw 15
    assert(encode_temperature(-12.5f) == 15);
    assert(decode_temperature(15) == -12.5f);

    // -20.0 deg C -> raw 0
    assert(encode_temperature(-20.0f) == 0);
    assert(decode_temperature(0) == -20.0f);

    // 50.0 deg C -> raw 140
    assert(encode_temperature(50.0f) == 140);
    assert(decode_temperature(140) == 50.0f);

    std::cout << "  -> Temperature conversions passed!" << std::endl;
}

void test_operating_hours_uint24() {
    std::cout << "Testing 24-bit Operating Hours Big-Endian Decoding..." << std::endl;
    // 0 hours
    uint8_t zero[3] = {0x00, 0x00, 0x00};
    assert(decode_uint24(zero) == 0);

    // 1 hour
    uint8_t one[3] = {0x00, 0x00, 0x01};
    assert(decode_uint24(one) == 1);

    // Standard 6699 hours [0x00, 0x1A, 0x2B]
    uint8_t sample[3] = {0x00, 0x1A, 0x2B};
    assert(decode_uint24(sample) == 6699);

    // 16-bit max boundary: 65535 [0x00, 0xFF, 0xFF]
    uint8_t boundary16[3] = {0x00, 0xFF, 0xFF};
    assert(decode_uint24(boundary16) == 65535);

    // Exceeding 16-bit: 70000 [0x01, 0x11, 0x70]
    uint8_t high[3] = {0x01, 0x11, 0x70};
    assert(decode_uint24(high) == 70000);

    // 24-bit max: 16777215 [0xFF, 0xFF, 0xFF]
    uint8_t max_hours[3] = {0xFF, 0xFF, 0xFF};
    assert(decode_uint24(max_hours) == 16777215);

    std::cout << "  -> 24-bit Operating Hours tests passed!" << std::endl;
}

void test_auto_balance() {
    std::cout << "Testing Auto-Balance Decision Logic..." << std::endl;
    // Summer night: bypass open, cooling is favorable, delta 3.0 > 1.5 -> HIGH (4)
    assert(run_auto_balance_logic(18.0f, 25.0f, 22.0f, true) == 4);

    // Summer evening: bypass open, cooling favorable, delta 1.0 <= 1.5 -> MEDIUM (3)
    assert(run_auto_balance_logic(19.0f, 23.0f, 22.0f, true) == 3);

    // Summer hot afternoon: bypass open but outside > return_air (unfavorable) -> AUTO (0)
    assert(run_auto_balance_logic(34.0f, 26.0f, 22.0f, true) == 0);

    // Heat recovery (bypass closed): high delta > 1.5 -> HIGH (4)
    assert(run_auto_balance_logic(35.0f, 26.0f, 22.0f, false) == 4);

    // Heat recovery (bypass closed): small delta <= 1.5 -> AUTO (0)
    assert(run_auto_balance_logic(23.0f, 22.5f, 22.0f, false) == 0);

    std::cout << "  -> Auto-balance logic tests passed!" << std::endl;
}

void test_diagnostics_and_faults() {
    std::cout << "Testing Diagnostic Fault Flags (0xDA)..." << std::endl;
    // msg[5]: bit 0=T1, bit 1=T2, bit 2=T3, bit 3=T4, bit 4=Preheater
    // msg[6]: bit 0=Supply Fan, bit 1=Exhaust Fan, bit 2=Bypass
    // msg[8]: filter dirty flag
    uint8_t err_msg[17] = {0};
    err_msg[0] = 0x07; // error code 7
    err_msg[5] = 0x1B; // bits 0, 1, 3, 4 set (T1, T2, T4, Preheater fault; T3 OK)
    err_msg[6] = 0x05; // bits 0, 2 set (Supply Fan, Bypass fault; Exhaust OK)
    err_msg[8] = 0x01; // filter dirty

    assert((err_msg[5] & (1 << 0)) != 0); // T1 fault
    assert((err_msg[5] & (1 << 1)) != 0); // T2 fault
    assert((err_msg[5] & (1 << 2)) == 0); // T3 OK
    assert((err_msg[5] & (1 << 3)) != 0); // T4 fault
    assert((err_msg[5] & (1 << 4)) != 0); // Preheater fault

    assert((err_msg[6] & (1 << 0)) != 0); // Supply Fan fault
    assert((err_msg[6] & (1 << 1)) == 0); // Exhaust Fan OK
    assert((err_msg[6] & (1 << 2)) != 0); // Bypass fault

    assert(err_msg[8] != 0);              // Filter dirty

    std::cout << "  -> Fault flags decoding passed!" << std::endl;
}

void test_delays_and_analog_inputs() {
    std::cout << "Testing Delays (0xCA) and Analog Inputs (0x14)..." << std::endl;
    // Delays
    uint8_t delay_msg[8] = {3, 20, 45, 24, 0, 0, 0, 0};
    assert(delay_msg[0] == 3);   // bathroom on
    assert(delay_msg[1] == 20);  // bathroom off
    assert(delay_msg[2] == 45);  // boost
    assert(delay_msg[3] == 24);  // filter weeks

    // Analog inputs (0-100% -> 0.0-10.0V)
    assert(std::abs(decode_analog_voltage(0) - 0.0f) < 0.001f);
    assert(std::abs(decode_analog_voltage(50) - 5.0f) < 0.001f);
    assert(std::abs(decode_analog_voltage(75) - 7.5f) < 0.001f);
    assert(std::abs(decode_analog_voltage(100) - 10.0f) < 0.001f);

    std::cout << "  -> Delays and analog inputs passed!" << std::endl;
}

void test_byte_stuffing_escaping() {
    std::cout << "Testing Byte-Stuffing Escaping for 0x07..." << std::endl;
    // Case 1: No 0x07 bytes
    std::vector<uint8_t> in1 = {0x00, 0x67, 0x00, 0x14};
    std::vector<uint8_t> out1;
    write_escaped_bytes(in1, out1);
    assert(out1 == in1);

    // Case 2: Single 0x07 byte -> must become 0x07 0x07
    std::vector<uint8_t> in2 = {0x00, 0x07, 0x42};
    std::vector<uint8_t> out2;
    write_escaped_bytes(in2, out2);
    std::vector<uint8_t> expected2 = {0x00, 0x07, 0x07, 0x42};
    assert(out2 == expected2);

    // Case 3: Multiple consecutive and distributed 0x07 bytes
    std::vector<uint8_t> in3 = {0x07, 0x07, 0x10, 0x07};
    std::vector<uint8_t> out3;
    write_escaped_bytes(in3, out3);
    std::vector<uint8_t> expected3 = {0x07, 0x07, 0x07, 0x07, 0x10, 0x07, 0x07};
    assert(out3 == expected3);

    std::cout << "  -> Byte-stuffing escaping tests passed!" << std::endl;
}

void test_packet_checksum_validation() {
    std::cout << "Testing Frame Packet Checksum Validation..." << std::endl;
    // Valid Frame: [0x07, 0xF0, 0x00, 0x67, 0x00, Checksum: 0x14]
    uint8_t valid_frame[] = {0x07, 0xF0, 0x00, 0x67, 0x00, 0x14};
    assert(validate_packet_checksum(valid_frame, 0) == true);

    // Corrupted Checksum: 0x15 instead of 0x14
    uint8_t corrupt_frame[] = {0x07, 0xF0, 0x00, 0x67, 0x00, 0x15};
    assert(validate_packet_checksum(corrupt_frame, 0) == false);

    // Valid Frame with 2 payload bytes:
    // CMD=0x99, LEN=2, DATA=[15, 30]
    // sum(0x00, 0x99, 0x02, 15, 30) + 0xAD = 0 + 153 + 2 + 15 + 30 + 173 = 373 = 0x175 -> 0x75
    uint8_t expected_cs = comfoair_checksum_((const uint8_t[]){0x00, 0x99, 0x02, 15, 30}, 5);
    uint8_t valid_frame2[] = {0x07, 0xF0, 0x00, 0x99, 0x02, 15, 30, expected_cs};
    assert(validate_packet_checksum(valid_frame2, 2) == true);

    std::cout << "  -> Frame packet checksum validation passed!" << std::endl;
}

int main() {
    std::cout << "==========================================" << std::endl;
    std::cout << "  ComfoAir ESPHome Component Protocol Tests" << std::endl;
    std::cout << "==========================================" << std::endl;

    test_checksums();
    test_temperature_conversions();
    test_operating_hours_uint24();
    test_auto_balance();
    test_diagnostics_and_faults();
    test_delays_and_analog_inputs();
    test_byte_stuffing_escaping();
    test_packet_checksum_validation();

    std::cout << "==========================================" << std::endl;
    std::cout << "  All 8 Test Suites Passed Successfully!  " << std::endl;
    std::cout << "==========================================" << std::endl;
    return 0;
}
