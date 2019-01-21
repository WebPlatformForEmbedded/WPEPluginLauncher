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
    Time relativeTime;
    Time interval;
    string message;
    Config config;

    ASSERT(_service == nullptr);
    ASSERT(_memory == nullptr);

    // Setup skip URL for right offset.
    _service = service;

    config.FromString(_service->ConfigLine());

    if (config.ScheduleTime.IsSet() == true) {

        relativeTime = Time(config.ScheduleTime.RelativeTime.Value());
        if (relativeTime.IsValid() != true) {
            SYSLOG(Trace::Warning, (_T("Time format is wrong")));
        }

        interval = Time(config.ScheduleTime.Interval.Value());
        if (interval.IsValid() != true) {
            SYSLOG(Trace::Warning, (_T("Interval format is wrong")));
        }
    }

    _activity = Core::ProxyType<Job>::Create(&config, interval);
    if (_activity.IsValid() == true) {
        if (_activity->IsOperational() == false) {
            // Well if we where able to parse the parameters (if needed) we are ready to start it..
            _observer.Register(&_notification);

            if (relativeTime.IsValid() == true) {
                Core::Time scheduledTime(Core::Time::Now());
                uint64_t timeValueToTrigger = ((relativeTime.Hour() * 60 + relativeTime.Minute()) * 60 + relativeTime.Second()) * 1000;
                scheduledTime.Add(timeValueToTrigger);

                PluginHost::WorkerPool::Instance().Schedule(scheduledTime, _activity);
            }
            else {
                PluginHost::WorkerPool::Instance().Submit(_activity);
            }
        }
        else {
            message = _T("Could not parse the configuration for the job.");
        }
    }
    else {
        message = _T("Could not create the job.");
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

    if (_activity->Process().IsActive() == true) {
        // First try a gentle touch....
        _activity->Process().Kill(false);

        // Wait for a maximum of 3 Seconds before we shoot the process!!
        if (_activity->Process().WaitProcessCompleted(_closeTime * 1000) != Core::ERROR_NONE) {
            _activity->Process().Kill(true);
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
    if ((_activity.IsValid() == true) && (_activity->Pid() == info.Id())) {

        ASSERT(_service != nullptr);

        if (info.Event() == ProcessObserver::Info::EVENT_EXIT) {
        
            if ((info.ExitCode() & 0xFFFF) == 0) {
                // Only do this if we do not need a retrigger on an intervall.
                if (_activity->IsOperational() == false) {
                    PluginHost::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::AUTOMATIC));
                }
                else {
                    TRACE(Trace::Information, (_T("The process has run, and completed succefully.")));
                }
            }
            else {
                PluginHost::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
            }
        }
    }
}

} //namespace Plugin

} // namespace WPEFramework
