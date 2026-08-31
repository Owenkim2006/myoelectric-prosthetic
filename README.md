# Myoelectric Prosthetic Hand


**BME 261 Final Project** · University of Waterloo · August 2026  

---

## Overview

This project develops a functional myoelectric prosthetic hand for transradial amputees performing demanding tasks. The system converts microvolt-level surface EMG signals into mechanical grip control through three integrated subsystems: a 7-stage analog front-end, non-blocking C++ firmware, and a dog clutch locking mechanism.

Benchmarked against the Cybathlon "Carry Bottles" task — gripping weighted bottles, placing them into a crate, and transporting the load.


---

## System Architecture

```
Surface EMG Electrodes
        │
        ▼
┌─────────────────────────────────────┐
│     7-Stage Analog Front-End        │
│                                     │
│  1. AD623 INA (gain ~101)           │
│  2. Sallen-Key HPF (48 Hz)          │
│  3. Sallen-Key LPF (400 Hz)         │
│  4. Isolation Buffer                │
│  5. Non-Inverting Amp (gain 11)     │
│  6. Peak Envelope Detector          │
│  7. Schmitt Trigger Comparator      │
│                                     │
│  Total gain: ~1,111×                │
└──────────────────┬──────────────────┘
                   │  Digital trigger (0–5V)
                   ▼
        Arduino Microcontroller
        (Non-blocking state machine)
                   │
                   ▼
          Position Servo Motor
                   │
                   ▼
            Dog Clutch
     (locks finger transmission shafts)
                   │
                   ▼
      Passive Spring-Loaded Fingers
```

---

## Electrical Subsystem

The analog front-end processes raw surface EMG signals (10μV–5mV) through a cascaded 7-stage single-supply architecture.

### Signal Chain

| Stage | Component | Function | Key Parameters |
|---|---|---|---|
| 1 | AD623 INA | Differential amplification, CM rejection | Gain ~101, R_G = 1kΩ |
| 2 | Sallen-Key HPF | Remove motion artifacts, electrode drift | f_c ≈ 48 Hz, C = 33nF, R = 1MΩ / 10kΩ |
| 3 | Sallen-Key LPF | Remove high-frequency noise | f_c ≈ 400 Hz, R = 20kΩ, C = 12nF / 33nF |
| 4 | Buffer (follower) | Isolation — prevent impedance loading | Unity gain |
| 5 | Non-inverting amp | Secondary voltage gain | Gain = 11, R_in = 10kΩ, R_f = 100kΩ |
| 6 | Peak detector | Rectify AC bursts to DC envelope | Active diode-capacitor |
| 7 | Schmitt trigger | Clean binary output to Arduino | Hysteretic comparator |

**Total system gain: ~1,111×**

### Virtual Ground

A buffered voltage divider (2× 10kΩ) creates a stable Vcc/2 reference, enabling single-supply operation. The reference automatically scales with battery voltage — no recalibration needed when switching between lab supply and battery.

### Power

- Lab: regulated DC supply (5V)
- Portable: dual 4×AA packs in parallel (~6V, doubled current capacity)

### Frequency Response

Measured passband: 48–400 Hz. Overdamped filter design produces gradual roll-off at corner frequencies.

---

## Firmware

Written in C++ on Arduino. Non-blocking time-domain state machine — no `delay()` calls.

### State Machine

```
         ┌─────────────────┐
         │      IDLE       │
         └────────┬────────┘
                  │ Edge detected on input pin
                  ▼
         ┌─────────────────┐
         │  BURST ACTIVE   │◄──────────────┐
         │  (timing burst) │               │ Still toggling
         └────────┬────────┘               │
                  │ Duration ≥ 500ms        │
                  ▼                        │
         ┌─────────────────┐               │
         │    ACTUATE      │               │
         │  Toggle servo   │               │
         │  Set lock flag  │               │
         └────────┬────────┘               │
                  │ Signal static > 400ms   │
                  ▼                        │
         ┌─────────────────┐               │
         │  RESET LOCKOUT  │───────────────┘
         │  Clear flags    │
         └─────────────────┘
```

