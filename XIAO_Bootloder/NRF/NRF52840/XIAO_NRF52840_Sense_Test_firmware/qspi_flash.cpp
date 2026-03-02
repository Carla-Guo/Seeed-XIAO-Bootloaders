#include "qspi_flash.h"
#include <string.h> // For strlen, memset, memcpy
#include "app_util_platform.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "sdk_config.h"
#include "nrf_delay.h"
#include <Arduino.h> // For Serial.print, millis, delay

// =====================================================================
// QSPI Settings and Static Variables (adapted from user's working snippet)
// =====================================================================
#define QSPI_STD_CMD_WRSR   0x01
#define QSPI_STD_CMD_RSTEN  0x66
#define QSPI_STD_CMD_RST    0x99
#define QSPI_DPM_ENTER      0x0003 // Deep Power-down Mode Enter duration
#define QSPI_DPM_EXIT       0x0003 // Deep Power-down Mode Exit duration

// Pointer to QSPI STATUS register on nRF52840 (specific hardware address)
static volatile uint32_t *QSPI_Status_Ptr = (uint32_t*) 0x40029604;
static nrfx_qspi_config_t QSPIConfig;
static nrf_qspi_cinstr_conf_t QSPICinstr_cfg;
static bool QSPIWait = false;                  // Flag for WIP bit (Write In Progress) waiting

// Internal buffer for write/read operations.
// Max string length to write is generally small, so a 64-byte buffer is sufficient
// for the test case and common small string writes. Adjust if larger strings are needed.
#define QSPI_INTERNAL_BUFFER_SIZE 256
static uint8_t s_qspi_internal_buffer[QSPI_INTERNAL_BUFFER_SIZE];

// Global debug print flag for the driver
static bool m_debug_on = false;

// =====================================================================
// QSPI Static Helper Functions (adapted from user's working snippet)
// =====================================================================

/**
 * @brief QSPI Event Handler - set to NULL in nrfx_qspi_init for polling mode
 * @param[in] event QSPI event.
 * @param[in] p_context Context passed to the handler.
 */
static void qspi_handler(nrfx_qspi_evt_t event, void *p_context) {
    // This handler is not actively used when nrfx_qspi_init is called with NULL.
    // It's kept here as a placeholder for completeness, if interrupt-driven
    // QSPI was desired later.
}

/**
 * @brief Prints the current QSPI status and busy check result.
 * @param[in] ASender String identifying the caller.
 */
static void qspi_print_status(const char ASender[]) {
    if (!m_debug_on) return;
    Serial.print("(");
    Serial.print(ASender);
    Serial.print(") QSPI Status: Reg=0x");
    Serial.print(*(QSPI_Status_Ptr), HEX);
    Serial.print(", nrfx_qspi_mem_busy_check() result=");
    Serial.println(nrfx_qspi_mem_busy_check());
}

/**
 * @brief Checks if the QSPI is ready for operations based on status register bits.
 * @return NRFX_SUCCESS if ready, NRFX_ERROR_BUSY otherwise.
 */
static nrfx_err_t qspi_is_ready() {
    volatile uint32_t current_status_reg_value = *(QSPI_Status_Ptr);
    // Check bit 3 (often indicates status validity/readiness) and bit 24 (busy flag)
    if (((current_status_reg_value & 0x08) == 0x08) && ((current_status_reg_value & 0x01000000) == 0)) {
        return NRFX_SUCCESS;
    } else {
        return NRFX_ERROR_BUSY;
    }
}

/**
 * @brief Waits for the QSPI to become ready, with a timeout.
 * @return NRFX_SUCCESS if ready within timeout, NRFX_ERROR_TIMEOUT otherwise.
 */
