#include "Launcher.h"

namespace WPEFramework {
namespace Plugin {

SERVICE_REGISTRATION(Launcher, 1, 0);


/* virtual */ const string Launcher::Initialize(PluginHost::IShell* service)
{
    string message;
    Config config;

    ASSERT(_service == nullptr);

    // Setup skip URL for right offset.
    _service = service;

    
    config.FromString(_service->ConfigLine());
    Core::Process::Options options(config.Command.Value().c_str());
    auto iter = config.Parameters.Elements();

    return message;
}

/* virtual */ void Launcher::Deinitialize(PluginHost::IShell* service)
{
    ASSERT(_service == service);

    _service = nullptr;
}

/* virtual */ string Launcher::Information() const
{
    // No additional info to report.
    return (string());
}

void Launcher::Update(const ProcessObserver::Info& info)
{
    // This can potentially be called on a socket thread, so the deactivation (wich in turn kills this object) must be done
    // on a seperate thread. Also make sure this call-stack can be unwound before we are totally destructed.
    if (_process.Id() == info.Id()) {
        ASSERT(_service != nullptr);
        PluginHost::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
    }
}

} //namespace Plugin
} // namespace WPEFramework