### Key Parameters

| Parameter | Value | Purpose |
|---|---|---|
| `activityWindow` | 500 ms | Minimum contraction duration — rejects twitches and noise spikes |
| `quietTimeout` | 400 ms | Rest period required before next trigger |
| Servo open | 135° | Dog clutch disengaged — fingers free |
| Servo closed | 150° | Dog clutch engaged — fingers locked |

### Why these values?

The 500ms window was chosen to eliminate false triggers from static shocks, brief muscle twitches, and electrical noise while remaining responsive enough for intentional grips. The 400ms quiet timeout prevents rapid re-triggering during a single sustained contraction. Servo angles are software-clamped to prevent mechanical binding and the 2.5A stall current observed near 170°.

---

## Mechanical Subsystem

- **Fingers:** Two interconnected phalange sections (base + top) coordinated by sprockets, chains, and gears
- **Actuation:** Passive spring loading — fingers open when pressed against object geometry
- **Locking:** Dog clutch controlled by position servo — mechanically retains grip with zero steady-state motor current
- **Structure:** Laser-cut plywood with box-joint connections, aluminum axles

### Finger Movement

When the hand contacts an object, friction and reaction forces passively open the fingers to conform to the object geometry. The internal spring tension closes the fingers around the object. On muscle activation, the servo engages the dog clutch, locking the transmission shafts and holding the grip position.

---

## Repository Structure

```
myoelectric-prosthetic/
  firmware/
    prosthetic_control.ino     # Arduino C++ state machine
    flowchart.png              # Firmware flowchart
  electrical/
    bodeplots.png             # Bode plots
    schematic.png              # Full system schematic
    falstad_diagram.txt                # Falstad simulation export
  pcb/
    gerbers/                   # Fabrication files
    prosthetic_pcb.kicad_pcb   # KiCad board file
    prosthetic.kicad_sch       # KiCad schematic
    bom.csv                    # Bill of materials
  mechanical/
    presentation.pdf
  docs/
    BME261_Final_Report.pdf    # Full project report
    bill_of_materials.pdf      # List of parts
```

---

## Running the Falstad Simulation

1. Open [Falstad Circuit Simulator](https://www.falstad.com/circuit/)
2. Click **File → Import from Text**
3. Paste the contents of `electrical/falstad/circuit.txt`
4. Press **Run** to simulate the signal conditioning pipeline

---

## Results

| Metric | Result |
|---|---|
| Signal passband | 48–400 Hz |
| Total analog gain | ~1,111× |
| EMG input range | 10μV–5mV |
| Output range | 0–5V (Arduino compatible) |
| Trigger delay | 500ms (intentional) |
| Servo travel | 135°–150° |
| Cybathlon task | Successfully demonstrated |

---

## Limitations & Future Work

- **Materials:** PETG components and superglue connections loosened during testing. Future: engineering-grade heat-resistant materials, pinned dog clutch connection
- **Firmware:** 500ms window reduces responsiveness. Future: continuous ADC sampling with digital moving-average filter, shorter activation window
- **Electrical:** Hand-soldered perfboard susceptible to EMI and stray capacitance. Future: multi-layer SMD PCB, LDO regulator for analog supply, buck-boost for servo
- **Power:** Unregulated battery causes reference drift as charge depletes. Future: dedicated LDO for analog front-end
- **Mechanical:** Smooth plastic palm reduces friction on objects. Future: compliant grip surface material

---

## Built With

- **Electrical:** AD623, TL072/TL082 op-amps, discrete passives
- **Simulation:** [Falstad Circuit Simulator](https://www.falstad.com/circuit/)
- **PCB:** KiCad
- **Mechanical:** Autodesk Fusion 360, laser-cut plywood, PETG 3D printing
- **Firmware:** Arduino (C++)
- **Course:** BME 261 — Biomedical Design, University of Waterloo

---

## Authors

- Owen Kim — Electrical subsystem, firmware, PCB design
- Adithi Nagarajan — Mechanical subsystem
- Hilda Zhang — Mechanical subsystem

---

## License

MIT — free to use, adapt, and build on with attribution.
