#include "PCH.h"
#include "Telekinesis.h"

static void SetupLogging()
{
    auto path = SKSE::log::log_directory();
    if (!path) SKSE::stl::report_and_fail("No SKSE log directory");
    *path /= "BetterTelekinesis.log";

    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
    auto log  = std::make_shared<spdlog::logger>("BT", std::move(sink));
    log->set_level(spdlog::level::trace);
    log->flush_on(spdlog::level::trace);
    spdlog::set_default_logger(std::move(log));
    spdlog::set_pattern("[%T] [%l] %v");
}

extern "C" [[maybe_unused]] __declspec(dllexport) bool SKSEPlugin_Load(
    const SKSE::LoadInterface* skse)
{
    SetupLogging();
    SKSE::Init(skse);
    logger::info("BetterTelekinesis loading (runtime {})",
                 REL::Module::get().version().string());

    SKSE::GetMessagingInterface()->RegisterListener(
        [](SKSE::MessagingInterface::Message* msg) {
            if (msg->type != SKSE::MessagingInterface::kDataLoaded) return;

            auto* spell = RE::TESForm::LookupByEditorID<RE::SpellItem>(
                "BetterTelekinesisSpell");
            if (spell)
                logger::info("Spell found: {:08X}", spell->GetFormID());
            else
                logger::warn("BetterTelekinesisSpell not found — is the ESP active?");

            Telekinesis::GetSingleton()->Initialize(spell);
        });

    return true;
}
