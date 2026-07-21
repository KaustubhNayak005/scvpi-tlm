import cocotb
from cocotb.triggers import Timer

print("Hello from TB")


async def my_task():
    print("my_task")
    #    print("--> wait")
    #    try:
    #        await Timer(5, "ns")
    #    except e as Exception:
    #        print("Exception: " + str(e))
    #    print("<-- wait")
    await Timer(100, "ns")
    if False:
        await h


@cocotb.test()
async def my_test(top):
    print("my_test: " + str(top))
    for h in top:
        print("  h: ")
    task = cocotb.start_soon(my_task())
    print("--> wait 1")
    await Timer(1, "ns")
    print("<-- wait 1")
    print("--> wait 2")
    await Timer(1, "ns")
    print("<-- wait 2")
    print("--> wait 3")
    await Timer(40, "ns")
    print("<-- wait 3")
    print("--> wait 4")
    await Timer(80, "ns")
    print("<-- wait 4")
    await task


# import cocotb;

# async def coro():
#    print("Hello from coro")
#    if False:
#        await oth

# @cocotb.test()
# def test(args):
#    print("Hello from test")
#    cocotb.fork(coro())