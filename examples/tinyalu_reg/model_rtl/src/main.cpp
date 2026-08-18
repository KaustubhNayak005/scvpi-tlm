#include "tinyalu_reg_systemc.h"
#include <systemc>
#include <cstdlib>
#include <string>

extern "C" void scvpi_autoregister_all();

int sc_main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  top t("top");

  sc_core::sc_trace_file* tf = nullptr;

  const char* trace_env = std::getenv("SC_TRACE");
  if (trace_env && std::string(trace_env) == "1") {
    const char* trace_name = std::getenv("SC_TRACE_FILE");
    std::string filename = trace_name ? trace_name : "systemc_rtl";

    tf = sc_core::sc_create_vcd_trace_file(filename.c_str());

    sc_core::sc_trace(tf, t.clk, "clk");
    sc_core::sc_trace(tf, t.reset_n, "reset_n");

    sc_core::sc_trace(tf, t.valid, "valid");
    sc_core::sc_trace(tf, t.read, "read");
    sc_core::sc_trace(tf, t.addr, "addr");
    sc_core::sc_trace(tf, t.wdata, "wdata");
    sc_core::sc_trace(tf, t.wmask, "wmask");
    sc_core::sc_trace(tf, t.rdata, "rdata");

    sc_core::sc_trace(tf, t.result, "result");

    sc_core::sc_trace(tf, t.done_sel, "done_sel");
    sc_core::sc_trace(tf, t.done_aax, "done_aax");
    sc_core::sc_trace(tf, t.done_mult, "done_mult");

    sc_core::sc_trace(tf, t.result_aax, "result_aax");
    sc_core::sc_trace(tf, t.result_mult, "result_mult");

    sc_core::sc_trace(tf, t.op_from_rf, "op_from_rf");
    sc_core::sc_trace(tf, t.start_single, "start_single");
    sc_core::sc_trace(tf, t.start_mult, "start_mult");
  }

  scvpi_autoregister_all();
  sc_core::sc_start();

  if (tf) {
    sc_core::sc_close_vcd_trace_file(tf);
  }

  return 0;
}