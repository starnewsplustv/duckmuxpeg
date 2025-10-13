#include <iostream>
#include <memory>
#include <signal.h>
#include <thread>
#include <chrono>
#include <vector>

#include "statmux/StatMuxEngine.h"
#include "common/Logger.h"
#include "common/Config.h"
#include "broadcast/ATSCCompliance.h"
#include "encoder/ModernMPEG2Encoder.h"
#include "obs-ui/BroadcastComplianceUI.h"

volatile bool g_running = true;

void signal_handler(int signum) {
    std::cout << "\nReceived signal " << signum << ", shutting down gracefully..." << std::endl;
    g_running = false;
}

int main(int argc, char* argv[]) {
    // Set up signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    try {
        // Initialize logger
        Logger::initialize("duckmuxpeg");
        Logger::info("Starting DuckMuxPeg Statistical Multiplexing Engine");

        // Load configuration
        Config config;
        bool dashboard_mode = false;
        std::string config_path;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--dashboard") {
                dashboard_mode = true;
            } else {
                config_path = arg;
            }
        }

        if (!config_path.empty()) {
            config.loadFromFile(config_path);
        } else {
            config.loadDefault();
        }
        
        // Create and initialize the statistical multiplexing engine
        auto statmux = std::make_unique<StatMuxEngine>(config);
        
        if (!statmux->initialize()) {
            Logger::error("Failed to initialize StatMux engine");
            return 1;
        }
        
        // Start the engine
        if (!statmux->start()) {
            Logger::error("Failed to start StatMux engine");
            return 1;
        }
        
        Logger::info("StatMux engine started successfully");

        if (dashboard_mode) {
            Logger::info("Rendering compliance dashboard");

            auto encoder = ModernMPEG2Encoder::create_for_broadcast();
            if (encoder) {
                encoder->initialize();
            }

            auto compliance = std::make_shared<ATSCCompliance>();
            compliance->set_video_format(VideoFormat::HDTV_1080i, AspectRatio::AR_16_9, 29.97);

            VirtualChannelConfig default_channel;
            default_channel.major_channel = 7;
            default_channel.minor_channel = 1;
            default_channel.short_name = "DUCKMPG";
            default_channel.service_name = "DuckMuxPeg";
            default_channel.program_number = 1;
            default_channel.source_id = 1;
            compliance->add_virtual_channel(default_channel);

            ProgramEvent showcase_event;
            showcase_event.event_id = 1001;
            showcase_event.title = "DuckMuxPeg Showcase";
            showcase_event.description = "Live demonstration of the DuckMuxPeg statistical multiplexing pipeline.";
            showcase_event.start_time = std::chrono::system_clock::now() + std::chrono::minutes(5);
            showcase_event.duration = std::chrono::minutes(30);
            compliance->add_event(default_channel.source_id, showcase_event);

            std::vector<uint8_t> synthetic_ts(188 * 5, 0);
            for (size_t i = 0; i < 5; ++i) {
                synthetic_ts[i * 188] = 0x47;
            }
            compliance->validate_transport_stream(synthetic_ts);

            BroadcastComplianceUI ui;
            ui.attach_encoder(std::move(encoder));
            ui.attach_compliance_checker(compliance);
            ui.set_compliance_callback([](const ComplianceReport& report) {
                if (!report.overall_compliant) {
                    Logger::warn("Compliance check reported issues");
                } else {
                    Logger::info("Compliance check passed without errors");
                }
            });

            auto report = ui.run_validation();
            if (!report.overall_compliant) {
                for (const auto& err : report.errors) {
                    Logger::warn("Compliance error: " + err);
                }
            }

            std::cout << "\n" << ui.render_dashboard() << std::endl;
        }

        // Main loop
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Check engine status
            if (!statmux->isRunning()) {
                Logger::warn("StatMux engine stopped unexpectedly");
                break;
            }
        }
        
        // Shutdown
        Logger::info("Shutting down StatMux engine");
        statmux->stop();
        
        Logger::info("DuckMuxPeg shutdown complete");
        
    } catch (const std::exception& e) {
        Logger::error("Fatal error: " + std::string(e.what()));
        return 1;
    }
    
    return 0;
}