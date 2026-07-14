/*
 * ScVpiHandle.h - Common VPI handle base class
 *
 * SCVPI-TLM: Unified SystemC TLM-2.0 & SystemVerilog RTL Co-Verification Framework
 * Created: 2026-07-14
 * Author: Kaustubh <nayakkaustubh@gmail.com>
 */

#pragma once

namespace scvpi {

enum ScVpiHandleKind {
	HandleKind_Callback,
	HandleKind_Iterator,
	HandleKind_Object
};


class ScVpiHandle {
public:
	ScVpiHandle(ScVpiHandleKind kind);

	virtual ~ScVpiHandle();

	ScVpiHandleKind kind() const { return m_kind; }

private:
	ScVpiHandleKind				m_kind;

};

}

