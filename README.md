# ALTROM-SDK
This is a software development kit for ALTROM.\
It lets you write apps to be run from sdcard.
### WHAT THE SDK HAS
* A single header file called sdk.h that contains helpful functions.
* Useful examples to learn how to use it
* Chip-8 emulator cause why not!
### FOR THE DEVS
* There is no concept as privilege levels so all apps run at the highest authority.
* Since altrom is a single task operating system, only one app can be run at a time.
* Apps are required to have the ```watchdog_feed()``` function in any loop  (espcially while loops) to prevent unpleasant watchdog resets.
* Apps are currently loaded at the beginning of PSRAM i.e mapped to 0x00000000
* Build an app by copying app.ld,Makefile and start.s to the same folder and run \
```make NAME=<your app name>```
* If possible try to use
```make NAME=<your app name> LTO=1```
to drastically reduce the size.
### NOTES:
* nothing :) enjoy!
