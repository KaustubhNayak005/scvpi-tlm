#include "tinyalu_reg_tlm.h"
#include "scvpi_tlm_bridge.h"

#include <systemc>
#include <tlm>

#include <cstdint>
#include <vector>

extern "C" void scvpi_autoregister_all();

static scvpi_tlm_response dispatch_transport(
    top& t,
    const scvpi_tlm_request& req) {
  tlm::tlm_generic_payload trans;
  sc_core::sc_time delay = sc_core::SC_ZERO_TIME;

  std::vector<unsigned char> data = req.data;
  std::vector<unsigned char> be = req.byte_en;

  if (data.empty()) {
    data.resize(4, 0);
  }

  trans.set_command(req.is_read ? tlm::TLM_READ_COMMAND
                                : tlm::TLM_WRITE_COMMAND);
  trans.set_address(req.addr);
  trans.set_data_ptr(data.data());
  trans.set_data_length(static_cast<unsigned int>(data.size()));

  if (!be.empty()) {
    trans.set_byte_enable_ptr(be.data());
    trans.set_byte_enable_length(static_cast<unsigned int>(be.size()));
  } else {
    trans.set_byte_enable_ptr(nullptr);
    trans.set_byte_enable_length(0);
  }

  trans.set_streaming_width(static_cast<unsigned int>(data.size()));
  trans.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

  t.bridge.isock->b_transport(trans, delay);

  scvpi_tlm_response rsp;
  rsp.response_status = static_cast<int>(trans.get_response_status());
  rsp.data = data;
  rsp.delay_ps = static_cast<std::uint64_t>(delay.to_seconds() * 1.0e12);
  return rsp;
}

int sc_main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  top t("top");

  scvpi_register_tlm_endpoint(
      "top.dut.tsock",
      [&](const scvpi_tlm_request& req) -> scvpi_tlm_response {
        return dispatch_transport(t, req);
      });

  scvpi_autoregister_all();
  sc_core::sc_start();
  return 0;
}