static nrfx_err_t qspi_wait_for_ready() {
    if (m_debug_on) {
        Serial.println("  [qspi_wait_for_ready] Entering loop...");
    }
    unsigned long start_time = millis();
    const unsigned long timeout_ms = 5000; // 5 seconds timeout
    unsigned long last_print_time = millis();

    while (qspi_is_ready() == NRFX_ERROR_BUSY) {
        if (millis() - last_print_time >= 500) { // Print "Still busy" every 500ms
            if (m_debug_on) {
                Serial.print("  [qspi_wait_for_ready] Still busy. RegVal: 0x"); Serial.print(*(QSPI_Status_Ptr), HEX);
                Serial.print(", BusyCheck: "); Serial.println(nrfx_qspi_mem_busy_check());
            }
            last_print_time = millis();
        }
        if (millis() - start_time >= timeout_ms) {
            Serial.println("  [qspi_wait_for_ready] Timeout: QSPI not ready!");
            return NRFX_ERROR_TIMEOUT;
        }
        nrf_delay_us(500); // Small delay to prevent tight busy-wait loop
    }
    if (m_debug_on) {
        Serial.println("  [qspi_wait_for_ready] Exited loop. QSPI is ready.");
    }
    return NRFX_SUCCESS;
}

/**
 * @brief Sends a Write Enable (WREN) command to the Flash chip.
 * This is often required before write or erase operations.
 * @return NRFX_SUCCESS if WREN command is sent and acknowledged, otherwise an error code.
 */
static nrfx_err_t qspi_flash_send_wren() {
    nrfx_err_t err;
    QSPICinstr_cfg.opcode = 0x06; // WREN command opcode
    QSPICinstr_cfg.length = NRF_QSPI_CINSTR_LEN_1B; // WREN is 1 byte opcode
    QSPICinstr_cfg.wren = false; // WREN itself does not need a preceding WREN
    QSPICinstr_cfg.io2_level = false; // WREN typically single-line operation
    QSPICinstr_cfg.io3_level = false; // WREN typically single-line operation

    err = qspi_wait_for_ready();
    if (err != NRFX_SUCCESS) {
        if (m_debug_on) { Serial.print("[qspi_flash_send_wren] QSPI not ready before WREN! Error: "); Serial.println(err); }
        return err;
    }
    err = nrfx_qspi_cinstr_xfer(&QSPICinstr_cfg, NULL, NULL);
    if (err != NRFX_SUCCESS) {
        if (m_debug_on) Serial.print("[qspi_flash_send_wren] WREN failed! Error: "); Serial.println(err);
    } else {
        if (m_debug_on) Serial.println("[qspi_flash_send_wren] WREN command sent.");
        nrf_delay_us(100); // Small delay after WREN
    }
    return err;
}


/**
 * @brief Reads a single byte from Flash Status Register (e.g., SR1 (0x05) or SR2 (0x35))
 * @param[in] command The opcode for the status register read (e.g., 0x05 for SR1, 0x35 for SR2).
 * @return The value of the status register, or 0xFF on error.
 */
static uint8_t qspi_read_flash_status_register(uint8_t command) {
    uint8_t status_reg = 0;
    nrf_qspi_cinstr_conf_t read_sr_instr_cfg = {
        .opcode = command, // e.g., 0x05 for SR1, 0x35 for SR2
        .length = NRF_QSPI_CINSTR_LEN_2B, // Opcode + 1 byte data
        .io2_level = false, // Status reads typically not quad-mode, use single line
        .io3_level = false, // Status reads typically not quad-mode, use single line
        .wipwait = QSPIWait,
        .wren = false
    };

    qspi_wait_for_ready();
    nrfx_err_t err = nrfx_qspi_cinstr_xfer(&read_sr_instr_cfg, NULL, &status_reg);

    if (err != NRFX_SUCCESS) {
        if (m_debug_on) {
            Serial.print("(qspi_read_flash_status_register) Failed to read status register 0x");
            Serial.print(command, HEX);
            Serial.print(". Error: "); Serial.println(err);
        }
        return 0xFF; // Return error indicator
    }
    return status_reg;
}


/**
 * @brief Configures the QSPI Flash memory chip itself (reset, set quad mode, etc.).
 * @return NRFX_SUCCESS if configuration is successful, otherwise an error code.
 */
