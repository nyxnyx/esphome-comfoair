#include <iostream>
#include <cassert>
#include <vector>
#include <cstdint>

// Minimal mock or just implementation copy for testing
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

} // namespace comfoair
} // namespace esphome

using namespace esphome::comfoair;

int main() {
    std::cout << "Running C++ Protocol Tests..." << std::endl;

    // Test Case 1: GET_BOOTLOADER_VERSION (0x67) with no data
    uint8_t cs1 = calculate_command_checksum(0x67, 0, nullptr);
    assert(cs1 == 0x14); // 0x67 + 0xad = 0x114
    std::cout << "Test 1 Passed: GET_BOOTLOADER_VERSION checksum" << std::endl;

    // Test Case 2: SET_LEVEL (0x99) with length 1 and data 0x02 (Level 2)
    uint8_t data2[] = {0x02};
    uint8_t cs2 = calculate_command_checksum(0x99, 1, data2);
    // 0x99 + 1 + (0x02 + 0xad) = 0x99 + 1 + 0xaf = 0x149 & 0xff = 0x49
    assert(cs2 == 0x49);
    std::cout << "Test 2 Passed: SET_LEVEL checksum" << std::endl;

    std::cout << "All C++ tests passed!" << std::endl;
    return 0;
}
