# Project MODO

**MODO: A Cartridge-Driven Embedded AI Companion**

A Raspberry Pi–STM32 embedded platform integrating USB HID controls, voice, vision, modular AI tools, and NFC-triggered personality modes — housed in a BMO-inspired enclosure.

## What it does

- **Physical controls that work everywhere** — a D-pad, three buttons, and a rotary encoder are exposed by an STM32 as a real USB HID keyboard, so they work in any app on the device, not just MODO's own software.
- **Voice + vision assistant** — push-to-talk speech in/out and a front-facing camera, driven by an agent runtime with a curated tool suite (timers, LEDs, device modes, notes search, camera capture).
- **Cartridge slot** — a front media slot (NFC/storage) that triggers personality modes and tool loadouts.
- **Swappable intelligence** — an `LLMProvider` interface supporting cloud, local (Ollama/llama.cpp), and mock providers; local inference is a benchmarked design axis, not a hard dependency.

## Architecture

See [docs/architecture.md](docs/architecture.md) for the full system design.

```
project-modo/
├── app/          # Raspberry Pi services: backend, agent, tools, speech, UI
├── firmware/     # STM32 HID controller firmware
├── docs/         # Architecture, protocol, and design-decision docs
├── enclosure/    # BMO-style enclosure CAD
└── README.md
```

## Status

Week 1 — architecture locked, parts ordered, repo skeleton in place. First milestone: STM32 D-pad registering as arrow keys over USB HID.
