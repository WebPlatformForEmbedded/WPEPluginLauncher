#include "Launcher.h"
#include <inttypes.h>

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(Plugin::Launcher::mode)

    { Plugin::Launcher::mode::RELATIVE, _TXT("relative") },
    { Plugin::Launcher::mode::ABSOLUTE, _TXT("absolute") },
    { Plugin::Launcher::mode::ABSOLUTE_WITH_INTERVAL, _TXT("interval") },

    ENUM_CONVERSION_END(Plugin::Launcher::mode)
;
namespace Plugin {

    namespace {

        static Metadata<Launcher> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {}
        );
    }

/* static */ Launcher::ProcessObserver Launcher::_observer;

/* virtual */ const string Launcher::Initialize(PluginHost::IShell* service)
{
    Core::Time scheduleTime;
    Time interval;
    string message;
    Config config;

    ASSERT(_service == nullptr);
    ASSERT(_memory == nullptr);
    ASSERT(_activity.IsValid() == false);

    // Setup skip URL for right offset.
    _service = service;
    _deactivationInProgress = false;

    config.FromString(_service->ConfigLine());

    if ((config.Command.IsSet() == false) || (config.Command.Value().empty() == true)) {
        message = _T("Command is not set");
    }
    else if (ScheduleParameters(config, message, scheduleTime, interval) == true) {
        _memory = Core::Service<MemoryObserverImpl>::Create<Exchange::IMemory>(0);
        ASSERT(_memory != nullptr);

        _activity = Core::ProxyType<Job>::Create(&config, interval, _memory);
        ASSERT (_activity.IsValid() == true);

        // Well if we where able to parse the parameters (if needed) we are ready to start it..
        _observer.Register(&_notification);

        _activity->Schedule(scheduleTime);
    }

    return (message);
}

/* virtual */ void Launcher::Deinitialize(PluginHost::IShell* /* service */)
{
    ASSERT(_service == service);
    ASSERT(_memory != nullptr);
    ASSERT(_activity.IsValid() == true);

    _deactivationInProgress = true;

    _activity->Shutdown();
    _observer.Unregister(&_notification);
    _activity.Release();

    _memory->Release();
    _memory = nullptr;
    _service = nullptr;
}

/* virtual */ string Launcher::Information() const
{
    // No additional info to report.
    return (string());
}

void Launcher::Update(const ProcessObserver::Info& info)
{
    // There is always an Activity as the unregister takes place in a locked sequence. So no new notifications
    // Will enter after the unregister of this handler.
    ASSERT (_activity.IsValid() == true);
    ASSERT(_service != nullptr);

    // This can potentially be called on a socket thread, so the deactivation (wich in turn kills this object) must be done
    // on a seperate thread. Also make sure this call-stack can be unwound before we are totally destructed.
    if (_activity->IsActive() == true) {

        _activity->Update(info);

        if (_activity->IsActive() == false) {
            uint32_t result = _activity->ExitCode();

            if (result != Core::ERROR_NONE) {
                if (_deactivationInProgress == false) {
                    _deactivationInProgress = true;
                    SYSLOG(Trace::Fatal, (_T("FORCED Shutdown: %s by error: %d."), _service->Callsign().c_str(), result));
                    PluginHost::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
                }
            }
            else if (_activity->Continuous() == false) {
                if (_deactivationInProgress == false) {
                    _deactivationInProgress = true;
                    TRACE(Trace::Information, (_T("Launcher [%s] has run succesfully, deactivation requested."), _service->Callsign().c_str()));
                    PluginHost::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::AUTOMATIC));
                }
            }
            else {
                TRACE(Trace::Information, (_T("Launcher [%s] has run succesfully, scheduled for the next run."), _service->Callsign().c_str()));
            }
        }
    }
}

bool Launcher::ScheduleParameters(const Config& config, string& message, Core::Time& scheduleTime, Time& interval) {

    // initialize with defaults..
    scheduleTime = Core::Time::Now();
    interval = Time();

    // Only if a schedule section is set, we need to do something..
    if (config.ScheduleTime.IsSet() == true) {

        mode timeMode = config.ScheduleTime.Mode.Value();
        Time time(config.ScheduleTime.Time.Value());

        interval = Time(config.ScheduleTime.Interval.Value());

        if (time.IsValid() != true) {
            message = _T("Incorrect time format for Scheduled time.");
        }
        else if ( (config.ScheduleTime.Interval.IsSet() == true) && (interval.IsValid() != true) ) {
            message = _T("Incorrect time format for Interval time.");
        }
        else if ( (timeMode == ABSOLUTE_WITH_INTERVAL) && ((interval.IsValid() == false) || (interval.TimeInSeconds() == 0)) ) {
            message = _T("Requested mode is ABSOLUTE WITH INTERVAL but no interval (or 0 second interval) is given.");

        }
        else {
            // All signals green, we have valid input, calculate the ScheduleTime/Interval time
            if (timeMode == RELATIVE) { //Schedule Job at relative timing
                scheduleTime.Add(time.TimeInSeconds() * Time::MilliSecondsPerSecond);
            }
            else {
                Core::Time now (scheduleTime);
                // Go to a first viable start time (compared to the current time, in seconds)
                scheduleTime = Core::Time(now.Year(), now.Month(), now.Day(),
                                             (time.HasHours()   ? time.Hours()   : now.Hours()),
                                             (time.HasMinutes() ? time.Minutes() : now.Minutes()),
                                              time.Seconds(), 0, false);

                if (timeMode == ABSOLUTE_WITH_INTERVAL) {
                    uint32_t intervalJump = interval.TimeInSeconds() * Time::MilliSecondsPerSecond;

                    if (scheduleTime >= now) {
                        Core::Time workTime (scheduleTime);

                        while (workTime.Sub(intervalJump) > now) { scheduleTime.Sub(intervalJump); }
                    }
                    else {
                        // Now increment with the interval till we reach a valid point
                        while (scheduleTime < now) { scheduleTime.Add(intervalJump); }
                    }
                }
                else if (scheduleTime < now) {
                    uint32_t jump = (time.HasHours()   ? Time::HoursPerDay * Time::MinutesPerHour * Time::SecondsPerMinute :
                                    (time.HasMinutes() ? Time::MinutesPerHour * Time::SecondsPerMinute : Time::SecondsPerMinute));

                    scheduleTime.Add(jump * Time::MilliSecondsPerSecond);
                }
            }
        }
    }
    return (message.empty());
}
 
} //namespace Plugin

} // namespace WPEFramework
