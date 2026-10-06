# SKILLS.md — reusable verification skills

This file contains agent-executable skills for this repository. Each skill is
written so an AI coding agent can apply it without additional context.

---

## Skill: `tlm2-reg-model-verification`

**Goal:** wrap a SystemC `SC_MODULE` with a TLM-2.0 blocking transport
(`b_transport`) target socket, publish it through SCVPI, and verify its register
map with a PyUVM RAL environment running under cocotb.

**Applies to:** SystemC loosely-timed models, virtual prototypes, peripheral
models, and register-mapped accelerators.

**Prerequisites:**

- SystemC 3.0.2 with `tlm_utils::simple_target_socket`
- SCVPI bridge (`src/scvpi*.{h,cpp}`)
- Python 3.12+, cocotb 2.x, PyUVM
- A register map (RDL, spreadsheet, or address table)
- The `scvpi_tlm_ext` CPython extension built for the target

**Reference implementation:** `examples/tinyalu_reg/`.

---

### 1. Freeze the register contract

Before writing code, write down:

| Field | Meaning |
| --- | --- |
| Register name | Human-readable name used by the RAL |
| Byte address | Absolute address used by `b_transport` |
| Width | Register width in bits (usually 32 or 16) |
| Access | RO, RW, W1C, etc. |
| Fields | Name, bit offset, width, reset value |
| Side effects | Start pulse, clear-on-read, done flags |

Also record the bus semantics:

- byte enables supported? (`byte_en` vector)
- read latency and write latency
- error responses for unmapped addresses
- whether the target consumes time or annotates delay

Do not start the RAL until this table is stable. The SystemC target, RDL, and
PyUVM classes must agree on it.

---

### 2. Create the SystemC target socket

```cpp
#pragma once

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>

struct MyPeripheral : sc_core::sc_module {
  tlm_utils::simple_target_socket<MyPeripheral> tsock{"tsock"};

  // Register state
  uint32_t control = 0;
  uint32_t status  = 0;
  uint32_t data    = 0;

  // Loosely-timed latency
  sc_core::sc_time t_bus{4, sc_core::SC_NS};

  SC_HAS_PROCESS(MyPeripheral);
  explicit MyPeripheral(sc_core::sc_module_name nm) : sc_core::sc_module(nm) {
    tsock.register_b_transport(this, &MyPeripheral::b_transport);
  }

  void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
};
```

Register exactly one `b_transport` callback. If the model also needs
`get_direct_mem_ptr` or debug transport, add those explicitly; do not assume the
simple socket provides them.

---

### 3. Implement the `b_transport` contract

```cpp
void MyPeripheral::b_transport(tlm::tlm_generic_payload& trans,
                               sc_core::sc_time& delay) {
  const auto cmd = trans.get_command();
  const uint64_t addr = trans.get_address();
  auto* data = trans.get_data_ptr();
  const unsigned len = trans.get_data_length();
  const auto* be = trans.get_byte_enable_ptr();
  const unsigned be_len = trans.get_byte_enable_length();

  if (!data || len == 0) {
    trans.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
    return;
  }

  // Timing mode A: annotate delay for the Python side.
  delay += t_bus;

  if (cmd == tlm::TLM_READ_COMMAND) {
    uint32_t value = 0;
    switch (addr) {
      case 0x0: value = control; break;
      case 0x4: value = status;  break;
      case 0x8: value = data;    break;
      default:
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        return;
    }
    std::memcpy(data, &value, std::min<unsigned>(len, sizeof(value)));
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
    return;
  }

  if (cmd == tlm::TLM_WRITE_COMMAND) {
    uint32_t value = 0;
    std::memcpy(&value, data, std::min<unsigned>(len, sizeof(value)));

    switch (addr) {
      case 0x0:
        control = apply_byte_enables(control, value, be, be_len);
        break;
      case 0x8:
        data = apply_byte_enables(data, value, be, be_len);
        break;
      default:
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        return;
    }
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
    return;
  }

  trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
}
```

Rules:

- Always set a response status, including error paths.
- Honor byte enables on writes when the bus supports them.
- Never write read-only registers; return `TLM_OK_RESPONSE` or an error
  deliberately, and document which.
- Clamp `memcpy` to the register width; do not trust `len`.
- Keep `addr` byte-aligned and exact; the RAL adapter will use those addresses.

---

### 4. Choose the timing mode

| Mode | Target behavior | Python behavior | Build flag |
| --- | --- | --- | --- |
| Annotated delay | `delay += t_bus;` | `await Timer(delay_ps, "ps")` | default |
| Target-side wait | `wait(delay); wait(t_bus); delay = 0;` | no extra timer | `TLM_TARGET_CONSUMES_TIME=1` |

For target-side wait, guard the wait:

```cpp
#ifdef TLM_TARGET_CONSUMES_TIME
  if (delay != sc_core::SC_ZERO_TIME) {
    wait(delay);
    delay = sc_core::SC_ZERO_TIME;
  }
  wait(t_bus);
  delay = sc_core::SC_ZERO_TIME;
#endif
```

Never consume the delay twice. If the model computes in zero time, still return
a deterministic response; do not call `wait(SC_ZERO_TIME)` unnecessarily.

---

### 5. Publish the endpoint

Create a top-level module that owns the DUT and registers the endpoint after
construction:

```cpp
int sc_main(int argc, char* argv[]) {
  top t("top");

  scvpi_register_tlm_endpoint(
      "top.dut.tsock",
      [&](const scvpi_tlm_request& req) -> scvpi_tlm_response {
        tlm::tlm_generic_payload trans;
        sc_core::sc_time delay = sc_core::SC_ZERO_TIME;

        std::vector<unsigned char> data = req.data;
        std::vector<unsigned char> be = req.byte_en;

        trans.set_command(req.is_read ? tlm::TLM_READ_COMMAND
                                      : tlm::TLM_WRITE_COMMAND);
        trans.set_address(req.addr);
        trans.set_data_ptr(data.data());
        trans.set_data_length(static_cast<unsigned>(data.size()));
        trans.set_streaming_width(static_cast<unsigned>(data.size()));
        trans.set_byte_enable_ptr(be.empty() ? nullptr : be.data());
        trans.set_byte_enable_length(static_cast<unsigned>(be.size()));
        trans.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

        t.dut.tsock->b_transport(trans, delay);

        scvpi_tlm_response rsp;
        rsp.response_status = static_cast<int>(trans.get_response_status());
        rsp.data = data;
        rsp.delay_ps = static_cast<uint64_t>(delay.to_seconds() * 1.0e12);
        return rsp;
      });

  scvpi_autoregister_all();
  sc_core::sc_start();
  return 0;
}
```

Endpoint naming convention: `<top>.<instance>.<socket>`, for example
`top.dut.tsock`. Keep it stable; Python selects the endpoint by this string.

---

### 6. Add the Python transport

```python
import os
import cocotb
import scvpi_tlm_ext
from cocotb.triggers import Timer

TARGET_WAIT = os.getenv("TLM_TARGET_CONSUMES_TIME", "0") == "1"
_ready = False


class TlmRequest:
    def __init__(self, is_read: bool, addr: int, data: bytes,
                 byte_en: bytes | None = None):
        self.is_read = bool(is_read)
        self.addr = int(addr)
        self.data = bytes(data)
        self.byte_en = None if byte_en is None else bytes(byte_en)


async def transport(endpoint: str, req: TlmRequest):
    global _ready
    if TARGET_WAIT and not _ready:
        # One-time scheduler handoff: the first callback may arrive before a
        # SystemC process context exists. This is not modeled bus latency.
        await Timer(1, unit="step")
        _ready = True

    data, status, delay_ps = scvpi_tlm_ext.transport(
        endpoint, int(req.is_read), req.addr, req.data, req.byte_en
    )

    if delay_ps > 0 and not TARGET_WAIT:
        await Timer(delay_ps, unit="ps")

    return data, status, delay_ps
```

