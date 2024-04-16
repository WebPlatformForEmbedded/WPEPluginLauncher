# WPEPluginLauncher

Plugin to "Launch" linux applications and scripts

## Quick start

### How to launch a script/application

1. Build launcher plugin. It create /usr/lib/thunder/plugins/libThunderLauncher.so and /root/etc/Thunder/plugins/Launcher.json

   Generated Launcher.json contains,
   ```
   {
     "locator":"libThunderLauncher.so",
     "classname":"Launcher",
     "precondition":[
       "Platform"
     ],
     "startmode":"Activated"
   }
   ```
 
2. Update Launcher.json with the command to be run
   
   E.g. to run command < `du -a /etc` >
   ```
   {
      "locator":"libThunderLauncher.so",
      "classname":"Launcher",
      "precondition":[
        "Platform"
      ],
      "startmode":"Activated",
      "configuration": {
        "command":"du"
      }
   }
   ```

3. Update json field:"paramters" with options/values if required

   E.g. to run < `du -a /etc -h` >
   ```
   {
      "locator":"libThunderLauncher.so",
      "classname":"Launcher",
      "precondition":[
        "Platform"
      ],
      "startmode":"Activated",
      "configuration": {
        "command":"du",
        "parameters": [
          { "option": "-a", "value": "/etc" },
          { "option": "-h"}
        ]
      }
   }
   ```

### How to schedule an application/script with relative time

1. Add 'relative' time information to the Launcher.json in the HH:MM.SS format (Hour:Minute.Second)
   ```
   "configuration": {
     "command":"du",
     "parameters": [
       { "option": "-a", "value": "/etc" },
       { "option": "-h"}
      ],
     "schedule": {
       "mode": "relative",
       "time": "06:04.10"
     }
   }
   ```

Note:
1. If field "mode" is empty or not set, it will treat the time as relative
2. If relative time value is "00:00.00"/not set, the launcher will ignore the given time and launch the application at the launcher activation time itself.
3. If time format given is
  a. "XX", treat it as SS
  b. "XX.XX" treat it as MM.SS

### How to schedule an application/script with absolute time

1. Add 'absolute' time information to the Launcher.json in the HH:MM.SS format (Hour:Minute.Second)
   ```
   "configuration": {
     "command":"du",
     "parameters": [
       { "option": "-a", "value": "/etc" },
       { "option": "-h"}
      ],
     "schedule": {
       "mode": "absolute",
       "time": "06:04.10"
     }
   }
   ```

Note:
1. If absolute time value is not set, the launcher will ignore the given time and launch the application at the launcher activation time itself.
2. If time format given is
   a. "XX", treat it as SS, and schedule the application launch at next SSth time. i.e. if absolute time given is 25 and current time is 08:10:45, then the application will be
      launched at 08:11:25
   b. "XX.XX" treat it as MM.SS, and scedule the application launch at next MM:SSth time. i.e. if absolute time given is 30.20 and current time is 08:10:45, then the application will be
      launched at 09:30:20
   c. "00:00.00" treat it as midnight.
3. If absolute time given is less than the current time, it will launch the application the same time at the subsequent day. i.e, if absolute time given is 13:00:00, it will launch
   the application at same time at the next day.

### How to schedule an application/script to run in an interval

1. Add interval time information to the Launcher.json in the HH:MM.SS format (Hour:Minute.Second)
   ```
   "configuration": {
     "command":"du",
     "parameters": [
       { "option": "-a", "value": "/etc" },
       { "option": "-h"}
      ],
      "schedule": {
        "mode": "interval",
        "time": "06:04.10",
        "interval": "00:40.10"
      }
   }
   ```

Note:
1. If interval value is "00:00.00"/invalid format/not set, the launcher will treat it as invalid value and ignore the interval time settings.
2. If mode is interval (absolute with interval) and interval is set, it will identify next matching time and schedule application launch to that time.
   i.e, if the absolute time given is 04:00:00, current time is 05:10:00 and interval is 00:30:00, then next scheduling time will be 05:30:00 (will be identified from the next intervals - 04:30:00, 05:00:00, 05:30:00)
3. If mode is relative or absolute, the interval time will be taken only for the subsequent scheduling

### How to set wait time for the process to complete properly during the deactivation.
  add closetime parameter into the json with the average closing time for the script or application. This will wait till that configured time for a clean exit of process/script.

   ```
   {
      "locator":"libThunderLauncher.so",
      "classname":"Launcher",
      "precondition":[
        "Platform"
      ],
      "startmode":"Activated",
      "configuration": {
        "command":"du",
        "closetime":5
      }
   }
   ```
### How to launch multiple scripts/applcations

E.g.
``` testapp1, testapp2 and testapp3 are the three applications to be run ```

1. Create testapp1.json by copying Launcher.json (cp Launcher.json testapp1.json) and update with proper commands and option/values

2. Create testapp2.json by copying Launcher.json (cp Launcher.json testapp2.json) and update with proper commands and option/values

3. Create testapp3.json by copying Launcher.json (cp Launcher.json testapp3.json) and update with proper commands and options/values

4. Run Thunder

