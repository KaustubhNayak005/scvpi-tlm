#include "tinyalu_reg_tlm.h"

#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>

using namespace sc_core;

static inline uint32_t load_u32_le(const unsigned char* p) {
  uint32_t v = 0;
  std::memcpy(&v, p, sizeof(v));
  return v;
}

static inline void store_u32_le(unsigned char* p, uint32_t v) {
  std::memcpy(p, &v, sizeof(v));
}

static inline uint32_t apply_byte_enables_u32(
    uint32_t oldv,
    uint32_t newv,
    const unsigned char* be,
    unsigned int be_len) {
  if (!be || be_len == 0) return newv;

  uint32_t out = oldv;
  for (unsigned i = 0; i < 4; i++) {
    const bool en = (i < be_len) ? (be[i] != 0) : false;
    if (en) {
      const uint32_t byte_mask = 0xFFu << (i * 8);
      out = (out & ~byte_mask) | (newv & byte_mask);
    }
  }
  return out;
}

static inline const char* cmd_to_str(tlm::tlm_command cmd) {
  if (cmd == tlm::TLM_READ_COMMAND) return "READ ";
  if (cmd == tlm::TLM_WRITE_COMMAND) return "WRITE";
  return "OTHER";
}

static inline const char* addr_to_reg(uint64_t a) {
  if (a == 0x0) return "SRC   ";
  if (a == 0x2) return "RESULT";
  if (a == 0x4) return "CMD   ";
  return "UNKNOWN";
}

static inline const char* op_to_str(uint8_t op3) {
  switch (op3) {
    case 0b001: return "ADD";
    case 0b010: return "AND";
    case 0b011: return "XOR";
    case 0b100: return "MUL";
    default:    return "NOP/OTHER";
  }
}

static inline bool tlm_trace_enabled() {
  const char* e = std::getenv("TLM_TRACE");
  return e && std::atoi(e) != 0;
}

static inline void print_access(
    const sc_time& now,
    tlm::tlm_command cmd,
    uint64_t addr,
    uint32_t data,
    const sc_time& delay) {
  if (!tlm_trace_enabled()) return;

  std::cout << "[TLM] "
            << std::setw(8) << now << " "
            << cmd_to_str(cmd)
            << " reg=" << addr_to_reg(addr)
            << " addr=0x" << std::hex << addr
            << " data=0x" << std::setw(8) << std::setfill('0') << data
            << std::dec << std::setfill(' ')
            << " delay=" << delay
            << std::endl;
}

static inline void print_resp(
    const sc_time& now,
    uint64_t addr,
    uint32_t rdata) {
  if (!tlm_trace_enabled()) return;

  std::cout << "[TLM] "
            << std::setw(8) << now
            << " RESP "
            << " reg=" << addr_to_reg(addr)
            << " addr=0x" << std::hex << addr
            << " rdata=0x" << std::setw(8) << std::setfill('0') << rdata
            << std::dec << std::setfill(' ')
            << std::endl;
}

static inline void print_start(
    const sc_time& now,
    uint8_t a,
    uint8_t b,
    uint8_t op3,
    uint16_t result,
    const sc_time& done_time) {
  if (!tlm_trace_enabled()) return;

  std::cout << "[TLM] "
            << std::setw(8) << now
            << " START"
            << " op=" << op_to_str(op3)
            << " op_code=" << static_cast<unsigned>(op3)
            << " A=0x" << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<unsigned>(a)
            << " B=0x" << std::setw(2)
            << static_cast<unsigned>(b)
            << " expected_result=0x" << std::setw(4)
            << result
            << std::dec << std::setfill(' ')
            << " done_time=" << done_time
            << std::endl;
}

static inline void print_done(
    const sc_time& now,
    uint8_t a,
    uint8_t b,
    uint8_t op3,
    uint16_t result) {
  if (!tlm_trace_enabled()) return;

  std::cout << "[TLM] "
            << std::setw(8) << now
            << " DONE "
            << " op=" << op_to_str(op3)
            << " A=0x" << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<unsigned>(a)
            << " B=0x" << std::setw(2)
            << static_cast<unsigned>(b)
            << " result=0x" << std::setw(4)
            << result
            << std::dec << std::setfill(' ')
            << std::endl;
}

TinyALU_TLM::TinyALU_TLM(sc_module_name nm) : sc_module(nm) {
  tsock.register_b_transport(this, &TinyALU_TLM::b_transport);
}

TinyALU_TLM::~TinyALU_TLM() = default;

uint16_t TinyALU_TLM::compute_result(uint8_t A, uint8_t B, uint8_t op3) {
  const bool is_mul = (op3 & 0x4) != 0;

  if (!is_mul) {
    switch (op3) {
      case 0b001:
        return static_cast<uint16_t>(A) + static_cast<uint16_t>(B);
      case 0b010:
        return static_cast<uint16_t>(A) & static_cast<uint16_t>(B);
      case 0b011:
        return static_cast<uint16_t>(A) ^ static_cast<uint16_t>(B);
      default:
        return (static_cast<uint16_t>(A) << 8) | static_cast<uint16_t>(B);
    }
  }

  return static_cast<uint16_t>(
      static_cast<uint16_t>(A) * static_cast<uint16_t>(B));
}

