#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

struct scvpi_tlm_request {
    bool is_read = false;
    uint64_t addr = 0;
    std::vector<unsigned char> data;
    std::vector<unsigned char> byte_en;
};

struct scvpi_tlm_response {
    int response_status = 0;
    std::vector<unsigned char> data;
    uint64_t delay_ps = 0;
};

using scvpi_tlm_btransport_fn =
    std::function<scvpi_tlm_response(const scvpi_tlm_request&)>;

void scvpi_register_tlm_endpoint(
    const std::string& name,
    scvpi_tlm_btransport_fn fn);

bool scvpi_has_tlm_endpoint(const std::string& name);

scvpi_tlm_response scvpi_tlm_transport(
    const std::string& name,
    const scvpi_tlm_request& req);