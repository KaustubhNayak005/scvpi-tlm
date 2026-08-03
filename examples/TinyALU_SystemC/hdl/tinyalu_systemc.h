#pragma once
#include <systemc>

using namespace sc_core;
using namespace sc_dt;

SC_MODULE(single_cycle) {
  sc_in<sc_uint<8>>  A{"A"};
  sc_in<sc_uint<8>>  B{"B"};
  sc_in<sc_uint<3>>  op{"op"};
  sc_in<bool>        clk{"clk"};
  sc_in<bool>        reset_n{"reset_n"};
  sc_in<bool>        start{"start"};

  sc_out<bool>        done{"done"};
  sc_out<sc_uint<16>> result{"result"};

  void proc() {
    result.write(0);
    done.write(false);
    wait();

    while (true) {
      sc_uint<16> r;
      sc_uint<3>  o = op.read();
      sc_uint<8>  a = A.read();
      sc_uint<8>  b = B.read();

      switch (o.to_uint()) {
        case 0b001: r = (sc_uint<16>)a + (sc_uint<16>)b; break;
        case 0b010: r = (sc_uint<16>)a & (sc_uint<16>)b; break;
        case 0b011: r = (sc_uint<16>)a ^ (sc_uint<16>)b; break;
        default:    r = (sc_uint<16>)((a, b)); break;
      }
      result.write(r);

      done.write(start.read() && (o != 0));
      wait();
    }
  }

  SC_CTOR(single_cycle) {
    SC_CTHREAD(proc, clk.pos());
    reset_signal_is(reset_n, false);
  }
};

SC_MODULE(three_cycle) {
  sc_in<sc_uint<8>>  A{"A"};
  sc_in<sc_uint<8>>  B{"B"};
  sc_in<sc_uint<3>>  op{"op"};
  sc_in<bool>        clk{"clk"};
  sc_in<bool>        reset_n{"reset_n"};
  sc_in<bool>        start{"start"};

  sc_out<bool>        done{"done"};
  sc_out<sc_uint<16>> result{"result"};

  void proc() {
    sc_uint<8>  a_int = 0, b_int = 0;
    sc_uint<16> mult1 = 0, mult2 = 0;
    bool done1 = false, done2 = false, done3 = false;

    done.write(false);
    result.write(0);
    wait();

    while (true) {
      sc_uint<8>  a_int_next = A.read();
      sc_uint<8>  b_int_next = B.read();
      sc_uint<16> mult1_next = (sc_uint<16>)a_int * (sc_uint<16>)b_int;
      sc_uint<16> mult2_next = mult1;
      sc_uint<16> res_next   = mult2;

      bool done_curr  = done.read();
      bool done3_next = start.read() && !done_curr;
      bool done2_next = done3 && !done_curr;
      bool done1_next = done2 && !done_curr;
      bool done_next  = done1 && !done_curr;

      a_int = a_int_next;
      b_int = b_int_next;
      mult1 = mult1_next;
      mult2 = mult2_next;

      result.write(res_next);

      done3 = done3_next;
      done2 = done2_next;
      done1 = done1_next;
      done.write(done_next);

      wait();
    }
  }

  SC_CTOR(three_cycle) {
    SC_CTHREAD(proc, clk.pos());
    reset_signal_is(reset_n, false);
  }
};

SC_MODULE(tinyalu) {
  sc_in<sc_uint<8>>  A{"A"};
  sc_in<sc_uint<8>>  B{"B"};
  sc_in<sc_uint<3>>  op{"op"};
  sc_in<bool>        clk{"clk"};
  sc_in<bool>        reset_n{"reset_n"};
  sc_in<bool>        start{"start"};

  sc_out<bool>        done{"done"};
  sc_out<sc_uint<16>> result{"result"};

  sc_signal<sc_uint<16>> result_aax{"result_aax"}, result_mult{"result_mult"};
  sc_signal<bool>        start_single{"start_single"}, start_mult{"start_mult"};
  sc_signal<bool>        done_aax{"done_aax"}, done_mult{"done_mult"};

  single_cycle u_single{"u_single"};
  three_cycle  u_mult{"u_mult"};

  void comb() {
    bool op2 = op.read()[2];

    start_single.write(start.read() && !op2);
    start_mult.write(start.read() &&  op2);

    done.write(op2 ? done_mult.read() : done_aax.read());
    result.write(op2 ? result_mult.read() : result_aax.read());
  }

  SC_CTOR(tinyalu) {
    u_single.A(A);
    u_single.B(B);
    u_single.op(op);
    u_single.clk(clk);
    u_single.reset_n(reset_n);
    u_single.start(start_single);
    u_single.done(done_aax);
    u_single.result(result_aax);

    u_mult.A(A);
    u_mult.B(B);
    u_mult.op(op);
    u_mult.clk(clk);
    u_mult.reset_n(reset_n);
    u_mult.start(start_mult);
    u_mult.done(done_mult);
    u_mult.result(result_mult);

    SC_METHOD(comb);
    sensitive << A << B << op << start << done_aax << done_mult << result_aax << result_mult;
  }
};