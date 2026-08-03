#include <systemc>
#include "tinyalu_systemc.h"

extern "C" void scvpi_autoregister_all();

SC_MODULE(Top) {
  sc_signal<bool> reset_n{"reset_n"};
  sc_signal<bool> clk{"clk"};
  sc_signal<bool> start{"start"};

  sc_signal<sc_dt::sc_uint<8>>  A{"A"};
  sc_signal<sc_dt::sc_uint<8>>  B{"B"};
  sc_signal<sc_dt::sc_uint<3>>  op{"op"};

  sc_signal<bool>               done{"done"};
  sc_signal<sc_dt::sc_uint<16>> result{"result"};

  tinyalu dut{"tinyalu"};

  SC_CTOR(Top) {
    dut.reset_n(reset_n);
    dut.clk(clk);
    dut.start(start);
    dut.A(A);
    dut.B(B);
    dut.op(op);
    dut.done(done);
    dut.result(result);
  }
};

int sc_main(int argc, char* argv[]) {

  Top top{"top"};
  scvpi_autoregister_all();

  sc_core::sc_set_stop_mode(sc_core::SC_STOP_IMMEDIATE);
  sc_core::sc_start();
  return 0;
}