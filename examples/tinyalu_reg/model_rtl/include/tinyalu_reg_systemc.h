#pragma once
#include <systemc>

using namespace sc_core;

SC_MODULE(TinyALUreg_sc) {
  sc_in<bool> clk{"clk"};
  sc_in<bool> resetn{"resetn"};

  sc_out<sc_dt::sc_uint<8>> SRC_data0_q{"SRC_data0_q"};
  sc_out<sc_dt::sc_uint<8>> SRC_data1_q{"SRC_data1_q"};

  sc_in<sc_dt::sc_uint<16>> RESULT_data_wdata{"RESULT_data_wdata"};

  sc_out<sc_dt::sc_uint<5>> CMD_op_q{"CMD_op_q"};
  sc_out<bool>              CMD_start_q{"CMD_start_q"};

  sc_in<bool> CMD_done_wdata{"CMD_done_wdata"};

  sc_in<bool>              CMD_reserved_we{"CMD_reserved_we"};
  sc_in<sc_dt::sc_uint<9>> CMD_reserved_wdata{"CMD_reserved_wdata"};

  sc_in<bool>               valid{"valid"};
  sc_in<bool>               read{"read"};
  sc_in<sc_dt::sc_uint<32>> addr{"addr"};
  sc_in<sc_dt::sc_uint<32>> wdata{"wdata"};
  sc_in<sc_dt::sc_uint<4>>  wmask{"wmask"};
  sc_out<sc_dt::sc_uint<32>> rdata{"rdata"};

  sc_dt::sc_uint<32> ADDR_OFFSET = 0;

  sc_dt::sc_uint<8>  src_data0{};
  sc_dt::sc_uint<8>  src_data1{};
  sc_dt::sc_uint<5>  cmd_op{};
  bool               cmd_start{};
  sc_dt::sc_uint<9>  cmd_reserved{};

  static sc_dt::sc_uint<32> byte_mask_to_bit_mask(sc_dt::sc_uint<4> be) {
    sc_dt::sc_uint<32> m = 0;
    for (int i = 0; i < 4; i++) {
      if (be[i]) {
        m.range((i * 8) + 7, i * 8) = 0xFF;
      }
    }
    return m;
  }

  void seq_regs() {
    if (!resetn.read()) {
      src_data0 = 0;
      src_data1 = 0;
      cmd_op = 0;
      cmd_start = false;
      cmd_reserved = 0;
    } else {
      const bool sw_wr = valid.read() && !read.read();
      const sc_dt::sc_uint<32> a = addr.read();

      const sc_dt::sc_uint<32> mask = byte_mask_to_bit_mask(wmask.read());
      const sc_dt::sc_uint<32> masked_data = wdata.read() & mask;

      const bool SRC_sw_wr = sw_wr && (a == (ADDR_OFFSET + 0x0));
      const bool CMD_sw_wr = sw_wr && (a == (ADDR_OFFSET + 0x4));

      if (SRC_sw_wr) {
        sc_dt::sc_uint<8> oldv = src_data0;
        sc_dt::sc_uint<8> newv = masked_data.range(7, 0).to_uint();
        sc_dt::sc_uint<8> m8 = mask.range(7, 0).to_uint();
        src_data0 = (newv & m8) | (oldv & ~m8);
      }

      if (SRC_sw_wr) {
        sc_dt::sc_uint<8> oldv = src_data1;
        sc_dt::sc_uint<8> newv = masked_data.range(15, 8).to_uint();
        sc_dt::sc_uint<8> m8 = mask.range(15, 8).to_uint();
        src_data1 = (newv & m8) | (oldv & ~m8);
      }

      if (CMD_sw_wr) {
        sc_dt::sc_uint<5> oldv = cmd_op;
        sc_dt::sc_uint<5> newv = masked_data.range(4, 0).to_uint();
        sc_dt::sc_uint<5> m5 = mask.range(4, 0).to_uint();
        cmd_op = (newv & m5) | (oldv & ~m5);
      }

      if (CMD_sw_wr) {
        const bool oldv = cmd_start;
        const bool newv = masked_data[5];
        const bool m1 = mask[5];
        cmd_start = (newv && m1) || (oldv && !m1);
      }

      {
        sc_dt::sc_uint<9> next = cmd_reserved;

        if (CMD_reserved_we.read()) {
          next = CMD_reserved_wdata.read();
        }

        if (CMD_sw_wr) {
          sc_dt::sc_uint<9> oldv = cmd_reserved;
          sc_dt::sc_uint<9> newv = masked_data.range(15, 7).to_uint();
          sc_dt::sc_uint<9> m9 = mask.range(15, 7).to_uint();
          next = (newv & m9) | (oldv & ~m9);
        }

        cmd_reserved = next;
      }
    }

    SRC_data0_q.write(src_data0);
    SRC_data1_q.write(src_data1);
    CMD_op_q.write(cmd_op);
    CMD_start_q.write(cmd_start);
  }

  void comb_rdata() {
    const bool sw_rd = valid.read() && read.read();
    const sc_dt::sc_uint<32> a = addr.read();

    sc_dt::sc_uint<32> SRC_q = 0;
    SRC_q.range(7, 0) = src_data0;
    SRC_q.range(15, 8) = src_data1;

    sc_dt::sc_uint<32> RESULT_q = 0;
    RESULT_q.range(15, 0) = RESULT_data_wdata.read();

    sc_dt::sc_uint<32> CMD_q = 0;
    CMD_q.range(4, 0) = cmd_op;
    CMD_q[5] = cmd_start ? 1 : 0;
    CMD_q[6] = CMD_done_wdata.read() ? 1 : 0;
    CMD_q.range(15, 7) = cmd_reserved;

    const sc_dt::sc_uint<32> SRC_rdata =
        (sw_rd && (a == (ADDR_OFFSET + 0x0))) ? SRC_q : sc_dt::sc_uint<32>(0);
    const sc_dt::sc_uint<32> RESULT_rdata =
        (sw_rd && (a == (ADDR_OFFSET + 0x2))) ? RESULT_q : sc_dt::sc_uint<32>(0);
    const sc_dt::sc_uint<32> CMD_rdata =
        (sw_rd && (a == (ADDR_OFFSET + 0x4))) ? CMD_q : sc_dt::sc_uint<32>(0);

    rdata.write(SRC_rdata | RESULT_rdata | CMD_rdata);
  }

  SC_CTOR(TinyALUreg_sc) {
    SC_METHOD(seq_regs);
    sensitive << clk.pos() << resetn.neg();
    dont_initialize();

    SC_METHOD(comb_rdata);
    sensitive << valid << read << addr << wdata << wmask
              << RESULT_data_wdata << CMD_done_wdata;
    dont_initialize();
  }
};