static nrfx_err_t qspi_configure_memory() {
    uint8_t temporary[] = {0x00, 0x02}; // Data to write to status registers (0x02 for QE bit in SR2)
    nrfx_err_t err;

    if (m_debug_on) Serial.println("(qspi_configure_memory) Starting Flash config...");

    // Send Reset Enable Command (RSTEN)
    QSPICinstr_cfg = {
        .opcode     = QSPI_STD_CMD_RSTEN,
        .length     = NRF_QSPI_CINSTR_LEN_1B,
        .io2_level = true, // Set to true as in user's working snippet
        .io3_level = true, // Set to true as in user's working snippet
        .wipwait    = QSPIWait,
        .wren       = true // RSTEN requires Write Enable (controller-side)
    };
    err = qspi_wait_for_ready();
    if (err != NRFX_SUCCESS) {
        if (m_debug_on) { Serial.print("(qspi_configure_memory) QSPI not ready before RSTEN! Error: "); Serial.println(err); }
        return err;
    }
    err = nrfx_qspi_cinstr_xfer(&QSPICinstr_cfg, NULL, NULL);
    if (err != NRFX_SUCCESS) {
        if (m_debug_on) Serial.print("(qspi_configure_memory) RSTEN failed! Error: "); Serial.println(err);
        return err;
    } else {
        if (m_debug_on) Serial.println("(qspi_configure_memory) RSTEN command sent.");
        nrf_delay_us(100);
    }

    // Send Reset Command (RST)
    QSPICinstr_cfg.opcode = QSPI_STD_CMD_RST;
    QSPICinstr_cfg.wren = false; // RST usually doesn't need WREN
    err = qspi_wait_for_ready();
    if (err != NRFX_SUCCESS) {
        if (m_debug_on) { Serial.print("(qspi_configure_memory) QSPI not ready before RST! Error: "); Serial.println(err); }
        return err;
    }
    err = nrfx_qspi_cinstr_xfer(&QSPICinstr_cfg, NULL, NULL);
    if (err != NRFX_SUCCESS) {
        if (m_debug_on) Serial.print("(qspi_configure_memory) RST failed! Error: "); Serial.println(err);
        return err;
    } else {
        if (m_debug_on) Serial.println("(qspi_configure_memory) RST command sent.");
        nrf_delay_us(100);
    }

    // Explicitly send Write Enable (WREN) command before WRSR
    err = qspi_flash_send_wren();
    if (err != NRFX_SUCCESS) return err;

    // Write Status Register (WRSR) to enable QSPI mode (e.g., Quad Enable bit)
    QSPICinstr_cfg.opcode = QSPI_STD_CMD_WRSR;
    QSPICinstr_cfg.length = NRF_QSPI_CINSTR_LEN_3B; // Opcode (1B) + 2 data bytes = 3B total
    QSPICinstr_cfg.wren = true; // WRSR requires Write Enable (controller-side, but already sent to flash)
    QSPICinstr_cfg.io2_level = true; // Still true for the WRSR command itself
    QSPICinstr_cfg.io3_level = true; // Still true for the WRSR command itself
    err = qspi_wait_for_ready();
    if (err != NRFX_SUCCESS) {
        if (m_debug_on) { Serial.print("(qspi_configure_memory) QSPI not ready before WRSR! Error: "); Serial.println(err); }
        return err;
    }
    err = nrfx_qspi_cinstr_xfer(&QSPICinstr_cfg, temporary, NULL);
    if (err != NRFX_SUCCESS) {
        if (m_debug_on) Serial.print("(qspi_configure_memory) WRSR failed! Error: "); Serial.println(err);
        return err;
    } else {
        if (m_debug_on) Serial.println("(qspi_configure_memory) WRSR command sent.");
        nrf_delay_us(100);
    }
    // This qspi_wait_for_ready() is for the WRSR command itself to complete
    err = qspi_wait_for_ready();
    if (err != NRFX_SUCCESS) {
        if (m_debug_on) { Serial.print("(qspi_configure_memory) WRSR completion failed! Error: "); Serial.println(err); }
        return err;
    }
    qspi_print_status("qspi_configure_memory");

    // --- Read back Status Registers to verify Quad Enable bit ---
    if (m_debug_on) {
        Serial.println("Verifying Flash Status Registers after WRSR:");
        uint8_t sr1_val = qspi_read_flash_status_register(0x05); // Read Status Register 1 (SR1)
        Serial.print("  SR1 (0x05): 0x"); Serial.println(sr1_val, HEX);
        uint8_t sr2_val = qspi_read_flash_status_register(0x35); // Read Status Register 2 (SR2) (common for QE bit)
        Serial.print("  SR2 (0x35): 0x"); Serial.println(sr2_val, HEX);

        // Common check for QE bit (e.g., bit 1 of SR2 for some chips, meaning SR2 should be 0x02)
        if (sr2_val != 0xFF && (sr2_val & 0x02) == 0x02) {
            Serial.println("  Quad Enable (QE) bit appears to be set in SR2. (Expected: 0x02)");
        } else {
            Serial.println("  WARNING: Quad Enable (QE) bit NOT set as expected in SR2, or SR2 read failed!");
            Serial.println("  Check your Flash chip datasheet for correct QE bit and WRSR value.");
            return NRFX_ERROR_INVALID_STATE; // Consider returning error if QE is critical for success
        }
    }

    if (m_debug_on) Serial.println("(qspi_configure_memory) Flash config complete.");
    return NRFX_SUCCESS; // Configuration successfully completed
}


