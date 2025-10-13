#include "TSDuckIntegration.h"

#include <array>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "common/Logger.h"

namespace broadcast {

namespace {
constexpr uint32_t kDefaultCarrierFrequencyHz = 491000000; // RF channel 16
constexpr uint16_t kDefaultTransportStreamId = 0x16A0;      // Arbitrary but stable
constexpr uint8_t kDefaultModulationMode = 0x08;            // 8-VSB per ATSC A/65
constexpr uint8_t kDefaultETMLocation = 0x00;
constexpr uint8_t kATSCServiceTypeDTV = 0x02;

VirtualChannelTable::Channel make_channel_from_service(
    const TSDuckIntegration::ProgramService& service)
{
    VirtualChannelTable::Channel channel{};
    channel.short_name = service.short_name;
    channel.major_channel_number = service.major_channel_number;
    channel.minor_channel_number = service.minor_channel_number;
    channel.modulation_mode = service.modulation_mode;
    channel.carrier_frequency = service.carrier_frequency;
    channel.channel_TSID = service.channel_tsid;
    channel.program_number = service.program_number;
    channel.ETM_location = service.etm_location;
    channel.access_controlled = service.access_controlled;
    channel.hidden = service.hidden;
    channel.hide_guide = service.hide_guide;
    channel.service_type = service.service_type;
    channel.source_id = service.source_id;
    channel.descriptors_length = 0;
    return channel;
}

bool run_command(const std::string& command)
{
    std::array<char, 128> buffer{};
    FILE* pipe = popen(command.c_str(), "r");
    if (pipe == nullptr) {
        return false;
    }

    bool has_output = false;
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        has_output = true;
    }
    const int rc = pclose(pipe);
    return rc == 0 && has_output;
}

std::u16string ascii_to_utf16_upper(const std::string& input)
{
    std::u16string result;
    result.reserve(7);
    for (char c : input) {
        if (result.size() == 7) {
            break;
        }
        char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        result.push_back(static_cast<char16_t>(upper));
    }
    while (result.size() < 7) {
        result.push_back(u' ');
    }
    return result;
}

uint32_t mpeg_crc32(const std::vector<uint8_t>& data)
{
    uint32_t crc = 0xFFFFFFFF;
    for (uint8_t byte : data) {
        crc ^= static_cast<uint32_t>(byte) << 24;
        for (int i = 0; i < 8; ++i) {
            if (crc & 0x80000000) {
                crc = (crc << 1) ^ 0x04C11DB7;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

void append_uint16(std::vector<uint8_t>& buffer, uint16_t value)
{
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
}

void append_uint32(std::vector<uint8_t>& buffer, uint32_t value)
{
    buffer.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
}

} // namespace

bool TSDuckIntegration::m_tsduck_available = false;
bool TSDuckIntegration::m_tsduck_probed = false;

bool TSDuckIntegration::is_available()
{
    if (!m_tsduck_probed) {
        m_tsduck_available = probe_tsduck();
        m_tsduck_probed = true;
    }
    return m_tsduck_available;
}

bool TSDuckIntegration::probe_tsduck()
{
    // Attempt to locate either the tsp multiplexer or the tsduck inspector.
    // Using `tsp --version` is a lightweight check that does not require root
    // privileges. The method intentionally ignores stderr noise.
    if (run_command("tsp --version")) {
        LOG_INFO("TSDuck detected via tsp --version");
        return true;
    }
    if (run_command("tsduck --version")) {
        LOG_INFO("TSDuck detected via tsduck --version");
        return true;
    }
    LOG_WARN("TSDuck tools not detected; falling back to internal PSIP serializer");
    return false;
}

std::vector<TSDuckIntegration::ProgramService> TSDuckIntegration::build_wmdv_program_lineup()
{
    // Engineering data for WMDV-CD (Martinsville, VA). Each sub-channel retains
    // the same transport parameters but advertises a unique program and source
    // identifier so downstream PSIP consumers can discriminate between the
    // services.
    const std::array<std::string, 8> suffixes = {
        "WMDV16A", "WMDV16B", "WMDV16C", "WMDV16D",
        "WMDV16E", "WMDV16F", "WMDV16G", "WMDV16H"};

    std::vector<ProgramService> services;
    services.reserve(suffixes.size());

    uint16_t program_number = 0x1010;
    uint16_t source_id = 0x7010;

    for (size_t i = 0; i < suffixes.size(); ++i) {
        ProgramService svc{};
        svc.short_name = suffixes[i];
        svc.major_channel_number = 16;
        svc.minor_channel_number = static_cast<uint16_t>(i + 1);
        svc.program_number = static_cast<uint16_t>(program_number + i);
        svc.source_id = static_cast<uint16_t>(source_id + i);
        svc.channel_tsid = kDefaultTransportStreamId;
        svc.service_type = kATSCServiceTypeDTV;
        svc.etm_location = kDefaultETMLocation;
        svc.access_controlled = false;
        svc.hidden = false;
        svc.hide_guide = false;
        svc.carrier_frequency = kDefaultCarrierFrequencyHz;
        svc.modulation_mode = kDefaultModulationMode;
        services.push_back(svc);
    }

    return services;
}

VirtualChannelTable::Channel TSDuckIntegration::to_virtual_channel(
    const ProgramService& service)
{
    return make_channel_from_service(service);
}

std::vector<uint8_t> TSDuckIntegration::serialize_vct(const VirtualChannelTable& table)
{
    // When TSDuck is available we emit a table descriptor document and ask
    // tstabcomp to convert it into binary sections. Because the runtime
    // environment for this repository may not have TSDuck installed we keep a
    // robust fallback that can serialize the essential fields directly.
    if (!table.validate()) {
        LOG_ERROR("Virtual Channel Table validation failed prior to serialization");
        return {};
    }

    if (is_available()) {
        try {
            // Build an inline XML document describing the VCT. The document is
            // compatible with `tstabcomp --atsc`. We pipe it into the compiler
            // through a temporary file to keep the implementation dependency
            // free. If anything fails, we fall back to the internal serializer.
            const auto temp_dir = std::filesystem::temp_directory_path();
            const auto xml_path = temp_dir / "duckmuxpeg_vct.xml";
            const auto bin_path = temp_dir / "duckmuxpeg_vct.bin";

            std::ofstream xml(xml_path);
            if (!xml.is_open()) {
                throw std::runtime_error("Unable to create temporary VCT XML file");
            }

            xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
            xml << "<tables>\n";
            xml << "  <TVCT version=\"" << static_cast<int>(table.version_number)
                << "\" current=\"" << (table.current_next_indicator ? "true" : "false")
                << "\" transport_stream_id=\"" << table.transport_stream_id << "\""
                << " protocol_version=\"" << static_cast<int>(table.protocol_version)
                << "\">\n";

            for (const auto& channel : table.channels) {
                xml << "    <channel"
                    << " short_name=\"" << channel.short_name << "\""
                    << " major_channel_number=\"" << channel.major_channel_number << "\""
                    << " minor_channel_number=\"" << channel.minor_channel_number << "\""
                    << " modulation_mode=\"" << static_cast<int>(channel.modulation_mode) << "\""
                    << " carrier_frequency=\"" << channel.carrier_frequency << "\""
                    << " channel_TSID=\"" << channel.channel_TSID << "\""
                    << " program_number=\"" << channel.program_number << "\""
                    << " ETM_location=\"" << static_cast<int>(channel.ETM_location) << "\""
                    << " access_controlled=\"" << (channel.access_controlled ? "true" : "false") << "\""
                    << " hidden=\"" << (channel.hidden ? "true" : "false") << "\""
                    << " hide_guide=\"" << (channel.hide_guide ? "true" : "false") << "\""
                    << " service_type=\"" << static_cast<int>(channel.service_type) << "\""
                    << " source_id=\"" << channel.source_id << "\"";
                if (!channel.descriptors.empty()) {
                    xml << ">\n      <descriptors>";
                    // Hex encode the descriptor blob
                    std::ostringstream hex;
                    for (uint8_t byte : channel.descriptors) {
                        hex << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
                            << static_cast<int>(byte);
                    }
                    xml << hex.str() << "</descriptors>\n    </channel>\n";
                } else {
                    xml << " />\n";
                }
            }

            if (!table.additional_descriptors.empty()) {
                std::ostringstream hex;
                for (uint8_t byte : table.additional_descriptors) {
                    hex << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
                        << static_cast<int>(byte);
                }
                xml << "    <descriptors>" << hex.str() << "</descriptors>\n";
            }
            xml << "  </TVCT>\n";
            xml << "</tables>\n";
            xml.close();

            std::ostringstream cmd;
            cmd << "tstabcomp --atsc --binary " << bin_path << " " << xml_path;
            if (!run_command(cmd.str())) {
                throw std::runtime_error("tstabcomp execution failed");
            }

            std::ifstream bin(bin_path, std::ios::binary);
            if (!bin.is_open()) {
                throw std::runtime_error("Unable to read compiled VCT binary");
            }
            std::vector<uint8_t> payload((std::istreambuf_iterator<char>(bin)),
                                         std::istreambuf_iterator<char>());
            std::filesystem::remove(xml_path);
            std::filesystem::remove(bin_path);
            return payload;
        } catch (const std::exception& ex) {
            LOG_WARN(std::string("TSDuck serialization failed: ") + ex.what());
        }
    }

    // Manual serializer that emits a single-section terrestrial VCT. Only the
    // fields required by downstream multiplexers are populated.
    std::vector<uint8_t> payload;
    payload.reserve(1024);

    payload.push_back(static_cast<uint8_t>(table.table_id));
    // Placeholder for section length; fill later.
    payload.push_back(0); // will hold section_syntax_indicator + etc
    payload.push_back(0);

    append_uint16(payload, table.transport_stream_id);
    uint8_t version = static_cast<uint8_t>((table.version_number & 0x1F) << 1);
    uint8_t flags = version | (table.current_next_indicator ? 0x01 : 0x00);
    payload.push_back(0xC0 | flags);
    payload.push_back(table.section_number);
    payload.push_back(table.last_section_number);
    payload.push_back(table.protocol_version);
    payload.push_back(static_cast<uint8_t>(table.channels.size()));

    for (const auto& channel : table.channels) {
        const std::u16string short_name_utf16 = ascii_to_utf16_upper(channel.short_name);
        for (char16_t code_unit : short_name_utf16) {
            append_uint16(payload, static_cast<uint16_t>(code_unit));
        }

        const uint16_t major_minor = static_cast<uint16_t>((channel.major_channel_number & 0x3FF) << 6)
            | static_cast<uint16_t>((channel.minor_channel_number >> 4) & 0x3F);
        payload.push_back(static_cast<uint8_t>((major_minor >> 8) & 0xFF));
        payload.push_back(static_cast<uint8_t>(major_minor & 0xFF));

        const uint16_t minor_low = static_cast<uint16_t>((channel.minor_channel_number & 0x0F) << 12)
            | (static_cast<uint16_t>(channel.modulation_mode) << 4)
            | static_cast<uint16_t>((channel.carrier_frequency >> 28) & 0x0F);
        payload.push_back(static_cast<uint8_t>((minor_low >> 8) & 0xFF));
        payload.push_back(static_cast<uint8_t>(minor_low & 0xFF));

        append_uint32(payload, channel.carrier_frequency & 0x0FFFFFFF);
        append_uint16(payload, channel.channel_TSID);
        append_uint16(payload, channel.program_number);

        uint16_t status_bits = static_cast<uint16_t>((channel.ETM_location & 0x03) << 14);
        status_bits |= static_cast<uint16_t>((channel.access_controlled ? 1 : 0) << 13);
        status_bits |= static_cast<uint16_t>((channel.hidden ? 1 : 0) << 12);
        status_bits |= static_cast<uint16_t>((channel.hide_guide ? 1 : 0) << 11);
        status_bits |= static_cast<uint16_t>((channel.service_type & 0x3F) << 5);
        status_bits |= 0x001F; // source_id part high bits placeholder
        append_uint16(payload, status_bits);
        append_uint16(payload, channel.source_id);

        const uint16_t descriptor_length = static_cast<uint16_t>(channel.descriptors.size());
        payload.push_back(0xF0 | static_cast<uint8_t>((descriptor_length >> 8) & 0x0F));
        payload.push_back(static_cast<uint8_t>(descriptor_length & 0xFF));
        payload.insert(payload.end(), channel.descriptors.begin(), channel.descriptors.end());
    }

    const uint16_t additional_length = static_cast<uint16_t>(table.additional_descriptors.size());
    append_uint16(payload, static_cast<uint16_t>((0xF000) | additional_length));
    payload.insert(payload.end(), table.additional_descriptors.begin(), table.additional_descriptors.end());

    const size_t section_length = payload.size() + 4 - 3; // exclude table_id + section_length bytes
    payload[1] = static_cast<uint8_t>(0xB0 | ((section_length >> 8) & 0x0F));
    payload[2] = static_cast<uint8_t>(section_length & 0xFF);

    const uint32_t crc = mpeg_crc32(payload);
    append_uint32(payload, crc);
    return payload;
}

} // namespace broadcast