SC_MODULE(RegBlockWrap_sc) {
  sc_in<bool> clk{"clk"};
  sc_in<bool> resetn{"resetn"};

  sc_in<sc_dt::sc_uint<16>> RESULT_data_wdata{"RESULT_data_wdata"};
  sc_in<bool>               CMD_done_wdata{"CMD_done_wdata"};
  sc_in<bool>               CMD_reserved_we{"CMD_reserved_we"};
  sc_in<sc_dt::sc_uint<9>>  CMD_reserved_wdata{"CMD_reserved_wdata"};

  sc_in<bool>               valid{"valid"};
  sc_in<bool>               read{"read"};
  sc_in<sc_dt::sc_uint<32>> addr{"addr"};
  sc_in<sc_dt::sc_uint<32>> wdata{"wdata"};
  sc_in<sc_dt::sc_uint<4>>  wmask{"wmask"};
  sc_out<sc_dt::sc_uint<32>> rdata{"rdata"};

  sc_signal<sc_dt::sc_uint<8>> SRC_data0_q{"SRC_data0_q"};
  sc_signal<sc_dt::sc_uint<8>> SRC_data1_q{"SRC_data1_q"};
  sc_signal<sc_dt::sc_uint<5>> CMD_op_q{"CMD_op_q"};
  sc_signal<bool>              CMD_start_q{"CMD_start_q"};

  TinyALUreg_sc u{"u"};

  SC_CTOR(RegBlockWrap_sc) {
    u.clk(clk);
    u.resetn(resetn);
    u.SRC_data0_q(SRC_data0_q);
    u.SRC_data1_q(SRC_data1_q);
    u.RESULT_data_wdata(RESULT_data_wdata);
    u.CMD_op_q(CMD_op_q);
    u.CMD_start_q(CMD_start_q);
    u.CMD_done_wdata(CMD_done_wdata);
    u.CMD_reserved_we(CMD_reserved_we);
    u.CMD_reserved_wdata(CMD_reserved_wdata);
    u.valid(valid);
    u.read(read);
    u.addr(addr);
    u.wdata(wdata);
    u.wmask(wmask);
    u.rdata(rdata);
  }
};