// =====================================================================
// Public Functions (declared in qspi_flash.h)
// =====================================================================

nrfx_err_t qspi_flash_init(void) {
    nrfx_err_t Error_Code;

    // Initialize NRF_LOG as per user's working snippet
    NRF_LOG_INIT(NULL);
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    QSPIConfig.xip_offset = NRFX_QSPI_CONFIG_XIP_OFFSET;
    QSPIConfig.pins = {
       .sck_pin    = 21, .csn_pin    = 25, .io0_pin    = 20,
       .io1_pin    = 24, .io2_pin    = 22, .io3_pin    = 23,
    };
    QSPIConfig.irq_priority = (uint8_t)NRFX_QSPI_CONFIG_IRQ_PRIORITY;
    QSPIConfig.prot_if = {
        .readoc     = (nrf_qspi_readoc_t)NRF_QSPI_READOC_READ4O, // Set for Quad Read
        .writeoc    = (nrf_qspi_writeoc_t)NRF_QSPI_WRITEOC_PP4O, // Set for Quad Write
        .addrmode   = (nrf_qspi_addrmode_t)NRFX_QSPI_CONFIG_ADDRMODE,
        .dpmconfig  = false,
    };
    QSPIConfig.phy_if.sck_freq   = (nrf_qspi_frequency_t)NRF_QSPI_FREQ_32MDIV2;
    QSPIConfig.phy_if.spi_mode   = (nrf_qspi_spi_mode_t)NRFX_QSPI_CONFIG_MODE;
    QSPIConfig.phy_if.dpmen      = false;

    // Setup QSPI to allow for DPM but with it turned off
    QSPIConfig.prot_if.dpmconfig = true;
    NRF_QSPI->DPMDUR = (QSPI_DPM_ENTER << 16) | QSPI_DPM_EXIT;

    if (m_debug_on) Serial.println("(qspi_flash_init) Calling nrfx_qspi_init...");
    Error_Code = 1; // Initialize with non-success code
    while (Error_Code != NRFX_SUCCESS) {
        // Calling nrfx_qspi_init with NULL, NULL for handler and context for polling mode
        Error_Code = nrfx_qspi_init(&QSPIConfig, NULL, NULL);
        if (Error_Code != NRFX_SUCCESS) {
            if (m_debug_on) {
                Serial.print("(qspi_flash_init) nrfx_qspi_init failed: ");
                Serial.println(Error_Code);
            }
            nrf_delay_ms(100);
        } else {
            if (m_debug_on) Serial.println("(qspi_flash_init) nrfx_qspi_init successful.");
        }
    }

    qspi_print_status("qspi_flash_init (Before qspi_configure_memory)");
    nrfx_err_t config_err = qspi_configure_memory(); // Configure the Flash chip itself
    if (config_err != NRFX_SUCCESS) {
        if (m_debug_on) { Serial.print("(qspi_flash_init) Flash configuration failed! Error: "); Serial.println(config_err); }
        return config_err;
    }
    qspi_print_status("qspi_flash_init (After qspi_configure_memory)");

    if (m_debug_on) Serial.println("(qspi_flash_init) Activating QSPI...");
    NRF_QSPI->TASKS_ACTIVATE = 1;
    if (m_debug_on) Serial.println("(qspi_flash_init) TASKS_ACTIVATE triggered.");

    if (m_debug_on) Serial.println("(qspi_flash_init) Final qspi_wait_for_ready...");
    nrfx_err_t final_wait_err = qspi_wait_for_ready();
    if (final_wait_err != NRFX_SUCCESS) {
        if (m_debug_on) { Serial.print("(qspi_flash_init) QSPI not ready after init. Error: "); Serial.println(final_wait_err); }
    } else {
        if (m_debug_on) Serial.println("(qspi_flash_init) QSPI fully ready.");
    }
    return final_wait_err;
}

