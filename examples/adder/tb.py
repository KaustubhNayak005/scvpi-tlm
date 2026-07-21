import random
import cocotb
from cocotb.triggers import Timer

async def drive_and_read(dut, a, b, delay_ns=1) -> int:
    dut.a.value = a
    dut.b.value = b
    await Timer(delay_ns, unit="ns")
    return int(dut.sum.value)

@cocotb.test()
async def test_adder_vectors(dut):
    vectors = [
        (0, 0, 0),
        (1, 2, 3),
        (7, 35, 42),
        (-1, 1, 0),
        (-10, -5, -15),
        (123, 456, 579),
        (999, -1000, -1),
    ]

    for a, b, exp in vectors:
        got = await drive_and_read(dut, a, b)
        assert got == exp, f"a={a} b={b}: got={got}, expected={exp}"

@cocotb.test()
async def test_adder_random(dut):
    random.seed(1)

    for _ in range(30):
        a = random.randint(-1000, 1000)
        b = random.randint(-1000, 1000)
        exp = a + b

        got = await drive_and_read(dut, a, b)
        assert got == exp, f"random: a={a} b={b}: got={got}, expected={exp}"

@cocotb.test()
async def test_adder_error_demo(dut):
    """
    Intentional mismatch demo (does not fail CI)
    """
    a, b = 10, 20
    got = await drive_and_read(dut, a, b)
    wrong_exp = 999  # intentionally wrong

    if got != wrong_exp:
        cocotb.log.warning(
            f"[DEMO] Intentional mismatch: a={a} b={b} got={got} expected={wrong_exp}"
        )
    else:
        cocotb.log.warning(
            "[DEMO] Unexpectedly matched the intentionally wrong expected value."
        )