# winver.com

Very simple Windows executable that outputs basic information about the Operating System.  
*This is how a proper `ver` command should work.*

## Usage

Pick a proper version from `bin` directory and place it on shared dive or thumb drive,
or any place you access from PC you wish to query. 32-bit version will run almost everywhere,
except Server installations with WoW64 uninstalled.
Or include it in your deployment tools to have it on all your machines.

On my computer I see:

    C:\>Projects\winver\bin\x64\Release\winver.com
    
    Windows 10 Enterprise N 2016 LTSB [Version 1607 10.0.14393.6529] 64-bit

While regular `ver` shows just this bland:

    C:\>ver
    
    Microsoft Windows [Version 10.0.14393.6529]

## Other examples from my random VMs

    Windows 11 Pro for Workstations Insider Preview [Version 23H2 10.0.26020.1000] 64-bit
    Windows 11 Enterprise N [Version 21H2 10.0.22000.2652] 64-bit
    Windows 10 Enterprise for Virtual Desktops [Version 22H2 10.0.19045.3803] 64-bit
    Windows 10 IoT Enterprise LTSC [Version 21H2 10.0.19044.1288] 64-bit
    Windows 10 Enterprise N 2015 LTSB [Version 10.0.10240.20345] 32-bit
    Windows 8.1 Enterprise [Version 6.3.9600.20778] 32-bit
    Windows 7 Enterprise [Version 6.1.7601.24546 Service Pack 1] 32-bit
    Windows Vista (TM) Ultimate [Version 6.0.6002 Service Pack 2] 32-bit
    Microsoft Windows XP [Version 5.1.2600 Service Pack 3] 32-bit
    Microsoft Windows 2000 [Version 5.0.2195.6717 Service Pack 4] 32-bit
    Windows Server 2022 Datacenter Evaluation [Version 21H2 10.0.20348.2159] 64-bit
    Windows Server 2012 R2 Standard [Version 6.3.9600.21620] 64-bit
    Windows Server (R) 2008 Datacenter without Hyper-V [Version 6.0.6003.20706 Service Pack 2] 32-bit
    Hyper-V Server 2016 [Version 1607 10.0.14393.6529] 64-bit
    Azure Stack HCI [Version 23H2 10.0.25398.584] 64-bit

Command-line parameters added to display additional information:  
*may be passed both distinct `-l -o` or stacked `-lo`*

* `-a` displays ALL additional informations
* `-b` displays full build string, e.g.: `14393.6611.amd64fre.rs1_release.231218-1733`
* `-h` displays virtualization status and some hypervisor info
* `-s` displays Secure Kernel version and Secure Boot status
* `-l` displays expiration/licensing/activation status
* `-n` displays installed languages
* `-o` displays *Licensed to User/Organization* information, just like winver.exe
* `-i` displays supported architectures and whether they are native, emulated or WoW64
* `-u` displays uptime, e.g.: `Uptime: 1y 123d 23:01:59`

## Requirements

* Windows NT4

## Notes and limitations

* In console output, Release ID (23H2) is highlighted, and 10.0 is darker color (not significant anymore).
* Non-native builds on ARM64 may fail to report on virtualization properly