nrfx_err_t qspi_flash_erase_block(uint32_t address) {
    nrfx_err_t err;

    if (m_debug_on) {
        Serial.print("(qspi_flash_erase_block) Erasing 64KB page at 0x");
        Serial.println(address, HEX);
    }

    nrfx_err_t wait_err = qspi_wait_for_ready();
    if (wait_err != NRFX_SUCCESS) {
        if (m_debug_on) { Serial.print("(qspi_flash_erase_block) QSPI not ready before erase. Error: "); Serial.println(wait_err); }
        return wait_err;
    }

    // Explicitly send Write Enable (WREN) command before Erase
    err = qspi_flash_send_wren();
    if (err != NRFX_SUCCESS) return err;

    nrfx_err_t err_code = nrfx_qspi_erase(NRF_QSPI_ERASE_LEN_64KB, address);
    if (err_code != NRFX_SUCCESS) {
        if (m_debug_on) {
            Serial.print("(qspi_flash_erase_block) Failed to send erase command for 0x");
            Serial.print(address, HEX);
            Serial.print(". Error: ");
            Serial.println(err_code);
        }
        return err_code;
    } else {
        if (m_debug_on) Serial.println("(qspi_flash_erase_block) Erase command sent. Waiting for completion...");
    }

    wait_err = qspi_wait_for_ready();
    if (wait_err != NRFX_SUCCESS) {
        if (m_debug_on) { Serial.print("(qspi_flash_erase_block) QSPI not ready after erase. Error: "); Serial.println(wait_err); }
        return wait_err;
    }
    if (m_debug_on) Serial.println("(qspi_flash_erase_block) Erase successful and QSPI is ready.");
    return NRFX_SUCCESS;
}


nrfx_err_t qspi_flash_write(const uint8_t *p_data, uint32_t length, uint32_t address) {
    if (m_debug_on) {
        Serial.print("(qspi_flash_write) Writing ");
        Serial.print(length);
        Serial.print(" bytes to 0x");
        Serial.println(address, HEX);
    }

    nrfx_err_t wait_status = qspi_wait_for_ready();
    if (wait_status != NRFX_SUCCESS) {
        if (m_debug_on) { Serial.print("(qspi_flash_write) Write failed (not ready): "); Serial.println(wait_status); }
        return wait_status;
    }

    // Explicitly send Write Enable (WREN) command before Write
    nrfx_err_t err = qspi_flash_send_wren();
    if (err != NRFX_SUCCESS) return err;

    nrfx_err_t err_code = nrfx_qspi_write(p_data, length, address);

    if (err_code == NRFX_SUCCESS) {
        if (m_debug_on) Serial.println("(qspi_flash_write) Write command sent. Waiting for Flash to complete...");
        wait_status = qspi_wait_for_ready();
        if (wait_status == NRFX_SUCCESS) {
            if (m_debug_on) Serial.println("(qspi_flash_write) Write to Flash successful.");
            return NRFX_SUCCESS;
        } else {
            if (m_debug_on) Serial.print("(qspi_flash_write) Error: Flash not ready after write! Error: "); Serial.println(wait_status);
            return wait_status;
        }
    } else {
        if (m_debug_on) Serial.print("(qspi_flash_write) Error: nrfx_qspi_write failed! Error: "); Serial.println(err_code);
        return err_code;
    }
}

