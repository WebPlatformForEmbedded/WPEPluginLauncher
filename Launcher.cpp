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
    _options.Set(config.Command.Value().c_str());
    auto iter = config.Parameters.Elements();

    while (iter.Next() == true) {
        const Config::Parameter& element(iter.Current());

        if ((element.Option.IsSet() == true) && (element.Option.Value().empty() == false)) {
            if ((element.Value.IsSet() == true) && (element.Value.Value().empty() == false)) {
                _options.Set(element.Option.Value(), element.Value.Value());
            }
            else {
                _options.Set(element.Option.Value());
            }
        }
    }
    printf("%s:%s:%d \n", __FILE__, __func__, __LINE__);
    if (config.ScheduleTime.IsSet() == true) {

        string time(config.ScheduleTime.Time.Value());
        if (time.empty() == false) {
            if (_time.Parse(time) != true) {
                TRACE_L1("Time format is wrong");
            }
        }

        string interval(config.ScheduleTime.Interval.Value());
        if (interval.empty() == false) {
            if (_interval.Parse(interval) != true) {
                TRACE_L1("Interval format is wrong");
            }
        }
        printf("%s:%s:%d %s %s\n", __FILE__, __func__, __LINE__, time.c_str(), interval.c_str());
    }

    _observer.Register(&_notification);

    // Well if we where able to parse the parameters (if needed) we are ready to start it..
    bool status = LaunchJob(_time);
    if (status == false) {
        _observer.Unregister(&_notification);
        message = _T("Could not spawn the requested app/script [") + config.Command.Value() + ']';
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

bool Launcher::ScheduleJob(Time time)
{
    Core::Time scheduledTime(Core::Time::Now());
    uint64_t timeValueToTrigger = ((time.Hour() * 60 + time.Minute()) * 60 + time.Second()) * 1000;

    scheduledTime.Add(timeValueToTrigger);
    PluginHost::WorkerPool::Instance().Schedule(scheduledTime, _activity);
}

bool Launcher::LaunchJob(Time time)
{
    bool status = true;
    if (time.Hour() == 0 && time.Minute() == 0 && time.Second() == 0) {
        _process.Launch(_options, &_pid);

        if (_pid == 0) {
            _observer.Unregister(&_notification);
            status = false;
        }
        else {
            _memory = Core::Service<MemoryObserverImpl>::Create<Exchange::IMemory>(_pid);
            ScheduleJob(_interval);
        }
    }
    else {
       ScheduleJob(_time);
    }
    return status;
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
