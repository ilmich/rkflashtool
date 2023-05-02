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

**RockUSB**
Rockusb is the usb communication protocol implemented in most of the official rockchip firmwares.

**MaskROM**
It's a minimal firmware that is booted by the soc when it's unable to load a full bootloader. It's capable of loading a complete loader and is the mode required for flash/dump operations.

**DDRBIN**
It's the part of the proprietary bootloader that takes care of configuring and activating the ddr memory.

**USBPLUG**
It's the part of the proprietary bootloader that takes care of activating the internal memory and other components, and that takes care of managing the rockusb protocol.

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
```
$ ./rkflashtool 
info: rkflashtool v5.92
fatal: usage:
        rkflashtool a file                      install/update bootloader
        rkflashtool b [flag]                    reboot device
        rkflashtool d > outfile                 dump full internal memory to image file
        rkflashtool e                           wipe flash
        rkflashtool e offset nsectors           erase flash (fill with 0xff)
        rkflashtool e partname                  erase partition (fill with 0xff)
        rkflashtool f file                      flash image file
        rkflashtool l file                      load DDRINIT & USBPLUG from packed rockchip bootloader (MASK ROM MODE)
        rkflashtool n                           read nand flash info
        rkflashtool p >file                     fetch parameters
        rkflashtool r partname >outfile         read flash partition
        rkflashtool r offset nsectors >outfile  read flash
        rkflashtool v                           read chip version
        rkflashtool w partname < infile         write flash partition
        rkflashtool P <file                     write parameters
```
##
