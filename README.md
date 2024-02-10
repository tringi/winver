# winver.com

Very simple Windows executable that outputs basic information about the Operating System.  
*This is how a proper `ver` command should work.*

## Usage

Pick a proper version from `bin` directory and place it on shared dive or thumb drive,
or any place you access from PC you wish to query.
Or include it in your deployment tools to have it on all your machines.

On my computer I see:

    C:\>Projects\winver\bin\x64\Release\winver.com
    
    Windows 10 Enterprise N 2016 LTSB [Version 1607 10.0.14393.6529] 64-bit

While regular `ver` shows just this bland:

    C:\>ver
    
    Microsoft Windows [Version 10.0.14393.6529]

## Other examples from my random VMs

by version 0.2 (see below what's new in latest version)

    Windows 11 Pro for Workstations Insider Preview [Version 23H2 10.0.26020.1000]
    Windows 11 Enterprise N [Version 21H2 10.0.22000.2652]
    Windows 10 Enterprise for Virtual Desktops [Version 22H2 10.0.19045.3803]
    Windows 10 IoT Enterprise LTSC [Version 21H2 10.0.19044.1288]
    Windows 10 Enterprise N 2015 LTSB [Version 10.0.10240.20345]
    Windows 8.1 Enterprise [Version 6.3.9600.20778]
    Windows 7 Enterprise [Version 6.1.7601.24546 Service Pack 1]
    Windows Vista (TM) Ultimate [Version 6.0.6002 Service Pack 2]
    Microsoft Windows XP [Version 5.1.2600 Service Pack 3]
    Windows Server 2022 Datacenter Evaluation [Version 21H2 10.0.20348.2159]
    Windows Server 2012 R2 Standard [Version 6.3.9600.21620]
    Windows Server (R) 2008 Datacenter without Hyper-V [Version 6.0.6003.20706 Service Pack 2]
    Microsoft Windows Server 2003 [Version 5.2.3790 Service Pack 2]
    Hyper-V Server 2016 [Version 1607 10.0.14393.6529]
    Azure Stack HCI [Version 23H2 10.0.25398.584]

## What's new in 0.3

* OS architecture added to display `32-bit`, `64-bit` or `ARM-64`
* UBR number is now also displayed on XP/Vista and Server 2003/2008

Command-line parameters added to display additional information:  
*must be stacked, e.g.: `-ob` not `-o -b`*

* `-a` displays ALL additional informations
* `-b` displays full build string, e.g.: `14393.6611.amd64fre.rs1_release.231218-1733`
* `-o` displays *Licensed to User/Organization* information, just like winver.exe
* `-u` displays uptime, e.g.: `Uptime: 1y 123d 23:01:59`

## Notes

* In console output, Release ID (23H2) is highlighted, and 10.0 is darker color (not significant anymore).
