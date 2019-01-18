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
    Time time;
    Time interval;
    string message;
    Config config;

    ASSERT(_service == nullptr);
    ASSERT(_memory == nullptr);

    // Setup skip URL for right offset.
    _service = service;

    config.FromString(_service->ConfigLine());

    if (config.ScheduleTime.IsSet() == true) {

        time = Time(config.ScheduleTime.Time.Value());
        if (time.IsValid() != true) {
            SYSLOG(Trace::Warning, "Interval format is wrong");
        }

        interval = Time(config.ScheduleTime.Interval.Value());
        if (interval.IsValid() != true) {
            SYSLOG(Trace::Warning, "Interval format is wrong");
        }
        printf("%s:%s:%d %s %s\n", __FILE__, __func__, __LINE__, time.c_str(), interval.c_str());
    }

    _activity = Core::ProxyType<Job>::Create(&config, interval)

    if (_activity.IsValid() == true) {
        if (_activity->IsOperational() == true) {
            // Well if we where able to parse the parameters (if needed) we are ready to start it..
            _observer.Register(&_notification);

            if (_time.Valid() == true) {
                Workerpool::Instance().Schedule(scheduledTime, _activity);
            }
            else {
                Workerpool::Instance().Submit(_activity);
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
    if ((_activity.IsValid() == true) && (_activity->Process.Pid() == info.Id()) {

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
