#include "PCH.h"
#include "TelekinesisManager.h"
#include "InputHandler.h"
#include "DamageSystem.h"
#include "Settings.h"

// ─────────────────────────────────────────────────────────────────────────────
// SKSE plugin version metadata (required by SKSE64 / SKSEVR loader)
// ─────────────────────────────────────────────────────────────────────────────
extern "C" [[maybe_unused]] __declspec(dllexport) constinit auto SKSEPlugin_Version =
    []() noexcept {
        SKSE::PluginVersionData v{};
        v.PluginVersion(REL::Version{ PROJECT_VERSION_MAJOR,
                                      PROJECT_VERSION_MINOR,
                                      PROJECT_VERSION_PATCH, 0 });
        v.PluginName("BetterTelekinesis");
        v.AuthorName("NachCTE");
        v.UsesAddressLibrary();
        v.HasNoStructUse();
        return v;
    }();

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────
static void SetupLogging() {
    auto path = SKSE::log::log_directory();
    if (!path) {
        SKSE::stl::report_and_fail("Failed to get SKSE log directory");
    }
    *path /= "BetterTelekinesis.log";

    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
    sink->set_level(spdlog::level::trace);

    auto log = std::make_shared<spdlog::logger>("BetterTelekinesis", std::move(sink));
    log->set_level(spdlog::level::trace);
    log->flush_on(spdlog::level::warn);

    spdlog::set_default_logger(std::move(log));
    spdlog::set_pattern("[%Y-%m-%d %T.%e] [%l] %v");
}

static void OnDataLoaded() {
    Settings::GetSingleton()->Load();
    TelekinesisManager::GetSingleton()->Initialize();
    DamageSystem::GetSingleton(); // ensure singleton constructed
    logger::info("BetterTelekinesis data loaded — ready");
}

static void OnInputLoaded() {
    InputHandler::GetSingleton()->Install();
    logger::info("BetterTelekinesis input handler installed");
}

// ─────────────────────────────────────────────────────────────────────────────
// Plugin entry point
// ─────────────────────────────────────────────────────────────────────────────
extern "C" [[maybe_unused]] __declspec(dllexport) bool SKSEPlugin_Load(
    const SKSE::LoadInterface* a_skse) {
    SetupLogging();

    SKSE::Init(a_skse);

    const auto* plugin = SKSE::PluginDeclaration::GetSingleton();
    logger::info("BetterTelekinesis v{}.{}.{} loading (runtime: {})",
                 plugin->GetVersion()[0], plugin->GetVersion()[1], plugin->GetVersion()[2],
                 SKSE::GetRuntimeVersion().string());

    // Detect VR at runtime
    Settings::GetSingleton()->vrEnabled =
        (REL::Module::IsVR());
    logger::info("VR mode: {}", Settings::GetSingleton()->vrEnabled ? "yes" : "no");

    SKSE::GetMessagingInterface()->RegisterListener(
        [](SKSE::MessagingInterface::Message* msg) {
            switch (msg->type) {
            case SKSE::MessagingInterface::kDataLoaded:
                OnDataLoaded();
                break;
            case SKSE::MessagingInterface::kInputLoaded:
                OnInputLoaded();
                break;
            default:
                break;
            }
        });

    logger::info("BetterTelekinesis load complete");
    return true;
}
