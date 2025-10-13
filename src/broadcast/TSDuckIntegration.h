#pragma once

#include <string>
#include <vector>

#include "ATSCCompliance.h"

namespace broadcast {

// Encapsulates integration with the TSDuck toolchain so that PSIP data can be
// generated or validated using the external utilities when they are available
// on the host system. The integration gracefully falls back to an internal
// serializer when TSDuck cannot be located.
class TSDuckIntegration {
public:
    struct ProgramService {
        std::string short_name;
        uint16_t major_channel_number;
        uint16_t minor_channel_number;
        uint16_t program_number;
        uint16_t source_id;
        uint16_t channel_tsid;
        uint8_t service_type;
        uint8_t etm_location;
        bool access_controlled;
        bool hidden;
        bool hide_guide;
        uint32_t carrier_frequency;
        uint8_t modulation_mode;
    };

    // Returns true when the TSDuck command line tools are available. The check
    // is lightweight and cached after the first probe.
    static bool is_available();

    // Build the canonical WMDV lineup for physical channel 16. The lineup
    // covers virtual channels 16.1 through 16.8 and is sourced from the latest
    // engineering parameters maintained for the station. Each entry can be fed
    // directly into the VirtualChannelTable helper functions.
    static std::vector<ProgramService> build_wmdv_program_lineup();

    // Convert a ProgramService entry into a PSIP virtual channel description.
    static VirtualChannelTable::Channel to_virtual_channel(
        const ProgramService& service);

    // Serialize a VirtualChannelTable either using TSDuck's table compiler or
    // the internal serializer when TSDuck is unavailable.
    static std::vector<uint8_t> serialize_vct(const VirtualChannelTable& table);

private:
    static bool probe_tsduck();
    static bool m_tsduck_available;
    static bool m_tsduck_probed;
};

} // namespace broadcast
