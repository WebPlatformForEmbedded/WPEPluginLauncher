#include "Launcher.h"

namespace WPEFramework {

namespace Plugin {

SERVICE_REGISTRATION(Launcher, 1, 0);

    class MemoryObserverImpl : public Exchange::IMemory {
    private:
        MemoryObserverImpl();
        MemoryObserverImpl(const MemoryObserverImpl&);
        MemoryObserverImpl& operator=(const MemoryObserverImpl&);

    public:
        MemoryObserverImpl(const uint32_t id)
            : _main(id == 0 ? Core::ProcessInfo().Id() : id)
            , _observable(false)
        {
        }
        ~MemoryObserverImpl()
        {
        }

    public:
        virtual void Observe(const uint32_t pid)
        {
            if (pid == 0) {
                _observable = false;
             }
             else {
                _main = Core::ProcessInfo(pid);
                _observable = true;
             }
        }
        virtual uint64_t Resident() const
        {
            return (_observable == false ? 0 : _main.Resident());
        }
        virtual uint64_t Allocated() const
        {
            return (_observable == false ? 0 : _main.Allocated());
        }
        virtual uint64_t Shared() const
        {
            return (_observable == false ? 0 : _main.Shared());
        }
        virtual uint8_t Processes() const
        {
            return (IsOperational() ? 1 : 0);
        }
        virtual const bool IsOperational() const
        {
            return (_observable == false) || (_main.IsActive());
        }

        BEGIN_INTERFACE_MAP(MemoryObserverImpl)
        INTERFACE_ENTRY(Exchange::IMemory)
        END_INTERFACE_MAP

    private:
        Core::ProcessInfo _main;
        bool _observable;
    };


/* static */ Launcher::ProcessObserver Launcher::_observer;

/* virtual */ const string Launcher::Initialize(PluginHost::IShell* service)
{
    string message;
    Config config;

    ASSERT(_service == nullptr);
    ASSERT(_memory == nullptr);

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
    else {
        _memory = Core::Service<MemoryObserverImpl>::Create<Exchange::IMemory>(_pid);
    }

    return (message);
}

/* virtual */ void Launcher::Deinitialize(PluginHost::IShell* service)
{
    ASSERT(_service == service);
    ASSERT(_memory != nullptr);

    _observer.Unregister(&_notification);

    if (_memory != nullptr) {
        _memory->Release();
        _memory = nullptr;
    }

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
