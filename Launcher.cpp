#include "Launcher.h"

namespace WPEFramework {
namespace Plugin {

SERVICE_REGISTRATION(Launcher, 1, 0);

/* static */ Launcher::ProcessObserver Launcher::_observer;

/* virtual */ const string Launcher::Initialize(PluginHost::IShell* service)
{
    string message;
    Config config;

    ASSERT(_service == nullptr);

    // Setup skip URL for right offset.
    _service = service;

    config.FromString(_service->ConfigLine());

    _closeTime = (config.CloseTime.Value());
    Core::Process::Options options(config.Command.Value().c_str());
    auto iter = config.Parameters.Elements();

    while (iter.Next() == true) {
        const Config::Parameter& element(iter.Current());

        if ((element.Option.IsSet() == true) && (element.Option.Value().empty() == false)) {
            if ((element.Value.IsSet() == true) && (element.Value.Value().empty() == false)) {
                options.Set(element.Option.Value(), element.Value.Value());
            }
            else {
                options.Set(element.Option.Value());
            }
        }
    }

    _observer.Register(&_notification);

    // Well if we where able to parse the parameters (if needed) we are ready to start it..
    _process.Launch(options, &_pid);

    if (_pid == 0) {
        _observer.Unregister(&_notification);
        message = _T("Could not spawn the requested app/script [") + config.Command.Value() + ']';
    }

    return (message);
}

/* virtual */ void Launcher::Deinitialize(PluginHost::IShell* service)
{
    ASSERT(_service == service);

    _observer.Unregister(&_notification);

    if (_process.IsActive() == true) {
        // First try a gentle touch....
        _process.Kill(false);

        // Wait for a maximum of 3 Seconds before we shoot the process!!
        if (_process.WaitProcessCompleted(_closeTime * 1000) != Core::ERROR_NONE) {
            _process.Kill(true);
        }
    }

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
    if (_pid == info.Id()) {

        ASSERT(_service != nullptr);

        if (info.Event() == ProcessObserver::Info::EVENT_EXIT) {
        
            if ((info.ExitCode() & 0xFFFF) == 0) {
                PluginHost::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::AUTOMATIC));
            }
            else {
                PluginHost::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
            }
        }
    }
}

} //namespace Plugin
} // namespace WPEFramework
