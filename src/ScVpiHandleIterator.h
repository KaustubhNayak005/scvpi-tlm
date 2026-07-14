/*
 * ScVpiHandleIterator.h - VPI iterator over SystemC objects
 *
 * SCVPI-TLM: Unified SystemC TLM-2.0 & SystemVerilog RTL Co-Verification Framework
 * Created: 2026-07-14
 * Author: Kaustubh <nayakkaustubh@gmail.com>
 */

#pragma once
#include <stdint.h>
#include <vector>
#include "ScVpiHandle.h"
#include "ScVpiHandleObject.h"

namespace sc_core {
	class sc_object;
}

namespace scvpi {

class ScVpiHandleIterator : public ScVpiHandle {
public:
	ScVpiHandleIterator(
			const char *kind,
			const std::vector<sc_core::sc_object*> &items);

	virtual ~ScVpiHandleIterator();

	sc_core::sc_object  *next();

	static bool equals(const char *s1, const char *s2);

private:
	const char									*m_kind;
	const std::vector<sc_core::sc_object*>		&m_items;
	uint32_t									m_index;
	ScVpiHandleObject							m_item;
};

}
