# Seeed Studio XIAO ESP32C5 Factory Test Firmware

This project contains the factory test firmware for the Seeed Studio XIAO ESP32C5 development board. It is designed to verify the hardware functionality (GPIO, Wireless, Battery, Button) during the manufacturing process and provide a user-friendly experience for the end user.

## Project Structure

```
ESP32C5_Test/
├── ESP32C5_Test.ino          # Main Arduino Sketch
├── src/
│   ├── XIAO_Common/          # Generic XIAO Utilities
│   │   ├── XIAO_Generic.h    # Pin definitions and utility class headers
│   │   └── XIAO_Generic.cpp  # GPIO, Battery, and LED implementation
│   └── XIAO_ESP32/           # ESP32 Specific Features
│       ├── XIAO_ESP32_Lib.h  # WiFi, BLE, and NVS headers
│       └── XIAO_ESP32_Lib.cpp# WiFi/BLE scanning and NVS storage
└── README.md                 # This file
```

## Operation Modes

The firmware operates in two distinct modes, determined by the state of the **D1** pin at startup:

### 1. Factory Test Mode
*   **Trigger**: Connect **D1** to **GND** (LOW) before powering on or resetting.
*   **Purpose**: Automated hardware verification on the factory test fixture.
*   **Behavior**:
    *   Initializes UART0 (`MySerial0`) for communication with the test fixture.
    *   Initializes USB Serial (`Serial`) for debugging.
    *   **Dual Logging**: All test logs are sent to *both* UART0 and USB Serial.
    *   **Test Sequence**:
        1.  **Button Test**: Checks if the BOOT button is functional.
        2.  **Wireless Test**: Scans for WiFi networks and Bluetooth devices.
        3.  **GPIO Test**: Checks input/output pins (requires loopback on fixture).
        4.  **Battery Test**: Measures battery voltage via the onboard divider.
    *   **Result Storage**: The final result (PASS/FAIL) is saved to Non-Volatile Storage (NVS).
    *   **LED Indication**:
        *   **PASS**: LED stays ON (after blinking).
        *   **FAIL**: LED blinks continuously.

### 2. Normal Mode (Shipping Mode)
*   **Trigger**: Leave **D1** floating (HIGH) at startup.
*   **Purpose**: Default behavior for the end user.
*   **Behavior**:
    *   Reads the saved test result from NVS.
    *   **If PASSED**: Prints "Hello from Seeed Studio XIAO ESP32-C5" to USB Serial and blinks the LED (Heartbeat).
    *   **If FAILED**: Prints "Device test FAILED".
    *   **If NOT RUN**: Prints "Device not tested".

## Hardware Requirements for Testing

*   **Test Fixture**: A jig that connects D1 to GND to trigger test mode.
*   **Loopback Connections**: For the GPIO test to pass, specific output pins must be connected to input pins (as defined in `XIAO_Generic.h`).
*   **Battery**: A battery or simulated voltage source connected to the battery pads for the battery test.

## Compilation & Upload

1.  Open `ESP32C5_Test.ino` in Arduino IDE.
2.  Select Board: **Seeed Studio XIAO ESP32C5**.
3.  Select Port: The COM port of the device.
4.  Click **Upload**.

## Debugging

*   **Upload Issues**: If upload fails with "Fatal error occurred", try:
    *   Lowering the Upload Speed (e.g., to 115200).
    *   Holding the BOOT button while connecting USB.
    *   Using a different USB cable.
*   **Viewing Logs**: Open Serial Monitor at **115200 baud**.
