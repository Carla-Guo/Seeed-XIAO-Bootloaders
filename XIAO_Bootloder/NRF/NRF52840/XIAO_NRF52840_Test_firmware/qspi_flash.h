#ifndef QSPI_FLASH_H
#define QSPI_FLASH_H

#include <stdint.h>
#include <stdbool.h>
#include "nrfx_qspi.h" // Required for nrfx_err_t

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the QSPI peripheral and the external Flash memory.
 *
 * This function performs the necessary setup for the nRF52840's QSPI peripheral
 * and configures the connected Flash chip (e.g., sending reset commands,
 * setting quad enable bit). It should be called once at startup.
 *
 * @return NRFX_SUCCESS if initialization is successful, otherwise an error code.
 */
nrfx_err_t qspi_flash_init(void);

/**
 * @brief Erases a 64KB block of Flash memory at the specified address.
 *
 * This function sends the 64KB block erase command to the QSPI Flash.
 * The address must be aligned to a 64KB boundary for a block erase.
 * The function waits for the erase operation to complete.
 *
 * @param[in] address The start address of the 64KB block to erase.
 * Must be a multiple of 0x10000 (64KB).
 * @return NRFX_SUCCESS if the erase command is sent and completes successfully,
 * otherwise an error code.
 */
nrfx_err_t qspi_flash_erase_block(uint32_t address);

/**
 * @brief Writes data to the QSPI Flash memory.
 *
 * This function writes a specified number of bytes from a buffer to the
 * given address in Flash memory. It handles the necessary Write Enable (WREN)
 * command and waits for the write operation to complete.
 *
 * @param[in] p_data Pointer to the data buffer to write from.
 * @param[in] length The number of bytes to write.
 * @param[in] address The Flash memory address to start writing to.
 * @return NRFX_SUCCESS if the write operation is successful,
 * otherwise an error code.
 */
nrfx_err_t qspi_flash_write(const uint8_t *p_data, uint32_t length, uint32_t address);

/**
 * @brief Reads data from the QSPI Flash memory.
 *
 * This function reads a specified number of bytes from the given Flash memory
 * address into a provided buffer. It waits for the read operation to complete.
 *
 * @param[out] p_buffer Pointer to the buffer where the read data will be stored.
 * @param[in] length The number of bytes to read.
 * @param[in] address The Flash memory address to start reading from.
 * @return NRFX_SUCCESS if the read operation is successful,
 * otherwise an error code.
 */
nrfx_err_t qspi_flash_read(uint8_t *p_buffer, uint32_t length, uint32_t address);

/**
 * @brief Writes a null-terminated string to QSPI Flash memory.
 *
 * This function writes a null-terminated string to the specified Flash address.
 * It automatically handles padding the string to a 4-byte boundary for QSPI
 * efficiency and includes the null terminator in the written data.
 *
 * @param[in] data_str The null-terminated string to write.
 * @param[in] address The Flash memory address to write the string to.
 * @param[in] max_len The maximum buffer size that can be used for writing.
 * This is used to ensure the padded string does not overflow
 * internal buffers.
 * @return true if the string is written successfully, false otherwise.
 */
bool qspi_flash_write_string(const char* data_str, uint32_t address, uint32_t max_len);

/**
 * @brief Reads a null-terminated string from QSPI Flash memory.
 *
 * This function reads bytes from Flash into a buffer until 'max_len' bytes
 * are read or a null terminator is encountered. The buffer is always
 * null-terminated.
 *
 * @param[out] buffer The buffer to store the read string.
 * @param[in] address The Flash memory address to read the string from.
 * @param[in] max_len The maximum number of bytes to read into the buffer,
 * including the null terminator. Ensure this buffer is
 * large enough.
 * @return true if the read operation is successful, false otherwise.
 */
bool qspi_flash_read_string(char* buffer, uint32_t address, uint32_t max_len);


#ifdef __cplusplus
}
#endif

#endif // QSPI_FLASH_H