void TinyALU_TLM::update_hw_state(const sc_time& now) {
  if (busy && now >= done_time) {
    result_hw = pending_result;
    done_hw = true;
    busy = false;

    print_done(now, pending_a, pending_b, pending_op3, pending_result);
  }
}

void TinyALU_TLM::b_transport(tlm::tlm_generic_payload& trans, sc_time& delay) {
  const auto cmd = trans.get_command();
  const uint64_t a = trans.get_address();
  auto* data_ptr = trans.get_data_ptr();
  const unsigned int len = trans.get_data_length();
  const auto* be_ptr = trans.get_byte_enable_ptr();
  const unsigned int be_len = trans.get_byte_enable_length();

  if (!data_ptr || len == 0) {
    trans.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
    return;
  }

  uint32_t access_data = 0;
  {
    unsigned char tmp_data[4] = {0, 0, 0, 0};
    for (unsigned i = 0; i < len && i < 4; i++) {
      tmp_data[i] = data_ptr[i];
    }
    access_data = load_u32_le(tmp_data);
  }

#ifdef TLM_TARGET_CONSUMES_TIME
  // Target-side timing mode: consume latency inside b_transport().
  // The Python side must not consume delay_ps again in this mode.
  if (delay != SC_ZERO_TIME) {
    wait(delay);
    delay = SC_ZERO_TIME;
  }

  wait(t_bus);
  delay = SC_ZERO_TIME;

  const sc_time now = sc_time_stamp();
#else
  // Annotated-delay LT style: return target latency in delay.
  // cocotb/Python consumes this delay with Timer(delay_ps).
  delay += t_bus;

  const sc_time now = sc_time_stamp() + delay;
#endif

  update_hw_state(now);
  print_access(now, cmd, a, access_data, delay);

  constexpr uint64_t SRC_A = 0x0;
  constexpr uint64_t RESULT_A = 0x2;
  constexpr uint64_t CMD_A = 0x4;

  if (cmd == tlm::TLM_READ_COMMAND) {
    uint32_t rd = 0;

    if (a == SRC_A) {
      rd |= static_cast<uint32_t>(src_data0);
      rd |= static_cast<uint32_t>(src_data1) << 8;
    } else if (a == RESULT_A) {
      rd |= static_cast<uint32_t>(result_hw);
    } else if (a == CMD_A) {
      rd |= static_cast<uint32_t>(cmd_op & 0x1F);
      rd |= (cmd_start ? 1u : 0u) << 5;
      rd |= (done_hw ? 1u : 0u) << 6;
      rd |= (static_cast<uint32_t>((cmd_reserved >> 7) & 0x1FF)) << 7;
    } else {
      trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
      return;
    }

    unsigned char tmp[4] = {0, 0, 0, 0};
    store_u32_le(tmp, rd);
    for (unsigned i = 0; i < len && i < 4; i++) {
      data_ptr[i] = tmp[i];
    }

    print_resp(now, a, rd);

    trans.set_response_status(tlm::TLM_OK_RESPONSE);
    return;
  }

  if (cmd == tlm::TLM_WRITE_COMMAND) {
    unsigned char tmp[4] = {0, 0, 0, 0};
    for (unsigned i = 0; i < len && i < 4; i++) {
      tmp[i] = data_ptr[i];
    }

    const uint32_t wd = load_u32_le(tmp);

    if (a == SRC_A) {
      const uint32_t old = static_cast<uint32_t>(src_data0) |
                           (static_cast<uint32_t>(src_data1) << 8);
      const uint32_t merged = apply_byte_enables_u32(old, wd, be_ptr, be_len);

      src_data0 = static_cast<uint8_t>(merged & 0xFF);
      src_data1 = static_cast<uint8_t>((merged >> 8) & 0xFF);

      trans.set_response_status(tlm::TLM_OK_RESPONSE);
      return;
    }

    if (a == CMD_A) {
      uint32_t old = 0;
      old |= static_cast<uint32_t>(cmd_op & 0x1F);
      old |= (cmd_start ? 1u : 0u) << 5;
      old |= (done_hw ? 1u : 0u) << 6;
      old |= (static_cast<uint32_t>((cmd_reserved >> 7) & 0x1FF)) << 7;

      const uint32_t merged = apply_byte_enables_u32(old, wd, be_ptr, be_len);

      cmd_op = static_cast<int>(merged & 0x1F);
      cmd_start = ((merged >> 5) & 0x1) != 0;

      const uint32_t res9 = (merged >> 7) & 0x1FF;
      cmd_reserved = static_cast<uint16_t>(res9 << 7);

      const uint8_t op3 = static_cast<uint8_t>(cmd_op & 0x7);

      if (cmd_start && op3 != 0) {
        busy = true;
        done_hw = false;

        pending_a = src_data0;
        pending_b = src_data1;
        pending_op3 = op3;
        pending_result = compute_result(pending_a, pending_b, pending_op3);

        const bool is_mul = (op3 & 0x4) != 0;
        const sc_time compute_delay = is_mul ? t_mul : t_single;

        done_time = now + compute_delay;

        print_start(now, pending_a, pending_b, pending_op3,
                    pending_result, done_time);
      }

      trans.set_response_status(tlm::TLM_OK_RESPONSE);
      return;
    }

    if (a == RESULT_A) {
      // Ignore writes to read-only RESULT register.
      trans.set_response_status(tlm::TLM_OK_RESPONSE);
      return;
    }

    trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
    return;
  }

  trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
}