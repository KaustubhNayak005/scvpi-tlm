#include "scvpi_tlm_bridge.h"

#include <stdexcept>
#include <utility>

static std::map<std::string, scvpi_tlm_btransport_fn>& scvpi_tlm_registry() {
    static std::map<std::string, scvpi_tlm_btransport_fn> registry;
    return registry;
}

void scvpi_register_tlm_endpoint(
    const std::string& name,
    scvpi_tlm_btransport_fn fn) {
    scvpi_tlm_registry()[name] = std::move(fn);
}

bool scvpi_has_tlm_endpoint(const std::string& name) {
    return scvpi_tlm_registry().find(name) != scvpi_tlm_registry().end();
}

scvpi_tlm_response scvpi_tlm_transport(
    const std::string& name,
    const scvpi_tlm_request& req) {
    auto it = scvpi_tlm_registry().find(name);
    if (it == scvpi_tlm_registry().end()) {
        throw std::runtime_error("Unknown TLM endpoint: " + name);
    }
    return it->second(req);
}