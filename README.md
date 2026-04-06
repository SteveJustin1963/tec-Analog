![59768314_2183250095045346_7398950653424304128_n](https://github.com/user-attachments/assets/bf3d6d35-c7c1-4e82-9948-6aa506b54d9f)

# tec-Analog: Z80-Controlled Hybrid Analog Computer

A hybrid analog computer interfaced to the TEC-1 Z80 SBC. Analog computation (op-amp integrators, summers) is patched under software control via a reed relay crossbar driven by the Z80. Based on HDR-75 / PEAC analog computer architecture, with optional memristor integrators.

---

## System Overview

```
  ┌──────────────────────────────────────────────────────────────┐
  │                        ANALOG CORE                           │
  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌───────────┐   │
  │  │ SUMMER   │  │INTEGRATOR│  │ INVERTER │  │   COEFF   │   │
  │  │   (Σ)    │  │   (∫)    │  │  (×-1)   │  │   POTS    │   │
  │  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬──────┘   │
  │       └─────────────┴─────────────┴───────────────┘          │
  │                            │                                  │
  │            PATCH PANEL (Reed Relay Crossbar)                  │
  └──────────────────────────┬───────────────────────────────────┘
                              │
               ┌──────────────┴────────────────┐
               │                               │
      ┌────────┴────────┐           ┌──────────┴──────────┐
      │  ADC (MCP3008)  │           │   DAC (MCP4921)      │
      │  Analog → Z80   │           │   Z80 → Analog       │
      └────────┬────────┘           └──────────┬──────────┘
               └──────────────┬────────────────┘
                              │
                     ┌────────┴─────────┐
                     │   74HC595 CHAIN  │
                     │  (Crossbar Ctrl) │
                     └────────┬─────────┘
                              │
                     ┌────────┴─────────┐
                     │   Z80 / TEC-1    │
                     │  (MINT autopatch)│
                     └──────────────────┘
```

---

## Build Order — Logical Flow

Build and verify each stage before moving to the next. Each stage has a measurable test output.

| Stage | Section | Test Output |
|-------|---------|-------------|
| 1 | Dual ±12V Power Supply | Meter reads ±12V, ±10V ref stable |
| 2 | One Integrator block | Output ramps linearly with DC input |
| 3 | Summer + Inverter blocks | Weighted sum of test voltages correct |
| 4 | Coefficient pots + patch panel | Manual harmonic oscillator on scope |
| 5 | ADC interface (MCP3008) | Z80 reads correct analog voltage |
| 6 | DAC interface (MCP4921) | Z80 sets analog voltage to target |
| 7 | Reed relay crossbar (74HC595) | Z80 connects two blocks via relay |
| 8 | Z80 control code | Full patch-apply routine works |
| 9 | MINT autopatch layer | Patched ODE runs from MINT commands |

---

## Section 1: Dual Power Supply (±12V)

### Why
The analog core runs on ±12V rails with a precision ±10V signal reference. Digital logic runs on +5V. These must be separate supplies to prevent switching noise from the digital side coupling into the analog signals and corrupting computation.

### How

```
  230V AC  L ──────────────────────────────────────┐
           N ───────┐                               │
                    │                               │
              ┌─────┴──────────────────────────┐    │
              │        MAINS TRANSFORMER        │    │
              │  Primary: 240V                  │    │
              │  Secondary: 2×15VAC centre-tap  │    │
              └──┬─────────────────────────┬───┘    │
                 │ +15VAC            -15VAC│         │
                 │                         │         │
  ┌──────────────┴─────────────────────────┴────────┐
  │                  BRIDGE RECTIFIER               │
  │                                                  │
  │   D1 1N4007                    D3 1N4007        │
  │   ─►──────────────┬────────────────►─           │
  │                   │                             │
  │  +raw (+20V DC) ──┴── C1 2200µF/35V             │
  │                   │                             │
  │                  GND  ◄── star ground point      │
  │                   │                             │
  │  -raw (-20V DC) ──┴── C2 2200µF/35V             │
  │                                                  │
  │   D2 1N4007                    D4 1N4007        │
  │   ─►──────────────┴────────────────►─           │
  └──────────────────────────────────────────────────┘
            │                              │
       +raw │                        -raw  │
            │                              │
        LM7812                         LM7912
       ┌────┴────┐                    ┌────┴────┐
  IN ──┤1       3├── +12V        IN ──┤1       3├── -12V
       │  TO220  │                    │  TO220  │
  GND──┤2        │             GND ───┤2        │
       └─────────┘                   └─────────┘
            │                              │
         C3 0.1µF                      C4 0.1µF
            │                              │
           GND                            GND

  ±10V PRECISION REFERENCE (for patch panel signal range):

  +12V ──── R1 2.2kΩ ──┬── R2 2.2kΩ ──── GND
                        │
                       +10V (REF+)
                        │
                      trim pot (optional)

  -12V ──── R3 2.2kΩ ──┬── R4 2.2kΩ ──── GND
                        │
                       -10V (REF-)

  Digital supply:
  +12V ──── LM7805 ──── +5V (for Z80, 74HC595, ICs)
                   └─── C5 100µF, C6 0.1µF to GND
```

**Parts:**
- Transformer: 240V → 2×15VAC, 1A (toroidal preferred for low hum)
- D1–D4: 1N4007 (or W04 bridge, 1A)
- C1, C2: 2200µF 35V electrolytic
- C3, C4: 0.1µF ceramic (regulator stability)
- U1: LM7812, U2: LM7912, U3: LM7805 (all TO-220, heatsink required)
- R1–R4: 2.2kΩ 1% metal film (±10V divider)

### Fault Finding

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| No output voltage | Bridge diode open | Diode test on D1–D4 |
| +12V present but no -12V | LM7912 miswired (pin 1 = IN on negative side) | Verify: LM7912 pin1=IN(-), pin2=ADJ, pin3=OUT(-) |
| Regulator very hot | No heatsink or overcurrent | Add TO-220 heatsink; analog core draws ~100mA |
| Hum on analog signals | Single ground point not enforced | Star-ground: one wire from analog GND to digital GND |
| ±10V reference drifts with temperature | Standard resistors | Replace R1–R4 with 0.1% metal film or add a TL431 ref |
| Regulator oscillates | No output capacitor | 0.1µF ceramic on each regulator output pin |

---

## Section 2: Op-Amp Integrator

### Why
The integrator is the fundamental compute element. It integrates its input voltage over time: `Vout = -(1/RC) × ∫Vin dt`. Each integrator stores one state variable (position, velocity, charge, etc.). The capacitor holds the current state. The 10MΩ feedback resistor prevents slow DC saturation from op-amp offset. Build 4–6 of these.

### How

```
                          C1  0.1µF polyester
              ┌───────────┤├──────────────────────────────┐
              │                                           │
              │           Rf  10MΩ                        │
              ├───────────/\/\/\──────────────────────────┤
              │                                           │
Vin ──R1─────►│                     ┌───────┐            │
    100kΩ 1%  │(-)                  │       │            │
              │         TL071       │  OUT  ├────────────┼──── Vout
              │(+)                  │       │            │
              │                     └───┬───┘            │
              │                        │   │             │
R_bias       GND                      V+   V-            │
100kΩ to GND                         +12V  -12V          │
(matches R1)                                             │
              │                                           │
              └────────────── (back to - input) ──────────┘

  IC / RESET mode control:
                                 K1 (reed relay, NO)
  IC voltage ──R_ic 10kΩ──►(-)  ├─────────────────────── across C1
                                 │  (shorts cap to set IC)
                                 │
  When K1 closes: cap is shorted, IC voltage charges to set w(0)
  When K1 opens:  integration begins from IC value

  Transfer function:
    Vout(t) = -(1/RC) × ∫Vin dt + Vic
    τ = R1 × C1 = 100kΩ × 0.1µF = 10ms (f_unity ≈ 160Hz)
```

**Parts per integrator:**
- U1: TL071 or TL081 (8-pin DIP, low offset)
- R1: 100kΩ 1% metal film (input)
- Rf: 10MΩ 1% (DC stabilising feedback)
- C1: 0.1µF polyester film (low leakage — not ceramic, not electrolytic)
- R_bias: 100kΩ to GND on (+) input (balances bias current = R1 value)
- R_ic: 10kΩ (initial condition input resistor)
- K1: 5V reed relay, NO contacts across C1 (RESET/IC control)
- 100nF ceramic cap between each V+ and V- pin and GND (essential)

### Fault Finding

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| Output immediately hits ±12V rail | No C1 or wired as amplifier not integrator | C1 must be in the feedback, not the input |
| Output slowly drifts to rail | Capacitor leakage or missing Rf | Use polyester/polystyrene cap; verify Rf 10MΩ installed |
| High-frequency oscillation | No decoupling capacitor on supply pins | Add 100nF ceramic directly at TL071 pins 4 and 7 |
| Cannot reset to zero | Relay K1 contact not shorting cap | Measure relay contact resistance: must be <5Ω |
| Gain wrong (output too large/small) | R1 or Rf value wrong | Measure with meter; 1% resistors are strongly recommended |
| Output = -Vin (acts as inverter) | C1 and Rf swapped | In integrator: R1 is input resistor, C1 is feedback element |

---

## Section 3: Summer & Inverter

### Why
Summers (adders) combine multiple signals with selectable weighting — used to add derivatives, forcing functions, and feedback terms. The inverter flips signal sign, which is essential because op-amp stages are inherently inverting. Most analog computer loops require at least one inverter to achieve negative feedback.

### How

**3-input Summer:**
```
  Va ──R1 10kΩ──┐
                │
  Vb ──R2 10kΩ──┼──────►(-)    ┌───────┐
                │               │ TL071 ├──── Vout = -(Va + Vb + Vc)
  Vc ──R3 10kΩ──┘               │       │     (when R1=R2=R3=Rf=10kΩ)
                       GND ────►(+)     │
                                │ Rf 10kΩ in feedback (- to out)
                                └───────┘

  For weighted sum: change input resistor values.
    Vout = -(Rf/R1×Va + Rf/R2×Vb + Rf/R3×Vc)

  Bias resistor on (+): R_bias = R1//R2//R3//Rf  (parallel combination)
  For 3× 10kΩ inputs + 10kΩ feedback: R_bias = 2.5kΩ (use 2.7kΩ)
```

**Unity Gain Inverter:**
```
  Vin ──R1 10kΩ──────►(-)    ┌───────┐
                              │ TL071 ├──── Vout = -Vin
              GND ───────────►(+)     │
                              │ Rf 10kΩ (= R1)
                              └───────┘
```

**Parts per summer (3-input) using TL084 quad op-amp:**
- U1: TL084 (4 op-amps per package — one package = 4 blocks)
- R1, R2, R3: 10kΩ 1% (input resistors)
- Rf: 10kΩ 1% (feedback)
- R_bias: 2.7kΩ to GND on (+) input
- 100nF ceramic on V+ and V- of each package

### Fault Finding

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| Output wrong sign | Input on (+) instead of (-) | Signals must go to inverting (-) input |
| Gain is ×2 instead of ×1 | Rf = 2×R_in | Make Rf = R_in for unity gain sum |
| Outputs sum at wrong ratio | Resistors not matched | Use 1% resistors from same batch |
| Oscillates at MHz | No supply decoupling | 100nF ceramic at each TL084 package supply pin |
| One input has no effect | That input resistor open | Check solder joint on that R |

---

## Section 4: Coefficient Potentiometers & Patch Panel

### Why
Coefficient pots set the constant multipliers in the problem equation (spring constant K, damping C, mass M, etc.). Without them, you can only solve equations with coefficients of ±1. The patch panel provides labelled connection points where block outputs connect to block inputs, forming the equation topology.

### How

**Coefficient potentiometer (manual, ±1 range):**
```
  +10V REF ─────────────┐
                         │
                       ──┤ top
                         │
                    [10kΩ 10-turn pot]
                         │
                    wiper ├──── R_iso 10kΩ ──── to block input
                         │     (isolation — prevents loading)
                       ──┤ bottom
                         │
  -10V REF ─────────────┘

  Wiper at top:    coefficient = +1 (output = +10V × gain of block)
  Wiper at centre: coefficient = 0
  Wiper at bottom: coefficient = -1
```

**Patch panel layout (8-block example, banana jacks):**
```
  ┌──────────────────────────────────────────────────────┐
  │               PATCH PANEL (top view)                  │
  │                                                       │
  │  OUTPUTS (red jacks):                                 │
  │  ○I1  ○I2  ○I3  ○I4  ○S1  ○S2  ○G1  ○G2             │
  │                                                       │
  │  ── patch cord slots ──                               │
  │                                                       │
  │  INPUTS (black jacks):                                │
  │  ○I1  ○I2  ○I3  ○I4  ○S1  ○S2  ○G1  ○G2             │
  │                                                       │
  │  COEFF (yellow jacks):                                │
  │  ○P1  ○P2  ○P3  ○P4  ○P5  ○P6  ○P7  ○P8             │
  │                                                       │
  │  REF (white): +10V  GND  -10V                        │
  └──────────────────────────────────────────────────────┘
  (I=integrator, S=summer, G=gain, P=pot)
  Use 4mm banana jacks, min 15mm spacing.
```

### Fault Finding

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| Coefficient won't reach full ±1 | REF voltage low or pot not reaching end | Measure ±10V at pot ends, check wiper travel |
| Noisy / scratchy coefficient | Dirty pot wiper | Use 10-turn cermet pot; add 10nF cap from wiper to GND |
| Patch point shorts to neighbour | Banana jacks too close | Minimum 15mm centre spacing; use different colours |
| Signal lost through patch cord | Bad banana plug contact | Test continuity of each patch cord |

---

## Section 5: Manual Test — Harmonic Oscillator

### Why
Before adding any digital control, verify the analog core works. Two integrators in a feedback loop produce a sinusoid — the classic analog computer test. This confirms: integrators integrate, the summer subtracts, the inverter works, and the power supply is stable. Observe as a Lissajous figure on an oscilloscope in X-Y mode.

### How

**Block diagram:**
```
  ┌──────────────────────────────────────────────────────┐
  │  Negative feedback loop (natural frequency ω₀)       │
  │                                                       │
  │  ┌──────────┐    ┌──────────┐                        │
  │  │  INT-A   │    │  INT-B   │                        │
  └─►│  ẋ→x    ├───►│  ẍ→ẋ    ├──────────────────────┐  │
     │  (pos)   │    │  (vel)   │                     │  │
     └──────────┘    └──────────┘                     │  │
          │ x                  │ ẋ                    │  │
          │                    │                      │  │
         OUT-X               OUT-Y                    │  │
         (scope CH1)        (scope CH2)               │  │
                                                      │  │
                    ┌──────────┐                      │  │
                    │ INVERTER │◄─────────────────────┘  │
                    │  (×-1)   │   x fed back negated     │
                    └────┬─────┘                         │
                         │ -x                            │
                         └──────────────────────────────►┘
                                (to INT-B input)
```

**Manual wiring (banana jacks):**
```
  INT-B output ─── 100kΩ ─── INT-A input
  INT-A output ─── 100kΩ ─── INVERTER input
  INVERTER out ─── 100kΩ ─── INT-B input

  IC: apply ~1V to INT-A input via pot, leave INT-B at 0V
  Then switch to COMPUTE (open IC relays)

  Expected frequency: f = 1 / (2π × R × C)
  With R = 100kΩ, C = 0.1µF:  f ≈ 16 Hz
```

**Oscilloscope setup:**
```
  CH1 (X-axis): INT-A output  → position
  CH2 (Y-axis): INT-B output  → velocity
  Mode: X-Y → expect circle or ellipse (90° phase)
  Time mode: CH1 and CH2 → expect two sinusoids 90° apart
```

### Fault Finding

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| Output immediately goes to rail | Feedback sign positive (not negative) | Check inverter is in loop; verify (−) input used |
| Oscillation decays to zero | Excess damping — Rf too small | Increase Rf on integrators to 10MΩ |
| Oscillation grows and clips | Gain > 1 in loop | Reduce initial condition or add small loss resistor |
| Frequency much too high/low | R or C value wrong | Measure C1 with LCR meter; 0.1µF polyester, not ceramic |
| Lissajous is a line not circle | Phase not 90° — timing mismatch | Both integrators must have identical R and C values |
| No oscillation at all | IC not applied or relay stuck open | Probe INT-A input: should see IC voltage before run |

---

## Section 6: ADC Interface (MCP3008)

### Why
The Z80 needs to read back analog voltages to monitor state variables, detect completion, log data, and implement closed-loop digital control. The MCP3008 provides 8 channels of 10-bit SPI ADC operating from the +5V digital supply. Analog signals (±10V) must be scaled into the 0–5V input range.

### How

```
  ANALOG CORE (±10V)              MCP3008 (0–5V input range)

  Vanalog (e.g. +7.3V)
       │
       ├── R1 20kΩ ──┬──────────────► CH0 (pin 1)
       │             │
  +5V ─┴── R2 20kΩ ──┘    (voltage divider: ±10V → 0-5V)

  Buffer op-amp on each channel (recommended):
                     │
                  ┌──┴───┐
                  │ BUF  ├──► to CHx  (TL071 unity gain follower)
                  │TL071 │
                  └──────┘

                                         MCP3008
                                       ┌──────────┐
  CH0..CH7 ────────────────────────── ►│CH0..CH7  │
  +5V ────────────────────────────────►│VDD       │
  +5V ────────────────────────────────►│VREF      │
  AGND ───────────────────────────────►│AGND      │
  GND ────────────────────────────────►│DGND      │
  Z80 b1 (CLK) ───────────────────────►│CLK       │
  Z80 b2 (MOSI) ──────────────────────►│DIN       │
  Z80 b3 (MISO) ◄─────────────────────┤DOUT      │
  Z80 b0 (/CS) ───────────────────────►│/CS       │
                                       └──────────┘

  Voltage scaling:
    Vscaled = (Vanalog + 10) × 5/20  =  Vanalog/4 + 2.5V
    Vanalog = (Vscaled − 2.5) × 4
    Example: Vanalog = +7.3V → Vscaled = 4.325V → ADC = 884
```

**SPI read sequence (Z80):**
```
  1. /CS low
  2. Send: start(1), SGL=1, D2,D1,D0 (channel select)
  3. Dummy clock; then read 10-bit result MSB first
  4. /CS high
  5. Vanalog = (ADC_count / 1023.0 × 5.0 − 2.5) × 4.0
```

**Parts:**
- U1: MCP3008 (DIP-16, 8-ch 10-bit SPI ADC)
- R1, R2 per channel: 20kΩ 1% (input divider)
- Op-amp buffer: TL071 per channel (or TL084 quad for 4 channels)
- C1: 100nF + 10µF on VDD and VREF pins

### Fault Finding

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| ADC reads 0 or 1023 always | Input out of 0–5V range | Verify divider; ensure Vanalog ≤ ±10V |
| Noisy/jumping readings | No decoupling on MCP3008 supply | 100nF ceramic + 10µF electrolytic on VDD pin |
| SPI timeout / no response | Timing too fast for bit-bang | Add 2× NOP between each clock edge |
| Wrong channel returned | /CS deasserted mid-frame | /CS must stay low for the full 16-bit frame |
| Reading offset by fixed amount | Divider reference wrong | Measure actual +5V; use it as VREF too |

---

## Section 7: DAC Interface (MCP4921)

### Why
The Z80 must set initial conditions for integrators and adjust coefficient values under program control. The MCP4921 provides 12-bit SPI DAC output (0–5V), which is then scaled by an op-amp stage to ±10V to match the analog core's signal range.

### How

```
  Z80 / TEC-1 (5V logic)

  Z80 b0 (/CS) ───────────────────────►┐
  Z80 b1 (CLK) ───────────────────────►┤  MCP4921
  Z80 b2 (MOSI) ──────────────────────►┤ ┌──────────┐
  GND ────────────────────────────────►┤ │/LDAC(GND)│
  +5V ────────────────────────────────►┤ │VDD  +5V  │
  +5V ────────────────────────────────►┤ │VREF +5V  │
                                        │ │          │
                                        │ │VOUT      ├──► Vdac (0–5V)
                                        └─┤          │
                                          └──────────┘
                                               │
  OUTPUT SCALING STAGE (0–5V → ±10V):         │
                                               │
  Vdac ──── R1 10kΩ ────────────►(-)    ┌───────┐
                                         │ TL071 ├──► Vout (±10V)
  -10V ref ── R2 10kΩ ──────────►(+)   │       │
                                         │Rf 20kΩ│
                                         └───────┘

  Formula: Vout = 2 × Vdac − 10V
  Range:   Vdac=0V → Vout=−10V
           Vdac=2.5V → Vout=0V
           Vdac=5V → Vout=+10V
```

**SPI write sequence (Z80):**
```
  1. /CS low
  2. Send 16-bit word:
     bits 15-12: config  0b0111  (unbuffered, gain×1, active)
     bits 11-0:  12-bit DAC value (0–4095)
  3. /CS high  (auto-latches, /LDAC tied to GND)
  4. Vdac = (DAC_count / 4095.0) × Vref
```

**Parts:**
- U1: MCP4921 (DIP-8, 12-bit SPI DAC)
- U2: TL071 (output scaling op-amp)
- R1: 10kΩ 1%, R2: 10kΩ 1%, Rf: 20kΩ 1%
- C1: 100nF ceramic on VDD and VREF

### Fault Finding

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| DAC output stuck at 0V | /LDAC not tied to GND | Connect /LDAC pin directly to GND |
| Output range wrong | Rf/R ratio incorrect | Verify R1=R2=10kΩ, Rf=20kΩ; check −10V reference |
| Glitches on output during SPI | /CS released before clock idle | Wait for SCK low before raising /CS |
| Full-scale error | VREF not exactly 5V | Measure actual +5V and use in calculation |
| Scaling op-amp saturates | −10V reference missing | Check −10V ref supply to R2 |

---

## Section 8: Reed Relay Crossbar (74HC595 + ULN2803)

### Why
This is the software-defined patch panel — the key innovation. Instead of manually inserting banana jack cables, the Z80 opens and closes reed relays to connect block outputs to block inputs. Each relay replaces one patch cord. The 74HC595 shift register chain lets 32+ relays be controlled with only 3 Z80 I/O bits (DATA, CLK, LATCH). Reed relays are used because they have very low contact resistance, minimal leakage, and low coil current (~20mA at 5V).

### How

**Single relay cell:**
```
  74HC595          ULN2803 (open collector)      Reed Relay SIP-1A05
  output Q_n ─────►IN_n                         ┌────────────────┐
                                                 │                │
                   OUT_n ─── 82Ω ──────────────►│ COIL (+)       │
                                                 │                │
                              GND ─────────────►│ COIL (-)       │
                                                 │                │
  flyback diode:                                 │ NO   C         │
  +5V ──── D1 1N4148 ────────────────────────── │ (contact)      │
  (cathode to +5V, anode to coil +)             └───┬─────┬──────┘
                                                     │     │
                                      Block A OUT ───┘     └─── Block B IN
                                      (when relay closes, A connects to B)
```

**32-relay crossbar chain (4× 74HC595):**
```
  Z80 I/O port (3 bits):

  b0 DATA  ──────────────────────────────────────────────────────────┐
  b1 CLK   ──────┬───────────┬───────────┬───────────┐              │
  b2 LATCH ──────┼───────────┼───────────┼───────────┼──            │
                 │           │           │           │              │
           ┌─────┴───┐ ┌─────┴───┐ ┌─────┴───┐ ┌─────┴───┐         │
           │74HC595 1│ │74HC595 2│ │74HC595 3│ │74HC595 4│         │
           │         │ │         │ │         │ │         │         │
  DS ◄─────┤14   Q7' ├►┤14   Q7' ├►┤14   Q7' ├►┤14   Q7' │◄────────┘
  (from Z80)│         │ │         │ │         │ │         │
           │Q0..Q7   │ │Q0..Q7   │ │Q0..Q7   │ │Q0..Q7   │
           └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘
                │           │           │           │
           ULN2803      ULN2803      ULN2803      ULN2803
           (IC5)        (IC6)        (IC7)        (IC8)
                │           │           │           │
          [K1..K8]    [K9..K16]   [K17..K24]  [K25..K32]
          Reed relays forming analog crossbar matrix
```

**To close relay N:** set bit N in the 32-bit shift register, shift out MSB first, latch.

**Parts for 32-relay crossbar:**
- 4× 74HC595 shift register (Jaycar ZC4895, ~$2 each)
- 4× ULN2803 (8-channel Darlington driver array, ~$2 each)
- 32× 5V reed relay SIP-1A05 (AliExpress, 50 for ~$20)
- 32× 1N4148 flyback diode (cathode to +5V, across each coil)
- 4× 100nF ceramic decoupling (one per 74HC595 VCC pin)
- 82Ω series resistor per relay coil (protects ULN2803)

### Fault Finding

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| No relays activate at all | 74HC595 not clocking | Scope DATA and CLK lines during SEND_BYTE |
| Only first 8 relays work | Daisy chain broken | Check Q7' (pin 9) of IC1 → DS (pin 14) of IC2 |
| Relay chatters / buzzes | Missing flyback diode | 1N4148 across each coil, cathode to +5V rail |
| Analog signal corrupted through relay | Ground loop or contact bounce | Verify single star ground; reed relays have low leakage |
| Wrong relay activates | Bit order reversed | First shifted byte appears at last 595 outputs; adjust mapping |
| Relay stays on after commanded off | Latch not clocking | Verify LATCH pulse width ≥ 20ns (add NOP) |

---

## Section 9: Z80 Control Code

### Why
The Z80 is the digital controller: it shifts out the patch matrix, sets initial conditions via the DAC, reads results via the ADC, controls the analog machine mode (IC/HOLD/RUN), and hosts the MINT autopatch layer. All hardware is controlled through simple I/O port bit-banging.

### How

**Port map (adjust to your TEC-1 PIO address):**
```
  PORT 00h:  Shift register  (b0=DATA, b1=CLK, b2=LATCH)
  PORT 01h:  Mode control    (b0=RESET, b1=IC, b2=HOLD, b3=RUN)
  PORT 02h:  ADC SPI MCP3008 (b0=/CS, b1=CLK, b2=MOSI, b3=MISO)
  PORT 03h:  DAC SPI MCP4921 (b0=/CS, b1=CLK, b2=MOSI)
```

**74HC595 bit-bang driver:**
```asm
; ── Constants ──────────────────────────────────────────────────
DATA_BIT   EQU  01h        ; bit mask for DATA line
CLK_BIT    EQU  02h        ; bit mask for CLK line
LATCH_BIT  EQU  04h        ; bit mask for LATCH line
SR_PORT    EQU  00h        ; shift register I/O port

; ── SEND_BYTE ─────────────────────────────────────────────────
; Send byte in A to shift register chain (MSB first)
; Destroys: A, B, C
SEND_BYTE:
    ld   b, 8              ; 8 bits to send
    ld   c, a              ; preserve byte in C
SB_LOOP:
    ld   a, c
    and  80h               ; test MSB
    jr   z, SB_ZERO
    ld   a, DATA_BIT       ; data = 1
    out  (SR_PORT), a
    jr   SB_CLK
SB_ZERO:
    xor  a                 ; data = 0
    out  (SR_PORT), a
SB_CLK:
    ld   a, CLK_BIT        ; clock high
    out  (SR_PORT), a
    nop                    ; hold time
    xor  a                 ; clock low
    out  (SR_PORT), a
    rlc  c                 ; rotate to next bit
    djnz SB_LOOP
    ret

; ── LATCH_SR ──────────────────────────────────────────────────
; Latch all 595 outputs simultaneously
LATCH_SR:
    ld   a, LATCH_BIT
    out  (SR_PORT), a
    nop
    xor  a
    out  (SR_PORT), a
    ret

; ── PATCH_APPLY ───────────────────────────────────────────────
; Apply 4-byte (32-bit) patch matrix to relay crossbar
; HL = pointer to 4-byte relay bitmap array
PATCH_APPLY:
    ld   b, 4              ; 4 bytes = 32 relays
PA_LOOP:
    ld   a, (hl)
    call SEND_BYTE
    inc  hl
    djnz PA_LOOP
    call LATCH_SR
    ret
```

**Analog mode control:**
```asm
MODE_PORT  EQU  01h

RESET_ANALOG:   ld a, 01h  :  out (MODE_PORT), a  :  ret
SET_IC:         ld a, 02h  :  out (MODE_PORT), a  :  ret
SET_HOLD:       ld a, 04h  :  out (MODE_PORT), a  :  ret
SET_RUN:        ld a, 08h  :  out (MODE_PORT), a  :  ret
```

**Example: patch harmonic oscillator, run for 100ms, read result:**
```asm
    ld   hl, PATCH_DATA    ; relay bitmap: relays 0 and 1 closed
    call PATCH_APPLY
    call SET_IC            ; IC mode — write IC via DAC here
    ; ... write IC voltage via DAC SPI ...
    call SET_RUN           ; start computation
    ; ... wait 100ms via timer loop ...
    ; ... read result via ADC SPI ...
    ret

PATCH_DATA:
    DB  03h, 00h, 00h, 00h  ; relays 0 (bit0) and 1 (bit1) closed
```

### Fault Finding

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| Shift register not responding | Timing too fast | Add `NOP NOP` between each CLK toggle |
| Byte order wrong in patch | First shifted byte → last 595 | Reverse byte order in PATCH_DATA array |
| Mode lines not switching | Wrong port address | Verify TEC-1 PIO address with `OUT (port), A` test |
| ADC SPI fails | /CS deasserted early | Hold /CS low for full 16-bit transaction |
| DAC not latching | /LDAC not tied to GND | Connect /LDAC pin to GND on MCP4921 |

---

## Section 10: MINT Autopatch Layer

### Why
MINT is the high-level control language for the system. It implements the autopatch compiler and controller — the role historically played by APACHE, APSE, and Micropatch systems. MINT is **not** the patching language itself; it is the **host that implements** the autopatch compiler. Problem topology is expressed as data (block registry + wire table), not syntax. MINT compiles this to crossbar switch commands and hardware mode sequences.

### How

**Global state:**
```mint
[ ] b !     \ block registry  (array of [type id param] records)
[ ] p !     \ patch table     (array of [src dst] records)
0   m !     \ machine mode    (0=RESET 1=IC 2=HOLD 3=RUN)
```

**Block type codes:**
```
  1 = RC Integrator         (capacitor state)
  2 = Memristor Integrator  (resistive state, non-volatile)
  3 = Summer                (weighted addition)
  4 = Gain / Scale          (coefficient amplifier)
  5 = Nonlinear Function    (diode, limiter, multiplier)
  9 = I/O                   (input signal or scope output)
```

**Core command definitions:**
```mint
:RESET  0 m !  CB-RESET ;
:IC     1 m !  1 CB-MODE ;
:HOLD   2 m !  2 CB-MODE ;
:RUN    3 m !  3 CB-MODE ;

:BLOCK+                      \ ( type id param -- )
  [ " " " ] b ?! ;           \ store 3-field record in block table

:WIRE                        \ ( src dst -- )
  [ " " ] p ?! ;             \ store 2-field record in patch table

:PATCH-CHECK                 \ validate before applying
  p /S (
    p /i ?  " 0 ?  " 1 ?    \ for each wire: get src, dst
    \ verify both IDs exist in b
    \ verify no output shorted to output
  ) ;

:PATCH-APPLY                 \ compile and assert full patch
  PATCH-CHECK
  p /S (
    p /i ?  " 0 ?  " 1 ?    \ src, dst
    CB-CONNECT               \ close relay for this connection
  ) ;

:IC-RC   CB-IC-RC  ;         \ ( value id -- ) RC integrator IC
:IC-MEM  CB-IC-MEM ;         \ ( state id -- ) memristor IC
```

**Hardware abstraction (implement for your crossbar):**
```
  CB-CONNECT  ( src dst -- )   close relay connecting src to dst
  CB-DISCONN  ( src dst -- )   open relay
  CB-RESET    ( -- )           open all relays
  CB-MODE     ( mode -- )      assert mode lines to analog hardware
```

**Complete example — solve ẋ = -0.5x + 3u:**
```mint
RESET
[ ] b !  [ ] p !

2 501 0.02  BLOCK+      \ memristor integrator X, id=501
4 301 -0.5  BLOCK+      \ gain -0.5, id=301
4 302  3.0  BLOCK+      \ gain +3.0, id=302
3 201  0    BLOCK+      \ summer, id=201
9 100  0    BLOCK+      \ input U, id=100
9 901  0    BLOCK+      \ output Y, id=901

100 302 WIRE            \ U → gain 3.0
501 301 WIRE            \ X → gain -0.5
301 201 WIRE            \ gain → summer
302 201 WIRE            \ gain → summer
201 501 WIRE            \ summer → integrator (feedback)
501 901 WIRE            \ X → output

PATCH-APPLY
0.1 501 IC-MEM
RUN
```

**Autopatch system lineage:**

| System | Era | Role |
|--------|-----|------|
| APACHE | 1950s–60s | Autopatch compiler + wiring verifier |
| APSE | 1960s | Equation scaling + configuration |
| CSMP | 1960s–70s | Continuous system modeling language |
| Micropatch | 1970s | Digitally-controlled crossbar, patch recall |
| **MINT-AP** | Modern | Stack-based autopatch controller, hybrid elements |

### Fault Finding

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| PATCH-APPLY does nothing | CB-CONNECT not implemented | Implement hardware abstraction with Z80 machine code |
| Wrong blocks connect | Relay number vs block ID mismatch | Create and verify a relay-to-block routing table |
| IC not reaching integrator | IC mode not held long enough | Hold IC mode for ≥1ms before switching to RUN |
| MINT stack underflow | Missing operand before command | Test each command interactively with explicit values first |
| PATCH-CHECK passes but circuit wrong | Fan-in violation not detected | Add fan-in check: each input jack must have only one driver |

---

## Appendix A: Analog Computer Theory

### Building Blocks (Mathematical)

| Operation | Circuit | Transfer Function |
|-----------|---------|------------------|
| Integration | Op-amp + R_in + C_f | Vout = −(1/RC)∫Vin dt |
| Summation | Op-amp + R_in + R_f | Vout = −(Rf/R1·V1 + Rf/R2·V2 + ...) |
| Scaling | Op-amp + R divider | Vout = −k·Vin |
| Inversion | Op-amp unity gain | Vout = −Vin |

### Solving an ODE (Procedure)

1. Write the ODE; isolate the highest derivative on the left side
2. Draw a block diagram: highest derivative → integrators → lower derivatives → summer (feedback)
3. Scale: amplitudes must stay within ±10V; time scale chosen so solution is visible on scope
4. Wire the patch panel (or write MINT WIRE commands)
5. Set initial conditions (IC mode)
6. Run and observe on oscilloscope

**Mass-spring-damper example:**
```
  M·ẍ + C·ẋ + K·x = f(t)
  ẍ = (1/M)·f(t) − (C/M)·ẋ − (K/M)·x

  Block diagram:
  ẍ → INT-1 → ẋ → INT-2 → x
  x   → GAIN(−K/M) ─┐
  ẋ   → GAIN(−C/M) ─┼──► SUM ──► ẍ  (feedback)
  f   → GAIN(1/M)  ─┘
```

### Memristor Integrator (Optional Upgrade)

Replace the capacitor with a current-driven memristor. Internal state w(t) takes the role of capacitor charge.

```
  State evolution:  dw/dt = µv × (Ron/D) × I(t)
  Resistance:       M(w)  = Ron×(w/D) + Roff×(1 − w/D)
  Output mapping:   Vout  = ks × w(t) + Voffset
```

| Feature | RC Integrator | Memristor Integrator |
|---------|--------------|---------------------|
| State storage | Capacitor charge | Physical w(t) |
| Power off | State lost | State retained |
| Linearity | High | Moderate |
| Drift | Capacitor leakage | Device hysteresis |
| Best use | High accuracy | Memory effects, history |

For prototyping: emulate with a digital potentiometer + op-amp before using real memristors.

### HDR-75 Analog Computer Reference

The HDR-75 H.H. Borchers is the hardware reference for this project. A patch-panel analog computer used for ODE solving, flight simulation, and physical system modeling.

```
  OPERATOR CONSOLE (mode, time scale, run/hold/reset)
          │
  POWER & REFERENCE (±HV, ±10V precision ref)
          │
  INITIAL CONDITION UNITS (IC pots, ±10V preload)
          │
  PATCH PANEL (equation topology via patch cords)
          │
  ┌───────┴──────────┐
  │                  │
  SUMMING AMPS    SCALING AMPS
  │                  │
  └───────┬──────────┘
          │
  INTEGRATOR BANK (state variables)
          │
  NONLINEAR FUNCTIONS (multipliers, diodes)
          │
  OUTPUT (panel meters, X-Y recorder, scope)
```

---

## Appendix B: MINT Command Reference

| Command | Stack Effect | Description |
|---------|-------------|-------------|
| `RESET` | `--` | Reset all analog state and crossbar |
| `IC` | `--` | Enter initial condition mode |
| `HOLD` | `--` | Freeze state evolution |
| `RUN` | `--` | Begin analog computation |
| `BLOCK+` | `type id param --` | Register an analog block |
| `WIRE` | `src dst --` | Declare a patch connection |
| `PATCH-CHECK` | `--` | Validate patch before applying |
| `PATCH-APPLY` | `--` | Compile and apply patch to hardware |
| `IC-RC` | `value id --` | Set RC integrator initial condition |
| `IC-MEM` | `state id --` | Set memristor integrator initial state w(0) |
| `CB-CONNECT` | `src dst --` | Close crossbar relay (hardware) |
| `CB-DISCONN` | `src dst --` | Open crossbar relay (hardware) |
| `CB-RESET` | `--` | Open all crossbar relays |
| `CB-MODE` | `mode --` | Assert mode lines to analog hardware |

Mode values: `0=RESET  1=IC  2=HOLD  3=RUN`

Block type codes: `1=RC_INT  2=MEM_INT  3=SUM  4=GAIN  5=FUNC  9=IO`

---

## Parts Summary (Australia-Friendly)

| Item | Qty | Source | Approx AUD |
|------|-----|--------|-----------|
| TL071 / TL081 op-amp DIP-8 | 12 | Jaycar ZL3071 | $20 |
| TL084 quad op-amp DIP-14 | 4 | Jaycar ZL3084 | $12 |
| 74HC595 shift register | 4 | Jaycar ZC4895 | $8 |
| ULN2803 Darlington driver | 4 | Jaycar | $8 |
| MCP3008 8-ch SPI ADC | 1 | Jaycar ZK8868 | $5 |
| MCP4921 12-bit SPI DAC | 1 | AliExpress | $5 |
| LM7812 + LM7912 + LM7805 | 1 set | Jaycar | $8 |
| 5V reed relay SIP-1A05 | 32 | AliExpress (50pk) | $20 |
| 0.1µF polyester film caps | 20 | Jaycar | $8 |
| 100kΩ 1% metal film resistors | 50 | Jaycar / AliExpress | $5 |
| 10-turn 10kΩ cermet pots | 8 | AliExpress | $10 |
| 4mm banana jacks (red/black) | 40 | AliExpress | $10 |
| Protoboard / stripboard | 4 | Jaycar | $15 |
| **Total (approx)** | | | **~$130–150** |

---

## References

- https://hackaday.io/project/191238-jumperless
- https://hackaday.com/2023/06/12/a-modular-analogue-computer/
- https://hackaday.io/project/191334-flexible-analog-computer
- https://rclab.de/en/analogrechner/rg14-mini
- https://the-analog-thing.org/
- https://analogparadigm.com/
- http://www.analogmuseum.org/library/PEAC_small.pdf
- https://github.com/bernd-ulmann/DEQscaler
- https://github.com/SteveJustin1963/tec-Analog
- https://github.com/SteveJustin1963/tec-Analog-CPLD-THAT
- https://github.com/SteveJustin1963/tec-Analog-PEAC/tree/master
- https://github.com/SteveJustin1963/tec-Stochastic
- https://github.com/SteveJustin1963/linear-regression