The transport call is synchronous inside the SystemC executable; the `await`
points are what let cocotb and SystemC share the scheduler.

---

### 7. Build the PyUVM RAL environment

Minimum components:

1. `uvm_reg_field` objects with reset value and access policy.
2. `uvm_reg` subclasses with `configure()` and field `configure()`.
3. A `uvm_reg_block` with a `uvm_reg_map`; `add_reg(reg, byte_offset, "RW")`.
4. A `uvm_reg_adapter` translating `uvm_reg_bus_op` to a bus item.
5. An adapter driver that calls `transport()` and fills `rdata`.
6. A monitor that observes writes/reads and emits transactions.
7. A scoreboard comparing the RAL prediction with the observed result.

Adapter essentials:

```python
class SimpleBusAdapter(uvm_reg_adapter):
    def reg2bus(self, rw):
        item = SimpleBusItem("item")
        item.read = 1 if rw.kind == uvm_access_e.UVM_READ else 0
        item.addr = rw.addr
        item.wdata = rw.data
        return item

    def bus2reg(self, bus_item, rw):
        rw.kind = (uvm_access_e.UVM_READ if bus_item.read
                   else uvm_access_e.UVM_WRITE)
        rw.data = bus_item.rdata if bus_item.read else bus_item.wdata
        rw.addr = bus_item.addr
        rw.status = uvm_status_e.UVM_IS_OK
```

Doorbell/start bits:

- Write operands first.
- Write the command register last with the start bit set.
- Poll the done bit with bounded retries; never loop forever.
- Read the result after done is observed.
- Clear the command register before the next operation.

---

### 8. Sign-off checklist

- [ ] Register map addresses match the RDL and the SystemC decode switches.
- [ ] Each register has a reset value and access policy.
- [ ] `b_transport` sets a response status on every path.
- [ ] Byte enables are honored or explicitly rejected.
- [ ] Annotated-delay mode passes with `TLM_TARGET_CONSUMES_TIME=0`.
- [ ] Target-wait mode passes after `make clean` and a fresh configure.
- [ ] The same tests pass on `run-hdl`, `run-sc-rtl`, and `run-sc-tlm`.
- [ ] `RANDOM_SEED=12345` and at least one additional seed pass.
- [ ] `make run-sc-tlm TESTCASE=LongRandomTest` passes.
- [ ] `tools/check_cocotb_results.py` reports zero failures and zero errors.
- [ ] `SCVPI_LOG=1` shows the expected transaction stream.

---

### Failure playbook

| Symptom | Likely cause | Fix |
| --- | --- | --- |
| `Unknown TLM endpoint` | Endpoint name mismatch | Match the `scvpi_register_tlm_endpoint` string exactly |
| First target-wait call hangs | No SystemC process context | Keep the one-time `Timer(1, "step")` handoff |
| Reads return stale data | Missing `delay += t_bus` or target update order | Update state before building the response, or wait first |
| Double-counted latency | Target waits and Python also sleeps | Set `delay = SC_ZERO_TIME` after target-side waits |
| `TLM_ADDRESS_ERROR_RESPONSE` | Address/width mismatch | Check byte addresses, not word indices |
| RAL writes corrupt adjacent fields | Byte enables ignored | Merge writes with `apply_byte_enables()` |
| Random test fails only in RTL | Monitor samples at the wrong clock edge | Capture on the bus handshake, not an arbitrary cycle |
| CI passes locally, fails on clean checkout | Stale CMake cache | `make clean` before changing `TLM_TARGET_CONSUMES_TIME` |

---

### Reference commands

```bash
cd examples/tinyalu_reg

make run-sc-tlm              RANDOM_SEED=12345
make run-sc-tlm-target-wait  RANDOM_SEED=12345
make run-sc-tlm TESTCASE=LongRandomTest RANDOM_SEED=12345

# Compare all three abstraction levels
make run-hdl    RANDOM_SEED=12345
make run-sc-rtl RANDOM_SEED=12345
make run-sc-tlm RANDOM_SEED=12345
```
