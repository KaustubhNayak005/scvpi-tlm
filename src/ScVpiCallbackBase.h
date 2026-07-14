/*
 * ScVpiCallbackBase.h - VPI callback handle for SystemC
 *
 * SCVPI-TLM: Unified SystemC TLM-2.0 & SystemVerilog RTL Co-Verification Framework
 * Created: 2026-07-14
 * Author: Kaustubh <nayakkaustubh@gmail.com>
 */

#pragma once
#include "vpi_user_int.h"
#include "ScVpiHandle.h"

namespace scvpi {

class ScVpiCallbackBase : public ScVpiHandle {

public:
	ScVpiCallbackBase(p_cb_data cbd);

	virtual ~ScVpiCallbackBase();

	void invoke();

protected:
	s_cb_data			m_cb_data;
};

}
