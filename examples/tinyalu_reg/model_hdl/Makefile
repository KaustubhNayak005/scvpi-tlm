CWD := $(CURDIR)

export COCOTB_REDUCED_LOG_FMT = 1
export PYTHONPATH := $(abspath ..):$(PYTHONPATH)

SIM ?= xcelium
TOPLEVEL_LANG ?= verilog

VERILOG_SOURCES := \
  $(CWD)/tinyalu.sv \
  $(CWD)/TinyALUreg.sv

COCOTB_TEST_MODULES := tests.testbench
TOPLEVEL := tinyalu

COCOTB_HDL_TIMEUNIT := 1ns
COCOTB_HDL_TIMEPRECISION := 1ps

include $(shell cocotb-config --makefiles)/Makefile.sim