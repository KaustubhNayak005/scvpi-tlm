#include <systemc>

// Provided by SCVPI (linked from ../../src/scvpi.cpp)
extern "C" void scvpi_autoregister_all();

SC_MODULE(Adder) {
    sc_core::sc_in<int>  a{"a"};
    sc_core::sc_in<int>  b{"b"};
    sc_core::sc_out<int> sum{"sum"};

    void do_add() {
        sum.write(a.read() + b.read());
    }

    SC_CTOR(Adder) {
        SC_METHOD(do_add);
        sensitive << a << b;
        dont_initialize();
        sum.initialize(0);
    }
};

SC_MODULE(Top) {
    sc_core::sc_signal<int> a{"a"}, b{"b"}, sum{"sum"};
    Adder dut{"dut"};

    SC_CTOR(Top) {
        dut.a(a);
        dut.b(b);
        dut.sum(sum);
    }
};

int sc_main(int argc, char* argv[]) {
    Top top{"top"};

    scvpi_autoregister_all();

    sc_core::sc_set_stop_mode(sc_core::SC_STOP_IMMEDIATE);
    sc_core::sc_start();
    return 0;
}