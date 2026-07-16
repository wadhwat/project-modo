## USB HID Interface

### Purpose

The STM32 converts physical front-panel controls into standard USB HID keyboard and consumer-control inputs. These inputs must work on an ordinary computer without the Raspberry Pi application, UART connection, or custom input software.

### Control mappings

The initial mappings are provisional and should be stored in a centralized firmware configuration so they can be changed without restructuring the input logic.

| Physical control | Initial HID output |
|---|---|
| D-pad Up | Up Arrow |
| D-pad Down | Down Arrow |
| D-pad Left | Left Arrow |
| D-pad Right | Right Arrow |
| Red button | Enter |
| Green button | Escape |
| Blue/triangle button | F13 |
| Rotary encoder clockwise | Volume Up |
| Rotary encoder counterclockwise | Volume Down |
| Rotary encoder press | F14 or Mute |

### Press, hold, release, and repeat behavior

Each mechanical input has:

- A raw electrical state
- A candidate state
- A timestamp for the most recent raw transition
- An accepted debounced state

A raw transition does not immediately change the HID report. The new state must remain stable for an initial configurable debounce interval of approximately 15 milliseconds.

After a valid press:

- The key is included in the HID report.
- It remains pressed for as long as the debounced input remains active.
- The STM32 does not generate repeated artificial taps.
- Normal key-repeat behavior is controlled by the host operating system.

After a valid release:

- The key is removed from the HID report.
- A release report is sent promptly.
- No key may remain logically pressed after the physical control is released.

Multiple simultaneous controls should be represented in the same HID report where supported.

The rotary encoder’s volume actions use HID consumer-control reports rather than ordinary keyboard-key reports.

---

## STM32–Pi UART Interface

### Purpose

UART provides project-specific communication between the STM32 and Raspberry Pi. It is separate from USB HID:

- USB HID carries ordinary keyboard and media-control inputs.
- UART carries NFC events, hardware status, errors, and Pi-to-STM32 commands.
- Keyboard events are not duplicated over UART.

### Transport configuration

The initial UART configuration is:

- 115200 baud
- 8 data bits
- No parity
- 1 stop bit
- 3.3-volt logic
- Bidirectional communication

### Message framing

Protocol v1 uses newline-terminated printable ASCII messages. This format is easy to inspect in a serial terminal and simple to parse on both devices.

Each message follows:

```text
M1|SEQUENCE|TYPE|OPTIONAL_FIELDS\n
```

Where:

- `M1` identifies protocol version 1.
- `SEQUENCE` associates commands with responses.
- `TYPE` identifies the message.
- Additional fields are separated with `|`.
- `\n` terminates the message.
- The maximum frame length is 128 bytes.
- Unsolicited STM32 events use sequence `0`.
- Pi commands use a nonzero sequence number.

Examples:

```text
M1|0|NFC_PRESENT|04A72C91
M1|0|NFC_REMOVED
M1|21|GET_STATUS
M1|21|STATUS|READY|NFC_PRESENT|04A72C91
M1|22|SET_LED|1
M1|22|ACK|SET_LED
M1|22|ERR|INVALID_ARGUMENT
```

Protocol v1 does not include a checksum. The connection is short, internal, and low-speed. A checksum or binary framing protocol can be added if testing reveals corruption.

### STM32-to-Pi messages

| Message | Purpose |
|---|---|
| `BOOT` | Announces STM32 startup, firmware version, and reset reason |
| `NFC_PRESENT` | Reports that a tag is present and includes its raw UID |
| `NFC_REMOVED` | Reports that the previously detected tag is no longer present |
| `STATUS` | Responds to `GET_STATUS` with current hardware state |
| `HEARTBEAT` | Indicates that the STM32 remains responsive |
| `ACK` | Confirms successful completion of a command |
| `ERR` | Reports a rejected command or hardware/protocol error |
| `PONG` | Responds to a Pi `PING` command |

The STM32 sends `NFC_PRESENT` only when the NFC state changes. It does not repeatedly send the same UID while a cartridge remains inserted.

### Pi-to-STM32 messages

| Message | Purpose |
|---|---|
| `GET_STATUS` | Requests current NFC, firmware, and hardware-health state |
| `PING` | Tests whether the UART connection is responsive |
| `SET_LED` | Controls the Feather’s status LED or a future indicator |

Only these commands are required for Protocol v1. Additional commands should only be added when a real feature requires them.

### Acknowledgments

Every Pi command with a nonzero sequence number receives exactly one terminal response using the same sequence number:

- `ACK`
- `ERR`
- A command-specific response such as `STATUS` or `PONG`

Example:

```text
Pi:    M1|31|SET_LED|1
STM32: M1|31|ACK|SET_LED
```

The Pi waits up to one second for a response and retries once. If the retry also fails, the hardware connection is marked unavailable.

Unsolicited events such as `NFC_PRESENT` and `HEARTBEAT` do not require acknowledgments. If an NFC event is missed, the Pi recovers the authoritative state using `GET_STATUS`.

### Heartbeat

The STM32 sends a heartbeat approximately once per second:

```text
M1|0|HEARTBEAT|UPTIME_MS
```

If the Pi misses three consecutive heartbeats, it marks the STM32 connection unavailable. A heartbeat confirms communication health but does not replace `GET_STATUS`.

### Malformed messages

When either side receives a malformed frame:

- The frame is discarded.
- The parser resumes at the next newline.
- Frames longer than 128 bytes are discarded until the next newline.
- Unknown protocol versions are rejected.
- Unknown commands generate `ERR|UNKNOWN_COMMAND` when a valid sequence number is available.
- Invalid fields generate `ERR|INVALID_ARGUMENT`.
- Malformed unsolicited messages may be logged without a response.
- Neither device crashes or blocks indefinitely because of malformed data.

