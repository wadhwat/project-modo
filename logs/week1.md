## Architecture decisions

Decision: Use the Raspberry Pi 5 as the main application computer and the Adafruit Feather STM32F405 Express as the dedicated physical-hardware controller.

Alternative considered: Connect the buttons, encoder, and NFC reader directly to Raspberry Pi GPIO.

Reasoning: The STM32 provides deterministic hardware handling, standard USB HID operation, and a stable abstraction between the physical controls and the application.

Tradeoff: The project requires two communication paths and additional firmware.

Revisit trigger: Reconsider the split only if the STM32 cannot reliably provide USB HID and UART simultaneously, or if the extra hardware prevents the physical design from fitting inside the enclosure.

Much of the planning for this decision can be found in ../docs/basic_veritcal_slice.md