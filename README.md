# WPEPluginLauncher

Plugin to "Launch" linux applications and scripts

## Quick start

### How to launch a script/application

1. Build launcher plugin. It create /usr/lib/wpeframework/plugins/libWPEFrameworkLauncher.so and /root/etc/WPEFramework/plugins/Launcher.json

   Generated Launcher.json contains,
   ```
   {
     "locator":"libWPEFrameworkLauncher.so",
     "classname":"Launcher",
     "precondition":[
       "Platform"
     ],
     "autostart":true
   }
   ```
 
2. Update Launcher.json with the command to be run
   
   E.g. to run command < `du -a /etc` >
   ```
   {
      "locator":"libWPEFrameworkLauncher.so",
      "classname":"Launcher",
      "precondition":[
        "Platform"
      ],
      "autostart":true,
      "configuration": {
        "command":"du"
      }
   }
   ```

3. Update json field:"paramters" with options/values if required

   E.g. to run < `du -a /etc -h` >
   ```
   {
      "locator":"libWPEFrameworkLauncher.so",
      "classname":"Launcher",
      "precondition":[
        "Platform"
      ],
      "autostart":true,
      "configuration": {
        "command":"du",
        "parameters": [
          { "option": "-a", "value": "/etc" },
          { "option": "-h"}
        ]
      }
   }
   ```

### How to schedule an application/script

1. Add schedule time information to the Launcher.json in the HH:MM.SS format (Hour:Minute.Second)
   ```
   "configuration": {
     "command":"du",
     "parameters": [
       { "option": "-a", "value": "/etc" },
       { "option": "-h"}
      ],
     "schedule": {
       "time": "06:04.10"
     }
   }
   ```
Note: If time value is "00:00.00"/invalid format/not set, the launcher will ignore the given time and launch the application at the launcher activation time itself.

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
        "time": "06:04.10",
        "interval": "00:40.10"
      }
   }
   ```

Note: If interval value is "00:00.00"/invalid format/not set, the launcher will treat it as invalid value and ignore the interval time settings.

# How to launch multiple scripts/applcations

E.g.
``` testapp1, testapp2 and testapp3 are the three applications to be run ```

1. Create testapp1.json by copying Launcher.json (cp Launcher.json testapp1.json) and update with proper commands and option/values

2. Create testapp2.json by copying Launcher.json (cp Launcher.json testapp2.json) and update with proper commands and option/values

3. Create testapp3.json by copying Launcher.json (cp Launcher.json testapp3.json) and update with proper commands and options/values

4. Run WPEFramework

