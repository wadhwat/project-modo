# MODO Architecture

MODO is a BMO-shaped embedded AI companion built on a Raspberry Pi 5 and an STM32 coprocessor. The core design principle: **build a clean, swappable platform first** — cloud/local/mock AI providers behind one interface, hardware behind an abstraction layer, and physical controls that work at the OS level, not just inside MODO's own software.

## System overview

```
Physical Device (BMO-style enclosure)
├── 5" touchscreen (front, top)
├── Cartridge/media slot (NFC / storage) — above D-pad
├── Camera (right of cartridge slot)
├── D-pad (lower left) + 3 buttons (lower right)
├── Speaker/mic slots (bottom front)
├── Rotary encoder w/ push (side) — volume/mute
├── Rear I/O: 2x USB-A, USB-C power, debug hatch, ventilation
├── Raspberry Pi 5 (16GB) + NVMe SSD
└── STM32F411 controller board (accelerator-ready internal layout)
```

## Hardware architecture

### Raspberry Pi 5 — main compute

Runs embedded Linux and all application services: backend, UI, speech pipeline, agent runtime, and tools. NVMe storage (not SD-only) — model loading and storage speed are known bottlenecks on this class of device. The enclosure reserves space for a future AI accelerator (Hailo-10H class), but none is used in the MVP.

### STM32F411 — dual-role controller

The STM32 is deliberately **not** just a GPIO reader. It has two independent interfaces:

1. **USB HID keyboard** — button presses become OS-level keyboard events, usable by any application (browser, terminal, emulator, MODO's UI). Gamepad and media-key modes are planned follow-ups.
2. **UART control/telemetry channel** — the Pi sends commands (LED state, device mode); the STM32 sends ACKs, button events, and heartbeat/status messages.

MVP wiring is HID-over-USB + separate UART; a composite USB device (HID + CDC on one cable) is a later refinement.

Firmware v1 responsibilities: button-matrix scanning, debounce, D-pad mapping, HID output, UART command parser, LED state machine, heartbeats. Bare-metal first; FreeRTOS later if warranted.

### HID mapping (v1)

| Physical input | HID output |
|---|---|
| D-pad up/down/left/right | Arrow keys |
| Red button | Enter |
| Green button | Escape |
| Blue/triangle button | F13 (assistant / push-to-talk) |
| Rotary CW / CCW | Volume up / down |
| Rotary press | Mute / F14 |

Uncommon function keys (F13–F16) are reserved for assistant actions so they never conflict with normal typing.

## Software architecture (Raspberry Pi)

```
app/
├── FastAPI backend + WebSocket state stream
├── Touchscreen web UI (kiosk mode)
├── Speech pipeline: push-to-talk → STT → agent → TTS
├── Agent runtime
│   ├── LLMProvider interface (cloud / local / mock)
│   ├── Tool registry + JSON tool schemas
│   ├── Policy layer (confirmation-required tools, e.g. email)
│   └── Conversation state
├── Tool layer (device, timers, notes/RAG, camera, status)
├── Memory layer: SQLite state/logs, local notes, optional embeddings
└── Hardware abstraction layer
    ├── UART client + protocol parser
    ├── Mock hardware (for tests, no device needed)
    └── Device state
```

### Provider split

No single model is responsible for everything. Separate provider interfaces for **text LLM**, **vision**, **embeddings**, and **speech**, each independently swappable. MVP runs a cloud LLM as primary, a mock provider for tests, and an optional small local model (Ollama/llama.cpp) as a benchmarked fallback — local inference is an evaluated design axis (quality vs. latency vs. privacy vs. flexibility), not a requirement.

### Prompting constraints

Small local models degrade with long prompts, so: short system prompt, tool schemas outside the prose, memory retrieved on demand rather than stuffed into every turn, and small specialized prompts for tool routing.

### Tool suite

**Tier 1 (MVP):** `set_device_mode`, `set_led_state`, `get_device_status`, `get_system_status`, `start_timer`, `cancel_timer`, `list_timers`, `show_ui_message`, `search_local_notes`, `capture_image`

**Tier 2 (post-MVP):** `describe_camera_view`, `get_weather`, `draft_email`, `summarize_day`, `search_web_or_news`

**Tier 3 (gated):** `send_email` (confirmation-required), `safe_shell_command`, `file_write`, `calendar_create_event`

### Cartridge / personality modes

The front slot is implemented as an NFC/storage reader rather than a real disk mechanism. Inserting a cartridge triggers a personality mode: a short personality profile, LED theme, and tool loadout swap. Modes stay lightweight (short structured context, retrieved on demand) to keep local-model latency acceptable.

## Performance as a first-class metric

Latency is logged at every stage: STT, LLM time-to-first-token, tokens/sec, tool-call latency, TTS, camera capture, UART round-trip, UI update. These logs back the cloud-vs-local-vs-accelerator evaluation.

## Key design decisions

| Decision | Rationale |
|---|---|
| STM32 as USB HID, not GPIO-only | GPIO buttons only work in custom scripts; HID works OS-wide in any app |
| HID + separate UART for MVP (composite USB later) | Easier to implement and debug; composite device is a refinement, not a blocker |
| Cloud LLM primary, local as benchmark | Pi 5 CPU handles ~1.5B-param models; accelerators constrain model choice |
| No accelerator purchase up front | Baseline benchmarks first; enclosure is accelerator-ready |
| Fake the disk slot (NFC/storage, not a drive) | Keeps the iconic look without the bulk and reliability risk |
| Services + state machine, not one Python loop | Real event bus, tool registry, and hardware/speech/UI services |
| Split text/vision/embedding/speech providers | One multimodal model for everything performs worse at each job |

## MVP demo path

1. Device boots into touchscreen UI; front controls navigate it as real keyboard input.
2. Assistant button (F13) triggers push-to-talk: "Start focus mode for 25 minutes" → agent calls timer + LED + UI tools; STM32 changes LED state.
3. "What do you see?" → camera capture → vision description.
4. "What did I decide about the STM32 interface?" → notes search → grounded answer.

## Build order (critical path)

1. STM32 USB HID prototype (D-pad → arrow keys) — the foundational milestone
2. STM32 UART command/status channel
3. Pi backend with mock hardware + tool registry
4. Touchscreen UI
5. Speech pipeline
6. Camera + notes/RAG
7. Local-LLM experiment, accelerator decision, enclosure polish
