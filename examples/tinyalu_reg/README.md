# tinyalu_reg

This example demonstrates reuse of the same PyUVM register-model
verification environment across three TinyALU implementations:

- HDL RTL model
- SystemC RTL model
- SystemC TLM model

The same Python verification environment is used to verify all three
implementations without modification.

The TLM model supports two blocking-transport timing styles:

- **Annotated delay** (default): the SystemC target returns the transaction
  delay and cocotb advances simulation time.
- **Target-side wait**: the SystemC target consumes the transaction latency
  with `sc_core::wait()` inside `b_transport()`.

---

## Requirements

- SystemC 3.0.2
- Python 3.11 or newer
- uv
- cocotb 2.x
- PyUVM
- Icarus Verilog (required only for the HDL RTL model)

---

## Running the Example

From the repository root, install the Python dependencies:

```bash
uv sync
source .venv/bin/activate
```

If SystemC is not installed under `/usr`, set:

```bash
export SYSTEMC=/path/to/systemc
```

Change to the example directory:

```bash
cd examples/tinyalu_reg
```

Run one of the supported models with a fixed random seed:

```bash
# HDL RTL
make run-hdl RANDOM_SEED=12345

# SystemC RTL
make run-sc-rtl RANDOM_SEED=12345

# SystemC TLM, annotated-delay mode (default)
make run-sc-tlm RANDOM_SEED=12345

# SystemC TLM, target-side wait mode
make run-sc-tlm-target-wait RANDOM_SEED=12345
```

Run a specific test:

```bash
make run-sc-tlm TESTCASE=AluTest RANDOM_SEED=12345
```

Generate waveform traces:

```bash
make trace MODEL=tlm RANDOM_SEED=12345
```

---

## Models

### HDL RTL

Runs the SystemVerilog TinyALU implementation using Icarus Verilog.

### SystemC RTL

Runs the SystemC RTL implementation through SCVPI.

### SystemC TLM

Runs the SystemC transaction-level implementation through SCVPI using
TLM-2.0 blocking transport.

The default `run-sc-tlm` target uses annotated-delay timing. The
`run-sc-tlm-target-wait` target builds the same model with
`TLM_TARGET_CONSUMES_TIME=1`, causing `b_transport()` to consume the bus
latency with `sc_core::wait()`.

### Target-side wait startup

A SystemC `wait()` may only be called while an `SC_THREAD` or
`SC_CTHREAD` is active. Some simulator integrations invoke the initial
cocotb callback before a SystemC process has been entered.

To ensure consistent behaviour across supported simulators, the Python
TLM connector performs a one-time scheduler handoff before the first
transaction when target-side wait mode is enabled. This resumes the
coroutine from a valid SystemC callback context before invoking
`b_transport()`.

This scheduler handoff:

- is performed only once,
- is enabled only in target-side wait mode,
- is not part of the modeled TLM bus latency, and
- has no effect on annotated-delay mode.