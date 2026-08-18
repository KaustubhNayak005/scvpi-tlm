#pragma once

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

#include <cstdint>

struct TinyALU_TLM : sc_core::sc_module {
  tlm_utils::simple_target_socket<TinyALU_TLM> tsock{"tsock"};

  // Register state
  uint8_t  src_data0 = 0;
  uint8_t  src_data1 = 0;
  int      cmd_op = 0;
  bool     cmd_start = false;
  uint16_t cmd_reserved = 0;

  uint16_t result_hw = 0;
  bool     done_hw   = false;

  // Loosely-timed model timing
  sc_core::sc_time t_single{0, sc_core::SC_NS};
  sc_core::sc_time t_mul{8, sc_core::SC_NS};
  sc_core::sc_time t_bus{4, sc_core::SC_NS};

  // Pending operation state
  bool busy = false;
  sc_core::sc_time done_time{sc_core::SC_ZERO_TIME};
  uint16_t pending_result = 0;
  uint8_t pending_a = 0;
  uint8_t pending_b = 0;
  uint8_t pending_op3 = 0;

  SC_HAS_PROCESS(TinyALU_TLM);
  TinyALU_TLM(sc_core::sc_module_name nm);
~TinyALU_TLM();

  void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);

  static uint16_t compute_result(uint8_t A, uint8_t B, uint8_t op3);
  void update_hw_state(const sc_core::sc_time& now);
};

struct TinyALU_TLM_Bridge : sc_core::sc_module {
  tlm_utils::simple_initiator_socket<TinyALU_TLM_Bridge> isock{"isock"};

  TinyALU_TLM_Bridge(sc_core::sc_module_name nm) : sc_core::sc_module(nm) {}
};

struct top : sc_core::sc_module {
  TinyALU_TLM dut{"dut"};
  TinyALU_TLM_Bridge bridge{"bridge"};

  top(sc_core::sc_module_name nm) : sc_core::sc_module(nm) {
    bridge.isock.bind(dut.tsock);
  }
};