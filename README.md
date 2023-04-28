# Intro
rkflashtool is a tool for managing rockchip socs via the rockusb protocol. It was born as a fork of https://github.com/linux-rockchip/rkflashtool, with the aim of making the tool as user friendly as possible.

**WARNING**  
**This software is the result of many experiments and studies, and is to be considered unofficial. Use it consciously.**

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
info: rkflashtool v5.90
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

```
##
