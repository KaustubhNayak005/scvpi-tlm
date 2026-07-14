/*
 * ScVpiTimedCallback.h - Timed VPI callback handle
 *
 * SCVPI-TLM: Unified SystemC TLM-2.0 & SystemVerilog RTL Co-Verification Framework
 * Created: 2026-07-14
 * Author: Kaustubh <nayakkaustubh@gmail.com>
 */

#pragma once
#include <memory>
#include <systemc>
#include "ScVpiCallbackBase.h"

namespace scvpi {
class sc_timed_callback;
typedef std::unique_ptr<sc_timed_callback> sc_timed_callback_up;

class ScVpiTimedCallback : public ScVpiCallbackBase {
public:
	ScVpiTimedCallback(p_cb_data cbd);

	virtual ~ScVpiTimedCallback();

private:

	sc_timed_callback_up			m_cb;;
};

} /* namespace scvpi */

