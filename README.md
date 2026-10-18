# SCVPI-TLM: Unified SystemC TLM-2.0 & SystemVerilog RTL Co-Verification Framework

SCVPI-TLM is a co-verification framework that lets Python verification
environments written with **cocotb** and **PyUVM** run unchanged against:

- SystemVerilog RTL simulated by a conventional HDL simulator,
- signal-level SystemC RTL, and
- transaction-level SystemC models using **TLM-2.0 blocking transport**.

The framework provides a VPI-compatible shim around the SystemC kernel and a
small byte-oriented TLM bridge. The result is one register-model testbench,
one scoreboard, and one regression command set covering three abstraction
levels of the same design.

---

## Table of contents

- [Why SCVPI-TLM](#why-scvpi-tlm)
- [Architecture](#architecture)
- [Verification targets](#verification-targets)
- [Benchmark](#benchmark)
- [Features](#features)
- [Repository layout](#repository-layout)
- [Setup on Ubuntu / WSL2](#setup-on-ubuntu--wsl2)
- [Build the C++ bridge](#build-the-c-bridge)
- [Quick start](#quick-start)
- [TLM-2.0 timing modes](#tlm-20-timing-modes)
- [Continuous integration](#continuous-integration)
- [Troubleshooting](#troubleshooting)
- [License](#license)

---

## Why SCVPI-TLM

cocotb and PyUVM depend on simulator interfaces such as the Verilog
Procedural Interface (VPI). A plain SystemC executable does not expose VPI, so
the same Python testbench cannot normally drive an RTL model and a TLM virtual
prototype.

SCVPI-TLM closes that gap:

1. **VPI compatibility.** SCVPI implements the VPI entry points that cocotb
   needs (`vpi_handle_by_name`, `vpi_iterate`, `vpi_get_value`,
   `vpi_put_value`, `vpi_register_cb`, and friends) on top of the SystemC object
   registry and `sc_signal<T>` values.
2. **Coroutine-safe scheduling.** VPI callbacks are mapped onto SystemC
   processes so cocotb coroutines make progress at the correct simulation
   time.
3. **Transaction access.** A TLM-2.0 `b_transport` target socket is registered
   under a stable endpoint name and exposed to Python through a CPython
   extension module.
4. **Environment reuse.** The PyUVM register model, adapter, sequence library,
   monitor, and scoreboard are shared by every target.

---

## Architecture

```text
┌──────────────────────────── Python verification environment ────────────────────────────┐
│ PyUVM RAL · sequences · adapter/driver · monitor · scoreboard · cocotb tests             │
└───────────────┬───────────────────────────────────────────────────────────┬──────────────┘
                │ cocotb GPI / VPI                                           │ scvpi_tlm_ext
                ▼                                                           ▼
┌───────────────────────────── SCVPI VPI shim ─────────────────┐   ┌───────────────────────┐
│ handle registry · hierarchy walk · sc_signal access           │   │ byte-oriented TLM API │
│ value-change callbacks · write synchronization                │   │ endpoint registry     │
└───────────────────────────────┬───────────────────────────────┘   └───────────┬───────────┘
                                │ VPI                                           │ b_transport
                                ▼                                               ▼
┌──────────────────────────────────── SystemC kernel ──────────────────────────────────────┐
│  SC_MODULE RTL (sc_signal ports)                SC_MODULE TLM target (simple_target_socket)│
└───────────────────────────────────────────────────────────────────────────────────────────┘
```

The `examples/tinyalu_reg` testbench instantiates the same PyUVM environment for
all three model flavors. Only the bus-facing connector changes:

| Model | Python bus access | DUT boundary |
| --- | --- | --- |
| `hdl` | cocotb signal handles through Icarus VPI | SystemVerilog ports |
| `rtl` | cocotb signal handles through the SCVPI VPI shim | `sc_signal` ports |
| `tlm` | `scvpi_tlm_ext.transport()` → `b_transport()` | TLM-2.0 target socket |

---

## Verification targets

All commands are run from `examples/tinyalu_reg`:

| Target | DUT | Description |
| --- | --- | --- |
| `make run-hdl` | SystemVerilog RTL | Compiles `tinyalu.sv` + `TinyALUreg.sv` with Icarus Verilog and drives it through cocotb's VPI. |
| `make run-sc-rtl` | SystemC signal-level RTL | Compiles the SystemC RTL model and exposes its `sc_signal` ports through SCVPI. |
| `make run-sc-tlm` | SystemC TLM-2.0 | Calls the target's `b_transport()` and consumes the annotated delay on the Python side. |
| `make run-sc-tlm-target-wait` | SystemC TLM-2.0 | Rebuilds the target with `TLM_TARGET_CONSUMES_TIME=1`; `b_transport()` consumes latency with `sc_core::wait()`. |

Each target runs the same five PyUVM tests: `AluTest`, `FibonacciTest`,
`AluTestErrors`, `LongRandomTest`, and `ZeroCornerTest`.

---

## Benchmark

The table below reports the median of three warm-build runs of the complete
TinyALU register-model regression (five PyUVM tests, 127 ALU operations,
`RANDOM_SEED=12345`). Each measurement includes simulator startup, cocotb/PyUVM
initialization, and the full test run, but excludes compilation.

**Reference environment:** WSL2 Ubuntu 24.04, 16 vCPU, 8 GB RAM, gcc 13.3
(`-O3`), SystemC 3.0.2, Python 3.12, cocotb 2.1, PyUVM, Icarus Verilog 12.0.

| Target | Abstraction | Transport / kernel | Wall time | Relative speed |
| --- | --- | --- | ---: | ---: |
| `run-hdl` | SystemVerilog RTL | Icarus Verilog + VPI | 2.33 s | 1.00× |
| `run-sc-rtl` | SystemC signal-level RTL | SystemC kernel + SCVPI VPI | 3.17 s | 0.74× |
| `run-sc-tlm` | SystemC TLM-2.0 (LT) | `b_transport` + annotated delay | 1.78 s | 1.31× |
| `run-sc-tlm-target-wait` | SystemC TLM-2.0 (LT) | `b_transport` + target-side `wait()` | 1.65 s | 1.41× |

Observations:

- At this scale the run is startup-dominated. TLM is still the fastest path
  because it avoids per-cycle signal traffic.
- Signal-level SystemC is slightly slower than Icarus here: every register
  access crosses the SCVPI VPI shim plus the SystemC write-synchronization
  driver. Larger designs generally close that gap as RTL event activity grows.
- Annotated delay and target-side `wait()` are within run-to-run noise for this
  workload. Target-side waiting remains the more faithful model for peripherals
  that own their latency.
- Re-run the commands below on your own host; absolute numbers vary with CPU,
  WSL memory policy, and simulator version.

Reproduce with:

```bash
cd examples/tinyalu_reg
make clean
make run-hdl RANDOM_SEED=12345
make run-sc-rtl RANDOM_SEED=12345
make run-sc-tlm RANDOM_SEED=12345
make run-sc-tlm-target-wait RANDOM_SEED=12345
```

---

## Features

- VPI-compatible C API implemented directly on the SystemC kernel.
- Hierarchical object discovery for `sc_module` and `sc_signal<T>`.
- Direct value access for the signal types used by the examples.
- cocotb coroutine scheduling through VPI callbacks.
- `cbValueChange`, `cbAfterDelay`, `cbReadWriteSynch`, and simulation
  lifecycle callbacks.
- TLM-2.0 blocking transport bridge with byte enables and response status.
- Two timing styles: annotated delay and target-side `wait()`.
- Shared PyUVM register model, adapter, sequences, monitor, and scoreboard.
- CI regression covering all four targets.

### Supported signal types

- `sc_signal<bool>`
- `sc_signal<int>`
- `sc_signal<sc_int<4>>`
- `sc_signal<sc_uint<3>>`, `<4>`, `<5>`, `<8>`, `<9>`, `<16>`, `<32>`

Additional types can be added in `classify()`, `vpi_get_value()`, and
`vpi_put_value()` in `src/vpi.icc`.

---

## Repository layout

```text
.
├── CMakeLists.txt                  # bridge library + Python extension build
├── pyproject.toml                  # uv project and Python dependencies
├── src/
│   ├── scvpi.h / scvpi.cpp         # SCVPI runtime and C transport API
│   ├── vpi.icc                     # VPI implementation on the SystemC kernel
│   ├── scvpi_tlm_bridge.*          # endpoint registry and byte transport
│   ├── scvpi_tlm_ext.cpp           # CPython extension (scvpi_tlm_ext)
│   └── ScVpi*.{h,icc}              # handle, iterator, callback types
├── examples/
│   ├── basic/                      # minimal scheduler smoke test
│   ├── adder/                      # deterministic + randomized cocotb test
│   ├── TinyALU_SystemC/            # PyUVM TinyALU on a SystemC RTL model
│   └── tinyalu_reg/                # unified RAL testbench (HDL/RTL/TLM)
├── tools/check_cocotb_results.py   # cocotb XML result gate
├── .github/workflows/ci.yaml       # Linux regression pipeline
├── CLAUDE.md                       # AI-agent build and run instructions
└── SKILLS.md                       # reusable TLM-2.0 RAL verification skill
```

---

## Setup on Ubuntu / WSL2

The instructions below target Ubuntu 24.04, native or under WSL2.

### 1. System packages

```bash
sudo apt-get update
sudo apt-get install -y --no-install-recommends \
  build-essential \
  cmake \
  ninja-build \
  git \
  wget \
  iverilog \
  python3.12 \
  python3.12-venv
```

Verify Icarus Verilog:

```bash
iverilog -V
vvp -V
```

### 2. SystemC 3.0.2

Build SystemC once and install it to a stable prefix:

```bash
export SYSTEMC=/opt/systemc-3.0.2

wget https://github.com/accellera-official/systemc/archive/refs/tags/3.0.2.tar.gz
tar -xzf 3.0.2.tar.gz

cmake -S systemc-3.0.2 -B systemc-build \
  -DCMAKE_INSTALL_PREFIX="$SYSTEMC" \
  -DCMAKE_BUILD_TYPE=Release

cmake --build systemc-build -j"$(nproc)"
sudo cmake --install systemc-build
```

Add the runtime library to the loader path:

```bash
export LD_LIBRARY_PATH="$SYSTEMC/lib:$SYSTEMC/lib64:$LD_LIBRARY_PATH"
```

### 3. Python 3.12+ and uv

```bash
curl -LsSf https://astral.sh/uv/install.sh | sh
```

From the repository root:

```bash
uv sync
source .venv/bin/activate
```

`uv sync` installs cocotb 2.x, PyUVM, pytest, and the test tooling declared in
`pyproject.toml`.

### 4. Environment variables

```bash
export SYSTEMC=/opt/systemc-3.0.2
export SYSTEMC_HOME="$SYSTEMC"
export LD_LIBRARY_PATH="$SYSTEMC/lib:$SYSTEMC/lib64:$LD_LIBRARY_PATH"
```

The example Makefiles use `SYSTEMC` (default `/usr`). Set it if SystemC is
installed elsewhere.

---

## Build the C++ bridge

The root CMake project builds:

- `libscvpi.so` — the VPI shim and TLM endpoint registry, and
- `scvpi_tlm_ext*.so` — the CPython extension imported by the TLM testbench.

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$SYSTEMC" \
  -DSystemC_ROOT="$SYSTEMC"

cmake --build build -j"$(nproc)"
```

Artifacts:

```text
build/libscvpi.so
build/scvpi_tlm_ext*.so
```

The example Makefiles intentionally compile the SCVPI sources directly into
the simulator executable. This keeps each example self-contained and avoids a
runtime `LD_LIBRARY_PATH` dependency on the bridge. The CMake build is useful
for IDE integration, packaging, and standalone bridge development.

---

## Quick start

```bash
git clone <your-fork-url> sc-tlm-verify
cd sc-tlm-verify
uv sync
source .venv/bin/activate
```

Run the small examples:

```bash
make -C examples/basic run RANDOM_SEED=12345
make -C examples/adder run RANDOM_SEED=12345
make -C examples/TinyALU_SystemC run RANDOM_SEED=12345
```

Run the unified register-model regression:

```bash
cd examples/tinyalu_reg

make run-hdl                 RANDOM_SEED=12345
make run-sc-rtl              RANDOM_SEED=12345
make run-sc-tlm              RANDOM_SEED=12345
make run-sc-tlm-target-wait  RANDOM_SEED=12345
```

Run one test only:

```bash
make run-sc-tlm TESTCASE=AluTest RANDOM_SEED=12345
```

Enable transaction logs or waveforms:

```bash
make run-sc-tlm SCVPI_LOG=1 RANDOM_SEED=12345
make trace MODEL=tlm RANDOM_SEED=12345
```

---

## TLM-2.0 timing modes

The target exposes a `tlm_utils::simple_target_socket` and registers
`b_transport`. The port name is published to Python with
`scvpi_register_tlm_endpoint("top.dut.tsock", ...)`.

### Annotated delay (default)

```cpp
delay += t_bus;
// Python: await Timer(delay_ps, unit="ps")
```

The target computes but does not consume time. The Python connector advances
the simulation clock by the returned delay after `b_transport` returns. This is
the most portable style for loosely-timed models.

### Target-side wait (`TLM_TARGET_CONSUMES_TIME=1`)

```cpp
if (delay != SC_ZERO_TIME) {
  wait(delay);
  delay = SC_ZERO_TIME;
}
wait(t_bus);
```

The target owns its latency. The Python connector must not consume `delay_ps`
again. Because the first cocotb callback can arrive before a SystemC process
context is active, `examples/tinyalu_reg/tests/scvpi_tlm.py` performs a one-time
`Timer(1, "step")` scheduler handoff before the first transaction in this mode.

---

## Continuous integration

`.github/workflows/ci.yaml` installs SystemC 3.0.2, Icarus Verilog, and uv on
Ubuntu, then runs:

1. the three standalone examples (`basic`, `adder`, `TinyALU_SystemC`),
2. `run-hdl`,
3. `run-sc-rtl`,
4. `run-sc-tlm` with `TLM_TARGET_CONSUMES_TIME=0`,
5. `run-sc-tlm-target-wait` with a clean build and `TLM_TARGET_CONSUMES_TIME=1`.

Every run is gated by `tools/check_cocotb_results.py`, which rejects missing,
failed, or errored cocotb test cases.

---

## Troubleshooting

**`libsystemc.so: cannot open shared object file`**

Add the SystemC library directory to `LD_LIBRARY_PATH`. The Makefiles already
do this for their child processes; standalone executables need it in the
calling shell.

**`cocotb-config` not found**

Activate the uv environment first (`source .venv/bin/activate`) or let the
Makefile fall back to `uv run cocotb-config`.

**`No module named scvpi_tlm_ext`**

The TLM build produces the extension next to `TINYALU_REG_SC` and passes it via
`GPI_EXTRA`. Run through `make run-sc-tlm`; do not launch the executable
directly without that environment.

**Target-side wait fails before the first transaction**

Confirm `TLM_TARGET_CONSUMES_TIME=1` was set both at CMake configure time and at
runtime. Re-run `make clean` after changing the option; it is baked into the
model compile definitions.

**No tests were collected**

Check `COCOTB_TEST_MODULES` and `PYTHONPATH`. The unified example uses
`tests.testbench` from `examples/tinyalu_reg`.

---

## License

Released under the Apache License 2.0. See [LICENSE](LICENSE).