### Reset and reconnection

When the STM32 resets:

1. Physical outputs return to safe defaults.
2. The first USB HID report contains no pressed keys.
3. The PN532 is reinitialized.
4. The STM32 sends `BOOT`.
5. Normal heartbeat and event reporting resume.

When the Pi receives `BOOT`, it discards cached STM32 state and sends `GET_STATUS`.

When the Pi restarts:

1. The STM32 continues handling USB HID and monitoring the PN532 while powered.
2. The Pi opens the UART connection.
3. The Pi sends `GET_STATUS`.
4. The returned status becomes the Pi’s initial hardware-state snapshot.

The system does not assume that either device observed every event while disconnected.

---

## Device-State Ownership

### Ownership model

Every important piece of state has one authoritative owner. The other device may cache or display that state, but it does not become a second source of truth.

| State | Authoritative owner | Reason |
|---|---|---|
| Raw GPIO readings | STM32 | Directly observes the physical pins |
| Debounced button states | STM32 | Performs switch filtering and state transitions |
| Current USB HID report | STM32 | Generates the report sent to the USB host |
| Rotary-encoder state | STM32 | Directly monitors the encoder signals |
| NFC tag present or absent | STM32 | Directly operates the PN532 |
| Current raw NFC UID | STM32 | Obtained directly from the NFC reader |
| PN532 health | STM32 | Initializes and monitors the reader |
| STM32 firmware version, uptime, and reset reason | STM32 | Local hardware information |
| Desired LED or indicator state | Pi | Chooses what the indicator should represent |
| Actual applied LED or indicator state | STM32 | Physically drives the output |
| UID-to-cartridge-mode mapping | Pi | Application-level interpretation |
| Active personality or mode | Pi | Part of global application state |
| Touchscreen and UI state | Pi | The Pi runs the user interface |
| Camera, microphone, and audio state | Pi | These peripherals connect directly to the Pi |
| Assistant, tool, and AI state | Pi | High-level application responsibility |
| Network and external-service state | Pi | The STM32 does not use network services |

### Ownership rules

1. The STM32 reports raw NFC UIDs; it does not assign personality names.
2. The Pi does not independently decide whether a tag is physically present.
3. The Pi may cache the latest STM32 status, but that cache is invalidated after disconnection or STM32 reset.
4. After connection or reconnection, the Pi sends `GET_STATUS`.
5. A Pi command represents desired hardware state.
6. The STM32 response reports whether the desired state was physically applied.
7. The STM32 continues handling buttons and USB HID when the Pi application is unavailable.
8. Neither side silently reconstructs state that it does not own.

---

## Pi Hardware-Controller Interface

### Purpose

The Pi application does not access UART directly from multiple modules. One hardware-controller module owns the serial connection and translates UART frames into application-level requests, events, and state.

USB HID is not handled by this module. HID events enter Linux through its standard input subsystem.

### Responsibilities

The Pi hardware-controller module is responsible for:

- Opening and configuring the UART connection
- Parsing and validating Protocol v1 frames
- Assigning command sequence numbers
- Matching responses to pending commands
- Enforcing timeouts and retry behavior
- Monitoring heartbeats
- Maintaining the latest reported hardware status
- Translating UART frames into application-level events
- Reporting connection and hardware errors
- Resynchronizing state after reconnection

Only this module may read from or write to the STM32 UART.

### Application requests

| Request | Result |
|---|---|
| Start hardware connection | Opens UART and synchronizes state |
| Stop hardware connection | Closes UART cleanly |
| Get hardware status | Returns the latest confirmed STM32 status |
| Refresh hardware status | Sends `GET_STATUS` and waits for a response |
| Ping hardware | Tests UART responsiveness |
| Set indicator state | Sends `SET_LED` or a future equivalent |
| Check connection state | Reports connected, reconnecting, or unavailable |

These are conceptual operations, not final Python function signatures.

### Application events

| Event | Information |
|---|---|
| Hardware connected | Firmware version and capabilities |
| Hardware disconnected | Timeout or error reason |
| NFC tag presented | Raw UID |
| NFC tag removed | Previous UID, if known |
| Hardware status updated | Current STM32 status snapshot |
| Hardware error | Error code and relevant details |
| STM32 restarted | Firmware version and reset reason |

The cartridge-mode service—not the hardware-controller module—maps raw NFC UIDs to personalities.

### Real and mock implementations

The application depends on the conceptual hardware-controller interface instead of directly depending on UART.

Two implementations will eventually exist:

- **Real hardware controller:** Communicates with the STM32 over UART.
- **Mock hardware controller:** Generates simulated NFC, connection, and hardware-status events without requiring an STM32.

Higher-level application code operates the same way with either implementation.

### Failure behavior

If the STM32 is unavailable:

- The Pi application continues running.
- The UI may report that physical hardware is unavailable.
- UART commands fail with a controlled error or timeout.
- The hardware-controller module periodically attempts reconnection.
- Stale cached NFC state is not treated as current.
- AI, UI, camera, and audio features that do not require the STM32 may continue operating.

---

## Protocol v1 Decisions to Review

- UART uses 115200 baud and 8N1.
- Messages use human-readable, newline-delimited ASCII.
- Fields are separated with `|`.
- Frames are limited to 128 bytes.
- Protocol v1 does not include a checksum.
- Commands time out after one second and are retried once.
- The STM32 sends one heartbeat per second.
- The Pi declares the connection unavailable after three missed heartbeats.
- Unsolicited events are not acknowledged.
- `GET_STATUS` repairs state after missed events or reconnection.
- One Pi module exclusively owns the UART.
- NFC UID interpretation remains outside the hardware-controller module.