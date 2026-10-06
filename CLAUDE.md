# CLAUDE.md — agent guide for SCVPI-TLM

This file is the primary context document for AI coding agents (Claude Code,
Cursor, OpenCode, and similar tools) working in this repository.

**Project:** SCVPI-TLM: Unified SystemC TLM-2.0 & SystemVerilog RTL
Co-Verification Framework

**Purpose:** expose SystemC models to cocotb/PyUVM through a VPI-compatible
shim, and expose TLM-2.0 `b_transport` targets through a small Python bridge,
so one register-model testbench can verify HDL RTL, SystemC RTL, and SystemC TLM
implementations.

---

## 1. Non-negotiable constraints

1. **Keep the bridge ABI stable.** The shared library is named `scvpi`; the
   Python extension module is `scvpi_tlm_ext`; the C entry point is
   `scvpi_tlm_ext_entry`; the endpoint transport symbol is
   `scvpi_tlm_transport_bytes`. Do not rename these without updating every
   Makefile, CMake file, and Python import.
2. **Do not break the example Makefiles.** They compile `src/scvpi.cpp` and
   `src/scvpi_tlm_bridge.cpp` directly into the simulator executable. New
   source files must be added to every affected Makefile.
3. **Do not edit generated RTL.** `examples/tinyalu_reg/model_hdl/TinyALUreg.sv`
   is generated from `tests/TinyALUreg.rdl` by PeakRDL-verilog. Change the RDL
   and regenerate instead.
4. **Keep timing modes distinct.** Annotated-delay TLM returns `delay_ps` to
   Python; target-wait TLM consumes time with `sc_core::wait()` and returns
   `delay_ps == 0`. Never double-count latency.
5. **No generated files in git.** Build trees, waveforms, cocotb XML, and
   `__pycache__` are ignored.

---

## 2. Toolchain bootstrap

Target environment: Ubuntu 24.04 or WSL2.

```bash
sudo apt-get update
sudo apt-get install -y --no-install-recommends \
  build-essential cmake ninja-build git wget iverilog \
  python3.12 python3.12-venv

export SYSTEMC=/opt/systemc-3.0.2
export SYSTEMC_HOME="$SYSTEMC"
export LD_LIBRARY_PATH="$SYSTEMC/lib:$SYSTEMC/lib64:$LD_LIBRARY_PATH"

uv sync
source .venv/bin/activate
```

If SystemC is missing, build version 3.0.2 from the Accellera release tarball
and install it under `$SYSTEMC`. Do not silently switch to SystemC 2.3.x; the
examples use 3.0.2 APIs and CMake package hints.

---

## 3. Canonical build commands

### 3.1 C/C++ bridge via CMake

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$SYSTEMC" \
  -DSystemC_ROOT="$SYSTEMC" \
  -DPython3_EXECUTABLE="$(command -v python)"

cmake --build build -j"$(nproc)"
```

Expected artifacts:

- `build/libscvpi.so`
- `build/scvpi_tlm_ext*.so`

Useful options:

- `-DSCVPI_BUILD_PYTHON_EXT=OFF` — build only the C++ bridge.
- `-DCMAKE_BUILD_TYPE=Debug` — assertions and symbols.
- `-DSystemC_LIBRARY="$SYSTEMC/lib/libsystemc.a"` — override detection when
  using a non-standard SystemC installation.

### 3.2 Python environment via uv

```bash
uv sync
source .venv/bin/activate
python -c "import cocotb, pyuvm; print(cocotb.__version__)"
```

Never install project dependencies with plain `pip`; keep `pyproject.toml` as
the source of truth. If `uv.lock` is added later, use `uv sync --locked` in CI.

### 3.3 Examples

The standalone examples compile SCVPI into the executable:

```bash
make -C examples/basic run RANDOM_SEED=12345
make -C examples/adder run RANDOM_SEED=12345
make -C examples/TinyALU_SystemC run RANDOM_SEED=12345
```

The unified register-model regression lives in `examples/tinyalu_reg`:

```bash
cd examples/tinyalu_reg

