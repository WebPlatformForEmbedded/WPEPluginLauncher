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

SERVICE_REGISTRATION(Launcher, 1, 0);

/* static */ Launcher::ProcessObserver Launcher::_observer;

/* virtual */ const string Launcher::Initialize(PluginHost::IShell* service)
{
    Time time;
    Time interval;
    mode timeMode = RELATIVE;
    string message;
    Config config;

    ASSERT(_service == nullptr);
    ASSERT(_memory == nullptr);

    // Setup skip URL for right offset.
    _service = service;

    config.FromString(_service->ConfigLine());

    _closeTime = (config.CloseTime.Value());

    if (config.ScheduleTime.IsSet() == true) {

        timeMode = config.ScheduleTime.Mode.Value();

        time = Time(config.ScheduleTime.Time.Value());
        if (time.IsValid() != true) {
            SYSLOG(Trace::Fatal, (_T("Time format is wrong")));
        }

        interval = Time(config.ScheduleTime.Interval.Value());
        if (interval.IsValid() != true) {
            SYSLOG(Trace::Fatal, (_T("Interval format is wrong")));
        }
    }

    _memory = Core::Service<MemoryObserverImpl>::Create<Exchange::IMemory>(0);
    ASSERT(_memory != nullptr);

    _activity = Core::ProxyType<Job>::Create(&config, interval, _memory);
    if (_activity.IsValid() == true) {
        if (_activity->IsOperational() == false) {
            // Well if we where able to parse the parameters (if needed) we are ready to start it..
            _observer.Register(&_notification);
            Core::Time scheduledTime;
            if (time.IsValid() == true) {
                if (timeMode == RELATIVE) { //Schedule Job at relative timing
                    scheduledTime = Core::Time::Now();


                    uint64_t timeValueToTrigger = (((time.HasHours() ? time.Hours(): 0) * MinutesPerHour +
                                                    (time.HasMinutes() ? time.Minutes(): 0)) * SecondsPerMinute + time.Seconds()) * MilliSecondsPerSecond;
                    scheduledTime.Add(timeValueToTrigger);

                }
                else {
                    //Schedule Job at absolute timing
                    if (timeMode == ABSOLUTE_WITH_INTERVAL) {
                        scheduledTime = FindAbsoluteTimeForSchedule(time, interval);
                    }
                    else {
                        scheduledTime = FindAbsoluteTimeForSchedule(time, Time());
                    }
                }
                PluginHost::WorkerPool::Instance().Schedule(scheduledTime, _activity);
            }
            else {
                PluginHost::WorkerPool::Instance().Submit(_activity);
            }

        }
        else {
            _activity.Release();
            message = _T("Could not parse the configuration for the job.");
        }
    }
    else {
        message = _T("Could not create the job.");
    }

    if (_activity.IsValid() == false) {
        if (_memory != nullptr) {
            _memory->Release();
            _memory = nullptr;
        }
    }

    return (message);
}

/* virtual */ void Launcher::Deinitialize(PluginHost::IShell* service)
{
    ASSERT(_service == service);
    ASSERT(_memory != nullptr);

    PluginHost::WorkerPool::Instance().Revoke(_activity);

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
            _activity->Process().WaitProcessCompleted(_closeTime * 1000);
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

            _memory->Observe(0);
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

Core::Time Launcher::FindAbsoluteTimeForSchedule(const Time& absoluteTime, const Time& interval) {
    Core::Time startTime = Core::Time::Now();
    // Go to a first viable start time (compared to the current time, in seconds)
    Core::Time slotTime = Core::Time(startTime.Year(), startTime.Month(), startTime.Day(),
                                   (absoluteTime.HasHours()   ? absoluteTime.Hours() : startTime.Hours()),
                                   (absoluteTime.HasMinutes() ? absoluteTime.Minutes() : startTime.Minutes()),
                                   absoluteTime.Seconds(), 0, false);

    if (interval.IsValid() == false) {
        if (slotTime < startTime) {
            uint32_t jump ((absoluteTime.HasHours() ? HoursPerDay * MinutesPerHour * SecondsPerMinute :
                           (absoluteTime.HasMinutes() ? MinutesPerHour * SecondsPerMinute : SecondsPerMinute)) * MilliSecondsPerSecond);

            slotTime.Add(jump);
        }
    }
    else {
        uint32_t intervalJump = ( (interval.HasHours() ? interval.Hours() * MinutesPerHour * SecondsPerMinute : 0) +
                                  (interval.HasMinutes() ? interval.Minutes() * SecondsPerMinute : 0) +
                                   interval.Seconds() ) * MilliSecondsPerSecond;

        ASSERT (intervalJump != 0);
        if (slotTime >= startTime) {
            Core::Time workTime (slotTime);

            while (workTime.Sub(intervalJump) > startTime) {
                slotTime.Sub(intervalJump);
            }
        }
        else {
            // Now increment with the intervall till we reach a valid point
            while (slotTime < startTime) {
                slotTime.Add(intervalJump);
            }
        }
    }

    return slotTime;
}

} //namespace Plugin

} // namespace WPEFramework
