/*
 * scvpi.cpp - SCVPI runtime and TLM transport implementation
 *
 * SCVPI-TLM: Unified SystemC TLM-2.0 & SystemVerilog RTL Co-Verification Framework
 * Created: 2026-07-14
 * Author: Kaustubh <nayakkaustubh@gmail.com>
 *
 * Responsibilities:
 *  - load the cocotb VPI shim from the +vpi= command line
 *  - auto-register the SystemC object hierarchy for VPI access
 *  - expose blocking TLM transport endpoints to Python
 */

#include "scvpi.h"

#include <stdio.h>
#include <vector>
#include <string.h>

#ifndef _WIN32
#include <dlfcn.h>
#else
#error "Not supported on Windows currently"
#endif

#include "scvpi_timed_callback.hpp"
#include "ScVpiCallbackBase.h"

#include "scvpi_tlm_bridge.h"

#include <cstring>

extern "C" int scvpi_tlm_transport_bytes(const char* endpoint,
                                         int is_read,
                                         uint64_t addr,
                                         unsigned char* data,
                                         uint32_t data_len,
                                         const unsigned char* byte_en,
                                         uint32_t byte_en_len,
                                         int* rsp_status_out) {
    if (!endpoint || !data || data_len == 0) {
        return -1;
    }

    try {
        scvpi_tlm_request req;
        req.is_read = (is_read != 0);
        req.addr = addr;
        req.data.assign(data, data + data_len);

        if (byte_en && byte_en_len > 0) {
            req.byte_en.assign(byte_en, byte_en + byte_en_len);
        }

        auto rsp = scvpi_tlm_transport(endpoint, req);

        const uint32_t copy_len =
            (rsp.data.size() < data_len) ? static_cast<uint32_t>(rsp.data.size()) : data_len;

        for (uint32_t i = 0; i < copy_len; i++) {
            data[i] = rsp.data[i];
        }

        if (rsp_status_out) {
            *rsp_status_out = rsp.response_status;
        }

        return 0;
    } catch (const std::exception& e) {
        fprintf(stderr, "scvpi_tlm_transport_bytes exception: %s\n", e.what());
        return -1;
    } catch (...) {
        fprintf(stderr, "scvpi_tlm_transport_bytes unknown exception\n");
        return -1;
    }
}

namespace scvpi {

// Forward-declared in vpi.icc
extern "C" void scvpi_autoregister_all();

ScVpi::ScVpi(const sc_core::sc_module_name &name)
: sc_module(name) {
}

ScVpi::~ScVpi() {
}

void ScVpi::end_of_elaboration() {

	for (int i = 1; i < sc_core::sc_argc(); i++) {
		const char *filename = nullptr;

		if (!strncmp(sc_core::sc_argv()[i], "+vpi=", 5)) {
			filename = &sc_core::sc_argv()[i][5];
		} else if (!strcmp(sc_core::sc_argv()[i], "-m") && (i + 1) < sc_core::sc_argc()) {
			filename = sc_core::sc_argv()[i + 1];
			i++;
		}

		if (filename) {
			fprintf(stdout, "Load VPI: %s\n", filename);

			void *hndl = dlopen(filename, RTLD_LAZY);
			if (!hndl) {
				fprintf(stdout, "Failed to load library: %s\n", dlerror());
				return;
			}

			void *startup_sym = dlsym(hndl, "vlog_startup_routines");
			if (!startup_sym) {
				fprintf(stdout, "Failed to find vlog_startup_routines: %s\n", dlerror());
				return;
			}

			typedef void (*init_f)();
			init_f *startup_f = reinterpret_cast<init_f*>(startup_sym);

			for (int j = 0; startup_f[j]; j++) {
				startup_f[j]();
			}
		}
	}

	// Build SystemC object registry for VPI
	scvpi_autoregister_all();
}

void ScVpi::start_of_simulation() {
	for (auto &cb : m_start_of_sim_cb) {
		cb->invoke();
	}
}

void ScVpi::end_of_simulation() {
	for (auto &cb : m_end_of_sim_cb) {
		cb->invoke();
	}
}

void ScVpi::add_start_of_sim_cb(ScVpiCallbackBase *cb) {
	m_start_of_sim_cb.push_back(ScVpiCallbackBaseUP(cb));
}

void ScVpi::add_end_of_sim_cb(ScVpiCallbackBase *cb) {
	m_end_of_sim_cb.push_back(ScVpiCallbackBaseUP(cb));
}

ScVpi *ScVpi::inst() {
	return &m_inst;
}

ScVpi ScVpi::m_inst("__scvpi_plugin");

} // namespace scvpi

#include "vpi.icc"
#include "ScVpiCallbackBase.icc"
#include "ScVpiHandle.icc"
#include "ScVpiHandleIterator.icc"
#include "ScVpiHandleObject.icc"
#include "ScVpiTimedCallback.icc"