make run-hdl                 RANDOM_SEED=12345
make run-sc-rtl              RANDOM_SEED=12345
make run-sc-tlm              RANDOM_SEED=12345
make run-sc-tlm-target-wait  RANDOM_SEED=12345
```

`run-sc-tlm-target-wait` changes a CMake compile definition. Always run
`make clean` after switching between annotated-delay and target-wait mode.

---

## 4. Verification target matrix

| Command | DUT | Bridge | Required tools |
| --- | --- | --- | --- |
| `make run-hdl` | `tinyalu.sv` + `TinyALUreg.sv` | Icarus VPI | `iverilog`, `vvp` |
| `make run-sc-rtl` | `tinyalu_reg_systemc.h` | SCVPI VPI + `sc_signal` handles | SystemC |
| `make run-sc-tlm` | `tinyalu_reg_tlm.h` | `scvpi_tlm_ext` + `b_transport` (annotated delay) | SystemC, Python dev headers |
| `make run-sc-tlm-target-wait` | `tinyalu_reg_tlm.h` | `b_transport` with `wait()` | SystemC, Python dev headers |

All four targets must pass for a change to be considered complete.

Useful variables:

```text
RANDOM_SEED=<n>      cocotb random seed
TESTCASE=<name>      run one test
SCVPI_LOG=1          verbose signal access log
TRACE=1              VCD/transaction traces (see model)
TLM_TARGET_CONSUMES_TIME=0|1
```

Useful tests in the unified testbench: `AluTest`, `FibonacciTest`,
`AluTestErrors`, `LongRandomTest`, `ZeroCornerTest`.

---

## 5. Code map

```text
src/scvpi.h/.cpp              SCVPI runtime, +vpi= loader, C transport API
src/vpi.icc                   VPI entry points over SystemC
src/scvpi_tlm_bridge.h/.cpp   endpoint registry and byte transport
src/scvpi_tlm_ext.cpp         CPython extension module
src/ScVpi*.{h,icc}            handle, iterator, callback implementations
examples/tinyalu_reg/         unified PyUVM RAL testbench
examples/TinyALU_SystemC/     PyUVM on a signal-level SystemC model
examples/basic/adder/         cocotb scheduler smoke tests
tools/check_cocotb_results.py XML result gate
```

Key control flow:

1. `sc_main()` instantiates the DUT and calls `scvpi_autoregister_all()`.
2. The executable is launched with `+vpi=<cocotb vpi .so>`.
3. `ScVpi::end_of_elaboration()` loads that library and invokes its startup
   routines.
4. cocotb discovers handles through the SCVPI VPI functions.
5. For TLM targets, `scvpi_register_tlm_endpoint()` publishes a named
   `b_transport` callable; Python calls `scvpi_tlm_ext.transport()`.

---

## 6. Agent workflow

1. **Read before editing.** Inspect the relevant Makefile, CMakeLists, and
   Python testbench. The same model is built three ways; a change in one path
   often needs matching changes in the others.
2. **Plan the smallest coherent change.** Prefer extending `SignalKind`,
   `classify()`, `vpi_get_value()`, and `vpi_put_value()` together for a new
   signal type.
3. **Build the bridge first** with the CMake command in §3.1 when touching
   `src/`.
4. **Run the fast smoke tests** (`examples/basic`, `examples/adder`) before the
   full TinyALU regression.
5. **Run all four targets** for changes affecting callbacks, TLM transport,
   timing, or the register model.
6. **Gate on results.** A run is only successful when
   `tools/check_cocotb_results.py` reports all tests passed. Do not trust the
   process exit code alone; cocotb can exit zero with a failed XML result.
7. **Keep commits scoped.** Do not mix generated build output with source
   changes. Do not rewrite unrelated files.

### Debugging order

- Set `SCVPI_LOG=1` to confirm signal reads/writes and endpoint traffic.
- Set `TRACE=1` (and `TRACE_DIR=...`) for VCD/transaction traces.
- For TLM, log `response_status`, `delay_ps`, and the endpoint name.
- If the first target-wait transaction hangs, verify
  `TLM_TARGET_CONSUMES_TIME=1` and the one-time `Timer(1, "step")` handoff in
  `examples/tinyalu_reg/tests/scvpi_tlm.py`.
- If cocotb cannot find a handle, dump the SystemC hierarchy and compare full
  names with the names used in the testbench.

---

## 7. Coding conventions

- C++17, tabs or four spaces consistent with the surrounding file.
- Keep `.icc` implementation files included from `scvpi.cpp`; they are not
  standalone translation units.
- Guard public headers with `#pragma once`.
- Prefer `sc_core::` qualification in new code.
- Python: type hints on public helpers, `snake_case` functions, PyUVM class
  names in `UpperCamelCase`.
- Do not add license or author headers that conflict with the repository
  license. New files should carry a short file-purpose header.

---

## 8. Definition of done

A change is complete when:

- `cmake --build build` succeeds without new warnings,
- `uv sync` succeeds,
- the relevant standalone example passes,
- all four `tinyalu_reg` targets pass with `RANDOM_SEED=12345`,
- `git status` contains only intentional source changes.

For reusable TLM-2.0 register-model work, follow the
[`tlm2-reg-model-verification`](SKILLS.md) skill.