SC_MODULE(single_cycle_sc) {
  sc_in<bool> clk{"clk"};
  sc_in<bool> reset_n{"reset_n"};

  sc_in<sc_dt::sc_uint<8>> A{"A"};
  sc_in<sc_dt::sc_uint<8>> B{"B"};
  sc_in<sc_dt::sc_uint<3>> op{"op"};
  sc_in<bool> start{"start"};

  sc_out<bool> done{"done"};
  sc_out<sc_dt::sc_uint<16>> result{"result"};

  sc_dt::sc_uint<16> result_reg{};
  bool done_reg{};

  void seq() {
    if (!reset_n.read()) {
      result_reg = 0;
      done_reg = false;
    } else {
      sc_dt::sc_uint<16> r = 0;

      switch (op.read().to_uint()) {
        case 0b001:
          r = (sc_dt::sc_uint<16>)A.read() + (sc_dt::sc_uint<16>)B.read();
          break;
        case 0b010:
          r = (sc_dt::sc_uint<16>)A.read() & (sc_dt::sc_uint<16>)B.read();
          break;
        case 0b011:
          r = (sc_dt::sc_uint<16>)A.read() ^ (sc_dt::sc_uint<16>)B.read();
          break;
        default: {
          sc_dt::sc_uint<16> concat = 0;
          concat.range(15, 8) = A.read();
          concat.range(7, 0) = B.read();
          r = concat;
        } break;
      }

      if (op.read().to_uint() == 0b000 || op.read().to_uint() >= 0b100) {
        sc_dt::sc_uint<16> concat = 0;
        concat.range(15, 8) = A.read();
        concat.range(7, 0) = B.read();
        r = concat;
      }

      result_reg = r;
      done_reg = start.read() && (op.read() != 0);
    }

    result.write(result_reg);
    done.write(done_reg);
  }

  SC_CTOR(single_cycle_sc) {
    SC_METHOD(seq);
    sensitive << clk.pos();
    dont_initialize();
  }
};

SC_MODULE(three_cycle_sc) {
  sc_in<bool> clk{"clk"};
  sc_in<bool> reset_n{"reset_n"};

  sc_in<sc_dt::sc_uint<8>> A{"A"};
  sc_in<sc_dt::sc_uint<8>> B{"B"};
  sc_in<sc_dt::sc_uint<3>> op{"op"};
  sc_in<bool> start{"start"};

  sc_out<bool> done{"done"};
  sc_out<sc_dt::sc_uint<16>> result{"result"};

  sc_dt::sc_uint<8> a_reg{};
  sc_dt::sc_uint<8> b_reg{};
  sc_dt::sc_uint<16> mult1_reg{};
  sc_dt::sc_uint<16> mult2_reg{};
  sc_dt::sc_uint<16> result_reg{};
  bool done1_reg{};
  bool done2_reg{};
  bool done3_reg{};

  void seq() {
    if (!reset_n.read()) {
      a_reg = 0;
      b_reg = 0;
      mult1_reg = 0;
      mult2_reg = 0;
      result_reg = 0;
      done1_reg = false;
      done2_reg = false;
      done3_reg = false;
    } else {
      const sc_dt::sc_uint<8> next_a = A.read();
      const sc_dt::sc_uint<8> next_b = B.read();
      const sc_dt::sc_uint<16> next_mult1 =
          (sc_dt::sc_uint<16>)(a_reg * b_reg);

      a_reg = next_a;
      b_reg = next_b;

      mult2_reg = mult1_reg;
      mult1_reg = next_mult1;
      result_reg = mult2_reg;

      done3_reg = done2_reg;
      done2_reg = done1_reg;
      done1_reg = start.read();
    }

    result.write(result_reg);
    done.write(done3_reg);
  }

  SC_CTOR(three_cycle_sc) {
    SC_METHOD(seq);
    sensitive << clk.pos();
    dont_initialize();
  }
};