nrfx_err_t qspi_flash_read(uint8_t *p_buffer, uint32_t length, uint32_t address) {
    if (m_debug_on) {
        Serial.print("(qspi_flash_read) Reading ");
        Serial.print(length);
        Serial.print(" bytes from 0x");
        Serial.println(address, HEX);
    }

    nrfx_err_t wait_status = qspi_wait_for_ready();
    if (wait_status != NRFX_SUCCESS) {
        if (m_debug_on) { Serial.print("(qspi_flash_read) Read failed (not ready): "); Serial.println(wait_status); }
        return wait_status;
    }

    nrfx_err_t err_code = nrfx_qspi_read(p_buffer, length, address);

    if (err_code == NRFX_SUCCESS) {
        if (m_debug_on) Serial.println("(qspi_flash_read) Read command sent. Waiting for Flash to complete...");
        wait_status = qspi_wait_for_ready();
        if (wait_status == NRFX_SUCCESS) {
            if (m_debug_on) Serial.println("(qspi_flash_read) Read from Flash successful.");
            return NRFX_SUCCESS;
        } else {
            if (m_debug_on) Serial.print("(qspi_flash_read) Error: Flash not ready after read! Error: "); Serial.println(wait_status);
            return wait_status;
        }
    } else {
        if (m_debug_on) Serial.print("(qspi_flash_read) Error: nrfx_qspi_read failed! Error: "); Serial.println(err_code);
        return err_code;
    }
}

bool qspi_flash_write_string(const char* data_str, uint32_t address, uint32_t max_len) {
    uint32_t actual_str_len = strlen(data_str);
    uint32_t len_with_null = actual_str_len + 1; // String length + 1 for null terminator

    // Calculate padded length to be a multiple of 4 bytes (for Quad SPI alignment)
    uint32_t padded_len = (len_with_null + 3) & ~3; // Rounds up to the next multiple of 4

    if (padded_len > QSPI_INTERNAL_BUFFER_SIZE) {
        Serial.println("Error: Padded string length exceeds internal buffer capacity for writing.");
        return false;
    }
    if (padded_len > max_len) {
        Serial.println("Error: Padded string length exceeds provided max_len for writing.");
        return false;
    }

    // Clear the part of s_qspi_internal_buffer that will be written to ensure padding bytes are 0x00
    memset(s_qspi_internal_buffer, 0, padded_len);

    // Copy string content
    memcpy(s_qspi_internal_buffer, data_str, actual_str_len);

    // Explicitly place the null terminator at the correct position
    s_qspi_internal_buffer[actual_str_len] = '\0';


    if (m_debug_on) {
      Serial.print("[qspi_flash_write_string] Attempting to write '");
      Serial.print(data_str);
      Serial.print("' (actual len: ");
      Serial.print(actual_str_len);
      Serial.print(", len with null: ");
      Serial.print(len_with_null);
      Serial.print(" bytes) to 0x");
      Serial.println(address, HEX);
      Serial.print("Writing padded length: ");
      Serial.print(padded_len);
      Serial.println(" bytes.");
      Serial.print("Raw bytes to write (padded): ");
      for (uint32_t i = 0; i < padded_len; i++) {
          Serial.print(s_qspi_internal_buffer[i], HEX);
          Serial.print(" ");
      }
      Serial.println();
    }

    nrfx_err_t err_code = qspi_flash_write(s_qspi_internal_buffer, padded_len, address);

    return (err_code == NRFX_SUCCESS);
}

bool qspi_flash_read_string(char* buffer, uint32_t address, uint32_t max_len) {
    memset(buffer, 0, max_len); // Clear buffer

    if (max_len == 0) return false;

    if (m_debug_on) {
      Serial.print("[qspi_flash_read_string] Attempting to read up to ");
      Serial.print(max_len);
      Serial.print(" bytes from 0x");
      Serial.println(address, HEX);
    }

    // Read max_len bytes into the user-provided buffer
    nrfx_err_t err_code = qspi_flash_read((uint8_t*)buffer, max_len, address);

    if (err_code == NRFX_SUCCESS) {
        // Ensure the buffer is null-terminated, even if max_len bytes were read
        // and no null was found within them. This prevents buffer overruns
        // if the string on flash is not properly null-terminated or longer than expected.
        buffer[max_len - 1] = '\0';
        if (m_debug_on) {
            Serial.print("[qspi_flash_read_string] Read raw bytes: ");
            for (uint32_t i = 0; i < max_len; i++) {
                Serial.print(((uint8_t*)buffer)[i], HEX);
                Serial.print(" ");
            }
            Serial.println();
        }
        return true;
    } else {
        return false;
    }
}