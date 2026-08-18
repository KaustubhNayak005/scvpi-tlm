/*
 * scvpi.h - VPI-compatible interface for SystemC
 *
 * SCVPI-TLM: Unified SystemC TLM-2.0 & SystemVerilog RTL Co-Verification Framework
 * Created: 2026-07-14
 * Author: Kaustubh <nayakkaustubh@gmail.com>
 *
 * Exposes the SCVPI lifecycle hooks and the byte-oriented TLM transport
 * bridge used by the Python verification environment.
 */

#pragma once
#include <memory>
#include <vector>

#ifndef SC_INCLUDE_DYNAMIC_PROCESSES
#define SC_INCLUDE_DYNAMIC_PROCESSES
#endif
#include <systemc>
#include <cstdint>

namespace scvpi {

class ScVpiCallbackBase;
typedef std::unique_ptr<ScVpiCallbackBase> ScVpiCallbackBaseUP;

class ScVpi : public sc_core::sc_module {
public:
	ScVpi(const sc_core::sc_module_name &name);

	virtual ~ScVpi();

	virtual void end_of_elaboration();

	virtual void start_of_simulation() override;

    virtual void end_of_simulation() override;

	void add_start_of_sim_cb(ScVpiCallbackBase *cb);

	void add_end_of_sim_cb(ScVpiCallbackBase *cb);

	virtual const char *kind() const {
		return "scvpi_plugin";
	}

	static ScVpi *inst();

private:
	std::vector<ScVpiCallbackBaseUP>		m_start_of_sim_cb;
	std::vector<ScVpiCallbackBaseUP>		m_end_of_sim_cb;

	static ScVpi		m_inst;

};
}

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Generic byte-oriented transport API.
 *
 * is_read:
 *   0 = write
 *   1 = read
 *
 * data/data_len:
 *   For write: input payload bytes
 *   For read:  output buffer filled by transport
 *
 * byte_en/byte_en_len:
 *   Optional byte enable array. May be nullptr / 0.
 *
 * rsp_status_out:
 *   Returns tlm response status as integer if non-null.
 */
int scvpi_tlm_transport_bytes(const char* endpoint,
                              int is_read,
                              uint64_t addr,
                              unsigned char* data,
                              uint32_t data_len,
                              const unsigned char* byte_en,
                              uint32_t byte_en_len,
                              int* rsp_status_out);

#ifdef __cplusplus
}
#endif