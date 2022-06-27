# btmanager (boot time manager)

Author: Michael Shen <gpgpgp@google.com>

Other contributors: Medicine Yeh <medicineyeh@google.com>

Created: June 25, 2022

## Problem Description

The purpose of this design is to profile the time spent of each stage (the
definition of "stage" will be elaborated more in the following sections) in a
power cycle in a server. Then export the duration for each stage to a proper
interface. \
This data can be used for performance analysis, regression testing, ensuring
that each step is completed within the stipulated time, etc.

## Background and References

References:

1.  [5 boot up stages defined by systemd](https://www.freedesktop.org/wiki/Software/systemd/dbus/):
    These 5 stages listed below will be used in this design.
    -   Firmware
    -   Loader
    -   Kernel
    -   InitRD
    -   Userspace
2.  [systemd-analyze](https://github.com/systemd/systemd/blob/2f9b7186e3b2a98c2cc6135f2321b36121182a63/src/analyze/analyze-time-data.c#L20-L27)
    The implementation of how systemd obtain each boot time.
3.  [google-misc](https://github.com/openbmc/google-misc/blob/master/subprojects/metrics-ipmi-blobs/util.cpp#L280):
    An example of using these 5 boot up stages.

### Terminology

Some generic terms are used in this document.

| Term           | Definition                                                                                                   |
| -------------- | ------------------------------------------------------------------------------------------------------------ |
| AC power cycle | power off both motherboard and BMC then power on them both.                                                  |
| DC power cycle | power off motherboard only. BMC is still alive.                                                              |
| Stage          | A subflow within a full power cycle flow. (Each frame in    the "Power cycle flow" diagram below is a stage) |
| Unmeasured     | The sum of all times that cannot be measured. e.g. The time that both BMC and host are powered off.          |

## Requirements

btmanager should provide

-   Capability to determine where the current reboot stage is. (Either monitor
    services by the BMC itself or notified by IPMI command is fine)
-   Capability to compute the duration time for each stage.
-   Capability to export any additional duration that the user required.
-   A DBus interface to expose the boot time for each stage.
-   A DBus interface to expose power cycle related statistics such as the type
    of the power cycle (AC or DC).
-   A DBus interface to receive required commands.
-   Capability to export the boot time to a blob format.
-   New IPMI OEM commands (for receiving the notification and adding additional
    durations)

This feature needs host side to send notifications to BMC to inform BMC that the
current stage is changing but these host efforts won't be in scope of this doc.

## Proposed Design

```
            power cycle start                                                                         power cycle ends
                    |                                                                                         |
                    |                                                                                         |
                    |                                                                                         |
                    | Userspace | Kernel   | BMC      |     |          |        |        |        |           |
Stage       Serving | Shutdown  | Shutdown | Shutdown | BMC | Firmware | Loader | Kernel | InitRD | Userspace | Serving
           ---------+-----------+----------+----------+-----+----------+--------+--------+--------+-----------+---------
                    |           |          |          |     |          |        |        |        |           |
                    |           |          |          |     |          |        |        |        |           |
                    |           |          |          |     |          |        |        |        |           |
                                |  T_kernel_down_end  | T_fw_start     |  T_loader_end                        |
                                |                     |                |                                      |
Timestamp                       |                     |                |                                      |
                                |                     |                |                                      |
                     T_user_down_end_halt      T_bmc_down_end       T_fw_end                              T_user_end
                     T_user_down_end_reboot    T_bmc_start/T_0
```

*In the following section, I will use `T_<timestamp>` to represent a certain
timestamp and use `D_<duration>` to represent a certain duration.

The main function for btmanager is to report the duration for each power cycle
stage. \
All the stages are listed in the diagram above.

### Definition of each stage

| Stage              | Definition                                                                                                                                          |
| ------------------ | --------------------------------------------------------------------------------------------------------------------------------------------------- |
| Userspace Shutdown | Host is shutdowning all the services/daemons and all the processes that are owned by user mode.                                                     |
| Kernel Shutdown    | Host is shutdowning all the kernel processes and releasing all the resources that are owned by the kernel.                                          |
| BMC Shutdown       | It’s a period from host power-off to bmc power-off. Some components’ fw update will be triggered in this stage.                                     |
| BMC                | From BMC power on till BMC releases the host reset, including all the verification time.                                                            |
| Firmware           | Host firmware initialization stage. For most of the cases firmware is equal to BIOS                                                                 |
| Loader             | Host loader initialization stage. For example linuxboot/nerf                                                                                        |
| Kernel             | Host kernel initialization stage.                                                                                                                   |
| InitRD             | Host initRD initialization stage. For example systemd                                                                                               |
| Userspace          | Host userspace initialization stage.                                                                                                                |
| Unmeasured         | It doesn't show on the diagram because It’s not a specific stage during a power cycle. It’s the sum of all the time frames that cannot be measured. |

### How to obtain the duration of each stage?

BMC has 2 ways to obtain the durations.

1.  Computed through a formula with some timestamps and durations as input.
2.  Directly receiving the duration from the host.

The table below shows how we obtain the duration for each stage.

| Stage              | Duration name | Source | Formula(if source is BMC)                     |
| ------------------ | ------------- | ------ | --------------------------------------------- |
| Userspace Shutdown | D_user_down   | Host   |                                               |
| Kernel Shutdown    | D_kernel_down | BMC    | T_kernel_down_end - T_user_down_end           |
| BMC Shutdown*      | D_bmc_down    | BMC    | T_bmc_down_end - T_kernel_down_end            |
| BMC*               | D_bmc         | BMC    | T_fw_start                                    |
| Firmware           | D_fw          | BMC    | T_fw_end - T_fw_start                         |
| Loader             | D_loader      | Host   |                                               |
| Kernel             | D_kernel      | Host   |                                               |
| InitRD             | D_initrd      | Host   |                                               |
| Userspace          | D_user        | Host   |                                               |
| Entire power cycle | D_total       | Host   |                                               |
| Unmeasured         | D_unmeasured  | BMC    | D_total - sum(all the durations listed above) |
| Extra**            | D_extra       | Host   |                                               |

*If it's a DC power cycle(which mean BMC doesn't reboot), then the
`D_bmc_shutdow` and `D_bmc` will be 0. \
**Extra stores any additional durations that host want to expose. For example if
user uses nerf as the loader, they may want to detailly expose `D_nerf_kernel`
and `D_nerf_user` instead of just one `D_loader`. In this case users can send
the duration of `D_nerf_kernel` and `D_nerf_user` to BMC then ask BMC to expose.

### How to obtain the timestamps?

BMC has to know the exact time point for tagging a timestamp on it. So an
explicit symptom must appear when we tag the timestamp.

There are 2 different symptoms in current design, which are

-   Host state changing (from dbus)
-   The moment of receiving specific IPMI command from host (from IPMI)

The timestamps shown in the diagram and its corresponding symptoms are listed
below.

| Timestamp name                 | Symptom                                                                               |
| ------------------------------ | ------------------------------------------------------------------------------------- |
| `T_user_down_end<halt/reboot>` | Notified by host through IPMI                                                         |
| `T_kernel_down_end`            | DBus, `xyz.openbmc_project.State.Host.CurrentHostState "Running" -> "Off"`            |
| `T_bmc_down_end`               | A systemd service records the timestamp. with "After=final.target" argument           |
| `T_bmc_start/T_0`              | This value must be 0. It’s the base of timestamp                                      |
| `T_fw_start`                   | The first time that host status changes from "Off" -> "Running" within a power cycle. |
| `T_fw_end`                     | Notified by host through IPMI                                                         |
| `T_loader_end`                 | Notified by host through IPMI                                                         |
| `T_user_end`                   | Notified by host through IPMI                                                         |

### What statistics can we have and how?

#### Internal reboot count

The definition of `Internal reboot count` is the number of host status changing
from "Off" to "Running" within a power cycle. \
This value can be obtained by counting how many times the host status changed.

#### Power cycle type

If BMC experienced a power cycle as well then it's a AC power cycle. Otherwise
it's a DC power cycle.

## Alternatives Considered

### Can this be done with host notification only?

Sending the notification only will make btmanager lost the capability of
receiving additional duration because the host must send the notification when
the event happens. While if we allow host to directly send a duration to BMC,
then host can defer the send until a proper time.

### Can this be done with host sending duration only?

It's not that easy for the host to know every stage duration. \
Especially some of the durations can be computed by BMC only.(eg D_unmeasured)

### Can we merge this into **google-misc/metrics-ipmi-blobs**?

Not feasible. metrics-ipmi-blobs will be executed only when it is called. But
the btmanager needs to monitor host status to get some timepoints. \
So a daemon is a must have.

### Can we merge this into **phosphor-health-monitor**?

Probably not. phosphor-health-monitor is monitoring the BMC health, including
CPU/RAM/Storage utilization etc. \
Reporting host related information here might be against the purpose of this
repo.

### Can we merge this into **phosphor-watchdog**?

phosphor-watchdog monitors whether the host is still alive by receiving the
heartbeat signal (it can be a postcode or any other appropriate signal). If the
host stops sending heartbeat to BMC for a certain time. Then the watchdog will
trigger a predefined action.

From this point of view phosphor-watchdog serve a different purpose from
btmanager. I think putting btmanage into this repo might not be appropriate.

## Impacts

None

### Organizational

Does this repository require a new repository? (Yes, No)

- Probably yes.

Who will be the initial maintainer(s) of this repository?

- TBD

## Testing

If the host has the corresponding supports (eg: sending the IPMI command at
correct timepoint), then we can test this feature by triggering a reboot and
check if DBus has the correct result.

If the host hasn't supported this feature yet. We can send IPMI OEM command from
the BMC with a proper sequence to simulate host's behaviors. Then check if the
result is correct or not in the end.