SC_MODULE(top) {
  sc_signal<bool> clk{"clk"};
  sc_signal<bool> reset_n{"reset_n"};

  sc_signal<bool>               valid{"valid"};
  sc_signal<bool>               read{"read"};
  sc_signal<sc_dt::sc_uint<32>> addr{"addr"};
  sc_signal<sc_dt::sc_uint<32>> wdata{"wdata"};
  sc_signal<sc_dt::sc_uint<4>>  wmask{"wmask"};
  sc_signal<sc_dt::sc_uint<32>> rdata{"rdata"};

  sc_signal<sc_dt::sc_uint<16>> result{"result"};

  RegBlockWrap_sc regblock{"regblock"};
  single_cycle_sc aax{"aax"};
  three_cycle_sc  mul{"mul"};

  sc_signal<bool>               done_sel{"done_sel"};
  sc_signal<bool>               done_aax{"done_aax"};
  sc_signal<bool>               done_mult{"done_mult"};
  sc_signal<sc_dt::sc_uint<16>> result_aax{"result_aax"};
  sc_signal<sc_dt::sc_uint<16>> result_mult{"result_mult"};

  sc_signal<sc_dt::sc_uint<3>> op_from_rf{"op_from_rf"};
  sc_signal<bool>              start_single{"start_single"};
  sc_signal<bool>              start_mult{"start_mult"};

  sc_signal<bool>              const1_sig{"const1_sig"};
  sc_signal<sc_dt::sc_uint<9>> const0_9_sig{"const0_9_sig"};

  void comb_selects() {
    op_from_rf.write(regblock.CMD_op_q.read().range(2, 0));
    const bool is_mul = regblock.CMD_op_q.read()[2];
    start_single.write(regblock.CMD_start_q.read() && !is_mul);
    start_mult.write(regblock.CMD_start_q.read() && is_mul);

    done_sel.write(is_mul ? done_mult.read() : done_aax.read());
    result.write(is_mul ? result_mult.read() : result_aax.read());
  }

  SC_CTOR(top) {
    const1_sig.write(true);
    const0_9_sig.write(0);

    regblock.clk(clk);
    regblock.resetn(reset_n);
    regblock.RESULT_data_wdata(result);
    regblock.CMD_done_wdata(done_sel);
    regblock.CMD_reserved_we(const1_sig);
    regblock.CMD_reserved_wdata(const0_9_sig);
    regblock.valid(valid);
    regblock.read(read);
    regblock.addr(addr);
    regblock.wdata(wdata);
    regblock.wmask(wmask);
    regblock.rdata(rdata);

    aax.clk(clk);
    aax.reset_n(reset_n);
    aax.A(regblock.SRC_data0_q);
    aax.B(regblock.SRC_data1_q);
    aax.op(op_from_rf);
    aax.start(start_single);
    aax.done(done_aax);
    aax.result(result_aax);

    mul.clk(clk);
    mul.reset_n(reset_n);
    mul.A(regblock.SRC_data0_q);
    mul.B(regblock.SRC_data1_q);
    mul.op(op_from_rf);
    mul.start(start_mult);
    mul.done(done_mult);
    mul.result(result_mult);

    SC_METHOD(comb_selects);
    sensitive << regblock.CMD_op_q << regblock.CMD_start_q
              << done_aax << done_mult << result_aax << result_mult;
    dont_initialize();
  }
};