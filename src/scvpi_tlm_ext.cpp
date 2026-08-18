#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "scvpi.h"
#include "scvpi_tlm_bridge.h"

#include <stdint.h>
#include <stdio.h>
#include <vector>

static bool pybytes_to_vec(PyObject* obj, std::vector<unsigned char>& out) {
    if (!obj || obj == Py_None) {
        out.clear();
        return true;
    }

    if (!PyBytes_Check(obj)) {
        return false;
    }

    char* buf = nullptr;
    Py_ssize_t len = 0;
    if (PyBytes_AsStringAndSize(obj, &buf, &len) != 0) {
        return false;
    }

    out.assign(reinterpret_cast<unsigned char*>(buf),
               reinterpret_cast<unsigned char*>(buf) + len);
    return true;
}

static PyObject* py_transport(PyObject* self, PyObject* args) {
    const char* endpoint = nullptr;
    int is_read = 0;
    unsigned long long addr = 0;
    PyObject* data_obj = nullptr;
    PyObject* be_obj = nullptr;

    (void)self;

    if (!PyArg_ParseTuple(args, "siKOO", &endpoint, &is_read, &addr, &data_obj, &be_obj)) {
        return nullptr;
    }

    std::vector<unsigned char> data;
    std::vector<unsigned char> be;

    if (!pybytes_to_vec(data_obj, data)) {
        PyErr_SetString(PyExc_TypeError, "data must be bytes or None");
        return nullptr;
    }

    if (!pybytes_to_vec(be_obj, be)) {
        PyErr_SetString(PyExc_TypeError, "byte_en must be bytes or None");
        return nullptr;
    }

    if (data.empty()) {
        PyErr_SetString(PyExc_ValueError, "data must not be empty");
        return nullptr;
    }

	scvpi_tlm_request req;
	req.is_read = (is_read != 0);
	req.addr = static_cast<uint64_t>(addr);
	req.data = data;
	req.byte_en = be;

	scvpi_tlm_response rsp;

	try {
	    rsp = scvpi_tlm_transport(endpoint, req);
	} catch (const std::exception& e) {
	    PyErr_SetString(PyExc_RuntimeError, e.what());
	    return nullptr;
	}

	return Py_BuildValue("y#iK",
		             reinterpret_cast<const char*>(rsp.data.data()),
		             static_cast<Py_ssize_t>(rsp.data.size()),
		             rsp.response_status,
		             (unsigned long long)rsp.delay_ps);
}

static PyMethodDef ScvpiTlmMethods[] = {
    {"transport", py_transport, METH_VARARGS, "Generic byte transport via SCVPI TLM bridge"},
    {nullptr, nullptr, 0, nullptr}
};

static struct PyModuleDef ScvpiTlmModule = {
    PyModuleDef_HEAD_INIT,
    "scvpi_tlm_ext",
    "SCVPI TLM Python bridge module",
    -1,
    ScvpiTlmMethods
};

PyMODINIT_FUNC PyInit_scvpi_tlm_ext(void) {
    return PyModule_Create(&ScvpiTlmModule);
}

extern "C" void scvpi_tlm_ext_entry() {
    int rc = PyImport_AppendInittab("scvpi_tlm_ext", &PyInit_scvpi_tlm_ext);
    fprintf(stdout, "scvpi_tlm_ext_entry called (AppendInittab rc=%d)\n", rc);
    fflush(stdout);
}