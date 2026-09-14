import os

import cocotb
import scvpi_tlm_ext
from cocotb.triggers import Timer


TLM_TARGET_CONSUMES_TIME = os.getenv("TLM_TARGET_CONSUMES_TIME", "0") == "1"
_target_wait_context_ready = False


class TlmRequest:
    def __init__(self, is_read: bool, addr: int, data: bytes, byte_en: bytes | None = None):
        self.is_read = bool(is_read)
        self.addr = int(addr)
        self.data = bytes(data)
        self.byte_en = None if byte_en is None else bytes(byte_en)


class TlmResponse:
    def __init__(self, data: bytes, response_status: int, delay_ps: int = 0):
        self.data = bytes(data)
        self.response_status = int(response_status)
        self.delay_ps = int(delay_ps)


async def _ensure_target_wait_context() -> None:
    global _target_wait_context_ready

    if TLM_TARGET_CONSUMES_TIME and not _target_wait_context_ready:
        await Timer(1, unit="step")
        _target_wait_context_ready = True


async def transport(endpoint: str, req: TlmRequest) -> TlmResponse:
    # Required only by target-side wait mode. Annotated-delay mode does not
    # call sc_core::wait() and therefore does not need this startup yield.
    await _ensure_target_wait_context()

    be = req.byte_en if req.byte_en is not None else None

    data, rsp_status, delay_ps = scvpi_tlm_ext.transport(
        endpoint,
        1 if req.is_read else 0,
        int(req.addr),
        req.data,
        be,
    )

    # Default mode: target returns annotated delay and cocotb consumes it.
    # Target-side wait mode: target already consumed time inside b_transport().
    if int(delay_ps) > 0 and not TLM_TARGET_CONSUMES_TIME:
        await Timer(int(delay_ps), unit="ps")

    return TlmResponse(
        data=data,
        response_status=rsp_status,
        delay_ps=delay_ps,
    )