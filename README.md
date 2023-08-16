# RKFlashTool
rkflashtool is a tool for managing rockchip socs via the rockusb protocol.  
It was born as a fork of https://github.com/linux-rockchip/rkflashtool, with the aim of making the tool as user friendly as possible.

## !!! WARNING !!!
**I'm not a Rockchip employee, so this software is the result of personal studies and many tests.  
Rockchip boards are easily recoverable, but use at your own risk in any case.**

## Features
* install/upgrade bootloader
* full dump of internal memory
* flash raw image on internal memory
* read/write/erase partitions (for now only with rkparam method)
* read/write/erase LBA

## You need to know before start
Before you start, you need to know a couple of concepts

**MaskROM** It's a minimal firmware that is booted by the soc when it's unable to load a full bootloader. It's capable of loading a complete loader and is the mode required for flash/dump operations.

**DDRBIN** It's the part of the proprietary bootloader that takes care of configuring and activating the ddr memory.

**LOADER** It's the part of the proprietary bootloader that takes care of activating the internal memory and other components, and that takes care of managing the fastboot Google protocol

**USBPLUG** It's the part of the proprietary bootloader that takes care of activating the internal memory and other components, and that takes care of managing the rockusb protocol.

**RockUSB** Rockusb is the usb communication protocol implemented in most of the official rockchip firmwares.

rkflashtool is a cli implementation of the **RockUSB** protocol, so in order to work properly it needs the device to be in **USBPLUG** mode.

The generic workflow is as follows:
```
+---------------+   +---------------------+   +---------------------------+
| Enter MaskROM |-->| Load ddrbin/usbplug |-->| read/write flash, etc etc |
+---------------+   +---------------------+   +---------------------------+
```
To enter in MaskROM mode there are two ways:

* the so-called toothpick method (easy way)
* connect the pin clock of the internal storage to the ground (hard way)

### The toothpick method
On many boards, certainly those designed for Android boxes, there is a reset button. The position can vary, in the rk3228/rk3229 Android boxes, for example, it's hidden at the bottom of the A/V connector, in others it is visible, but the purpose is always the same.

In fact in Rockchip bootloaders pressing the reset button, with a toothpick, during startup allows
* enter in Android recovery mode if pressed after turning on the box (clearly if there is an Android firmware to recover)
* enter in **LOADER** mode if pressed before turning on the box

When in **LOADER** mode, you can enter **MaskROM** by issuing a particular reset command.

So, with this method, the workflow is as follows:
```
+------------------------|   +----------------+   +------------------------------+   +---------------------+   +-------------------+
| Use toothpick to press |-->| Power On Board |-->| Switch from LOADER to        |-->| Load ddrbin/usbplug |-->| read/write flash, |
| reset button           |   |                |   | MaskROM with reset command   |   |                     |   | etc etc           |
+------------------------+   +----------------+   +------------------------------+   +---------------------+   +-------------------+
```

### The pin clock method
TODO
## Tested with
* Rockchip 3128 android tv box
* Rockchip 322x android tv box

## Download prebuilt binaries
[* Windows 7/8/10 64bit](https://github.com/ilmich/rkflashtool/releases)

## Build from source
### Linux
```
$ git clone https://github.com/ilmich/rkflashtool/
$ make
```
### Windows with mingw w64 (WIP)
```
$ git clone https://github.com/ilmich/rkflashtool/
$ make CROSSPREFIX=x86_64-w64-mingw32- # for 32bit use i686-w64-mingw32-
```

## Getting started (WIP)
### rkflashtool
```
info: rkflashtool v5.94
fatal: usage:
        rkflashtool l file                      load DDRINIT & USBPLUG from packed rockchip bootloader (MASKROM MODE)
        rkflashtool a file                      install/update bootloader from packed rockchip bootloader
        rkflashtool b [flag]                    reboot device
        rkflashtool d > outfile                 dump full internal memory to image file
        rkflashtool e                           wipe flash
        rkflashtool e offset nsectors           erase flash (fill with 0xff)
        rkflashtool e partname                  erase partition (fill with 0xff)
        rkflashtool f file                      flash image file
        rkflashtool n                           read nand flash info
        rkflashtool p >file                     fetch parameters
        rkflashtool r partname >outfile         read flash partition
        rkflashtool r offset nsectors >outfile  read flash
        rkflashtool v                           read chip version
        rkflashtool w partname < infile         write flash partition
        rkflashtool P <file                     write parameters
```
### rkunpackfw
```
info: rkunpackfw v5.94
fatal: usage:
        rkunpackfw file                 unpack rockchip firmware
```
