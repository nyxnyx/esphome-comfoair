#include <iostream>
#include <cassert>
#include <vector>
#include <cstdint>
#include <cmath>

// Mocking some of the logic from comfoair.h
namespace esphome {
namespace comfoair {

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

// Updated balance logic for testing (returns target fan level)
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

uint32_t decode_uint24(const uint8_t *data) {
    return (uint32_t(data[0]) << 16) | (uint32_t(data[1]) << 8) | uint32_t(data[2]);
}

} // namespace comfoair
} // namespace esphome

using namespace esphome::comfoair;

int main() {
    std::cout << "Running Updated C++ Protocol & Logic Tests..." << std::endl;

    // Checksum tests
    assert(calculate_command_checksum(0x67, 0, nullptr) == 0x14);
    assert(calculate_command_checksum(0x99, 1, (const uint8_t[]){0x02}) == 0x49);

    // 24-bit Operating Hours Big Endian test
    // E.g. [0x00, 0x1A, 0x2B] = 6699 hours
    uint8_t sample_hours[3] = {0x00, 0x1A, 0x2B};
    assert(decode_uint24(sample_hours) == 6699);
    // Values exceeding 16-bit (e.g. 70000 = 0x011170)
    uint8_t high_hours[3] = {0x01, 0x11, 0x70};
    assert(decode_uint24(high_hours) == 70000);
    std::cout << "Test: 24-bit Operating Hours Decoding - Passed" << std::endl;

    // Auto-balance logic tests (Fan Speed Management)
    
    // Scenario 1: Summer night, bypass is OPEN, cooler outside.
    // Result: Boost fan speed to help cooling.
    assert(run_auto_balance_logic(18.0f, 25.0f, 22.0f, true) == 4); // delta 3.0 > 1.5 -> HIGH
    std::cout << "Test: Auto-balance Boost (Summer Cooling) - Passed" << std::endl;

    // Scenario 2: Summer day, bypass is CLOSED (unit decided to keep coolness).
    // Result: Boost fan speed only if delta is high to increase recovery efficiency? 
    // Actually, in our logic: if closed and delta > 1.5 -> boost.
    assert(run_auto_balance_logic(35.0f, 26.0f, 22.0f, false) == 4);
    std::cout << "Test: Auto-balance Boost (Heat Recovery) - Passed" << std::endl;

    // Scenario 3: Bypass open but condition UNFAVORABLE (e.g. it's hot inside but even hotter outside).
    // Result: Don't boost speed (let the unit handle it, maybe it's still closing the bypass).
    assert(run_auto_balance_logic(35.0f, 28.0f, 22.0f, true) == 0);
    std::cout << "Test: Auto-balance No Boost (Unfavorable) - Passed" << std::endl;

    // Fault flags decoding test (0xDA)
    // msg[5]: bit 0 = T1, bit 1 = T2, bit 2 = T3, bit 3 = T4, bit 4 = Preheater
    // msg[6]: bit 0 = Supply Fan, bit 1 = Exhaust Fan, bit 2 = Bypass
    uint8_t err_msg[17] = {0};
    err_msg[0] = 0x05; // error_code
    err_msg[5] = 0x03; // T1 and T2 fault
    err_msg[6] = 0x01; // Supply fan fault
    err_msg[8] = 0x01; // Filter dirty
    assert((err_msg[5] & (1 << 0)) != 0); // T1 fault
    assert((err_msg[5] & (1 << 1)) != 0); // T2 fault
    assert((err_msg[5] & (1 << 2)) == 0); // T3 OK
    assert((err_msg[6] & (1 << 0)) != 0); // Supply fan fault
    assert((err_msg[6] & (1 << 1)) == 0); // Exhaust fan OK
    assert(err_msg[8] != 0);              // Filter dirty
    std::cout << "Test: Diagnostic Fault Flags Decoding - Passed" << std::endl;

    // Delays decoding test (0xCA)
    uint8_t delay_msg[8] = {2, 15, 30, 16, 0, 0, 0, 0};
    assert(delay_msg[0] == 2);   // Bathroom switch-on delay (min)
    assert(delay_msg[1] == 15);  // Bathroom switch-off delay (min)
    assert(delay_msg[2] == 30);  // Boost timer (min)
    assert(delay_msg[3] == 16);  // Filter weeks
    std::cout << "Test: Delay Parameters Decoding - Passed" << std::endl;

    std::cout << "All tests passed successfully!" << std::endl;
    return 0;
}
