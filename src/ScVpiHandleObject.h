/*
 * ScVpiHandleObject.h - VPI handle for a SystemC object
 *
 * SCVPI-TLM: Unified SystemC TLM-2.0 & SystemVerilog RTL Co-Verification Framework
 * Created: 2026-07-14
 * Author: Kaustubh <nayakkaustubh@gmail.com>
 */

#pragma once
#include "ScVpiHandle.h"

namespace sc_core {
    class sc_object;
}

namespace scvpi {

class ScVpiHandleObject : public ScVpiHandle {
public:
	ScVpiHandleObject();

	ScVpiHandleObject(sc_core::sc_object *obj);

	virtual ~ScVpiHandleObject();

	const char *name() const;

	sc_core::sc_object *obj() const { return m_obj; }

	void obj(sc_core::sc_object *obj) { m_obj = obj; }

	int32_t getVpiType() const;

private:
	sc_core::sc_object					*m_obj;

};

}
