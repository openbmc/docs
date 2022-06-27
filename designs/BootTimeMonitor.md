# BootTimeMonitor

Author: Michael Shen <gpgpgp@google.com>

Other contributors: Medicine Yeh <medicineyeh@google.com>

Created: June 25, 2022

## Problem Description

During a server reboot/power cycle process, measuring the duration spent on each
[stage](#definition-of-each-stage) is the key to profiling and monitoring the
software quality. However, some parts of the flow are not easy to be measured
and exposed to host user space applications. For example, it's difficult to
measure the time spent on Linux kernel shutting down because the file system
might be unavailable at the end of kernel reboot process.

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
    The implementation of how systemd obtains each boot time.
3.  [google-misc](https://github.com/openbmc/google-misc/blob/master/subprojects/metrics-ipmi-blobs/util.cpp#L280):
    An example of using these 5 boot up stages.

### Terminology

Some generic terms are used in this document.

| Term       | Definition                                                                                                                                |
| ---------- | ----------------------------------------------------------------------------------------------------------------------------------------- |
| S0_G3_S0   | Power off both motherboard and BMC then power on them both. Base on the DC-SCM spec, BMC should power on before release host reset signal |
| S0_S5_S0   | Power off motherboard only. BMC is still alive.                                                                                           |
| Stage      | A subflow within a full power cycle flow. (Each frame in    the "Power cycle flow" diagram below is a stage)                              |
| Unmeasured | The sum of all times that cannot be measured. e.g. The time that both BMC and host are powered off.                                       |

## Requirements

BootTimeMonitor should provide

-   Capability to determine where the current reboot stage is. (Either monitor
    services by the BMC itself or notified by IPMI command is fine)
-   Capability to compute the duration time for each stage.
-   Capability to export any additional duration that the user required to DBus.
    These additional durations can be consumed by Redfish or IPMI blob, depends
    on the needs.
-   A DBus interface to expose the boot time for each stage. So that other
    consumers can obtain boot time information from this service.
-   A DBus interface to expose power cycle related statistics such as
    -   the type of the power cycle (This can be leveraged from
        `Host.RestartCause`. See [Power cycle type](#Power-cycle-type) section).
    -   how many times of retry it has been through before successfully booting
        to the OS.
-   A DBus interface to receive notification and to add additional durations.
-   Capability to export the boot time to a blob format.
-   New IPMI OEM commands (for receiving the notification and adding additional
    durations)

This feature needs host side to send notifications to BMC informing BMC that the
current stage is changing but these host efforts won't be in the scope of this
doc.

## Proposed Design

```
            power cycle start                                                                         power cycle ends
                    |                                                                                         |
                    |                                                                                         |
                    |                                                                                         |
                    | Userspace | Kernel   | Post     |     |          |        |        |        |           |
Stage       Serving | Shutdown  | Shutdown | Shutdown | BMC | Firmware | Loader | Kernel | InitRD | Userspace | Serving
           ---------+-----------+----------+----------+-----+----------+--------+--------+--------+-----------+---------
                    |           |          |          |     |          |        |        |        |           |
                    |           |          |          |     |          |        |        |        |           |
                    |           |          |          |     |          |        |        |        |           |
                                |  T_kernel_down_end  | T_fw_start     |  T_loader_end                        |
                                |                     |                |                                      |
Timestamp                       |                     |                |                                      |
                                |                     |                |                                      |
                     T_user_down_end_halt      T_post_down_end      T_fw_end                              T_user_end
                     T_user_down_end_reboot    T_bmc_start/T_0
```

*In the following section, I will use `T_<timestamp>` to represent a certain
timestamp and use `D_<duration>` to represent a certain duration.

The main function for BootTimeMonitor is to report the duration for each power
cycle stage. \
All the stages are listed in the diagram above.

### Definition of each stage

| Stage              | Definition                                                                                                                                          |
| ------------------ | --------------------------------------------------------------------------------------------------------------------------------------------------- |
| Userspace Shutdown | Host is shutting down all the services/daemons and all the processes that are owned by user mode.                                                   |
| Kernel Shutdown    | Host is shutting down all the kernel processes and releasing all the resources that are owned by the kernel and firmware.                           |
| Post Shutdown      | It’s a period from host power-off to bmc power-off. Some components’ fw update will be triggered in this stage.                                     |
| BMC                | From BMC power on till BMC releases the host reset, including all the verification time.                                                            |
| Firmware           | Host firmware initialization stage. For most of the cases firmware is equal to BIOS.                                                                |
| Loader             | Host loader initialization stage. For example linuxboot/nerf.                                                                                       |
| Kernel             | Host kernel initialization stage.                                                                                                                   |
| InitRD             | Host initRD initialization stage. For example systemd.                                                                                              |
| Userspace          | Host userspace initialization stage.                                                                                                                |
| Unmeasured         | It is not shown on the diagram because it’s not a specific stage during a power cycle. It’s the sum of all the time frames that cannot be measured. |

#### Where is BMC shutdown?

“Post Shutdown” stage is a period from host power-off to bmc power-off. Ideally
bmc shutdown time is also included in this stage. While in some cases bmc may
start rebooting at the same time as the host. \
In this case, bmc may complete the shutdown before the host. And the bmc
shutdown time will be omitted and `D_post_down` will also be 0.

### How to obtain the duration of each stage?

BMC has 2 ways to obtain the durations.

1.  Computed through a formula with some timestamps and durations as input.
2.  Directly receiving the duration from the host.

The table below shows how we obtain the duration for each stage.

| Stage              | Duration name   | Source | Formula(if source is BMC)                       |
| ------------------ | --------------- | ------ | ----------------------------------------------- |
| Userspace Shutdown | `D_user_down`   | Host   |                                                 |
| Kernel Shutdown    | `D_kernel_down` | BMC    | `T_kernel_down_end` - `T_user_down_end`         |
| Post Shutdown*     | `D_post_down`   | BMC    | `T_post_down_end` - `T_kernel_down_end`         |
| BMC*               | `D_bmc`         | BMC    | `T_fw_start`                                    |
| Firmware           | `D_fw`          | BMC    | `T_fw_end` - `T_fw_start`                       |
| Loader             | `D_loader`      | Host   |                                                 |
| Kernel             | `D_kernel`      | Host   |                                                 |
| InitRD             | `D_initrd`      | Host   |                                                 |
| Userspace          | `D_user`        | Host   |                                                 |
| Entire power cycle | `D_total`       | Host   |                                                 |
| Unmeasured         | `D_unmeasured`  | BMC    | `D_total` - sum(all the durations listed above) |
| Extra**            | `D_extra`       | Host   |                                                 |

*If it's a S0_S5_S0 power cycle (which mean BMC doesn't reboot), then the
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

| Timestamp name                 | Symptom                                                                              |
| ------------------------------ | ------------------------------------------------------------------------------------ |
| `T_user_down_end<halt/reboot>` | Notified by host through IPMI                                                        |
| `T_kernel_down_end`            | DBus, `xyz.openbmc_project.State.Host.CurrentHostState "Running" -> "Off"`           |
| `T_post_down_end`              | A systemd service records the timestamp. with "After=final.target" argument          |
| `T_bmc_start/T_0`              | This value must be 0. It’s the base of timestamp                                     |
| `T_fw_start`                   | The first time that host status changes from "Off" -> "Running" within a power cycle |
| `T_fw_end`                     | Notified by host through IPMI                                                        |
| `T_loader_end`                 | Notified by host through IPMI                                                        |
| `T_user_end`                   | Notified by host through IPMI                                                        |

### What do the OEM commands look like?

This section shows the format of the new IPMI OEM commands. \
Google OEM group will be used in the example below.

| Command                                         | Bytes                                                                                                                                                             |
| ----------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Notification (To notify BMC to tag a timestamp) | <netfn+cmd+oem (5 bytes)> + <subcommand for notification (1 byte)> + <timestamp code (1 byte)>                                                                    |
|                                                 | e.g. 0x2e 0x32 0x79 0x2b 0x00 0x0f 0x00                                                                                                                           |
| Set Duration                                    | <netfn+cmd+oem (5 bytes)>* + <subcommand for set duration (1 byte)> + <length of duration name (1 byte)> + <duration name (n bytes)> + <duration in ms (8 bytes)> |
|                                                 | e.g. 0x2e 0x32 0x79 0x2b 0x00 0x10 0x04 0x44 0x48 0x43 0x50 0xa0 0xbb 0x0d 0x00 0x00 0x00 0x00 0x00 (Name = “DHCP”, Duration = 15 mins in this example)           |

### What statistics can we have and how?

#### Internal reboot count

The definition of `Internal reboot count` is the number of host status changing
from "Off" to "Running" within a power cycle. \
This value can be obtained by counting how many times the host status changed.

#### Power cycle type

In this design we have 3 different power cycle types.

-   S0_G3_S0
-   S0_S5_S0
-   Unknown

From the BMC perspective, we can leverage the `RestartCause` in the
[Host.interface](https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/State/Host.interface.yaml#L87)
to get the corresponding power type. \
Hence, we don't need a new DBus interface for power cycle type but a simple
`map<RestartCause, power cycle type>` would be enough.

### Unexpected power cycle

During a full power cycle, hypothetically, there are some possible combinations
and scenarios that might happen and cause server having another unexpected
S0_G3_S0/S0_S5_S0 power cycle. This section discussed what impact we would have
when an unexpected power cycle happens under different stages.

| Stage                                                           | Power cycle type    | Impact                                                                                                             |
| --------------------------------------------------------------- | ------------------- | ------------------------------------------------------------------------------------------------------------------ |
| Serving                                                         | S0_S5_S0            | `D_total` = 0* <br/> `D_unmeasured` = 0* <br/> `D_user_down` = 0 <br/> `D_kernel_down` = 0                         |
| Serving                                                         | S0_G3_S0            | `D_total` = 0* <br/> `D_unmeasured` = 0* <br/> `D_user_down` = 0 <br/> `D_kernel_down` = 0 <br/> `D_post_down` = 0 |
| Userspace shutdown                                              | S0_S5_S0            | `D_user_down` = 0 <br/> `D_kernel_down` = 0                                                                        |
| Userspace shutdown                                              | S0_G3_S0            | `D_user_down` = 0 <br/> `D_kernel_down` = 0 <br/> `D_post_down` = 0                                                |
| Kernel shutdown                                                 | S0_S5_S0            | None                                                                                                               |
| Kernel shutdown                                                 | S0_G3_S0            | `D_post_down` = 0                                                                                                  |
| Post shutdown                                                   | S0_S5_S0            | None                                                                                                               |
| Post shutdown                                                   | S0_G3_S0            | `D_post_down` = 0                                                                                                  |
| BMC                                                             | S0_S5_S0            | Impossible scenario. Because the host is power off at this stage.                                                  |
| BMC                                                             | S0_G3_S0            | None                                                                                                               |
| Firmware <br/> Loader <br/> Kernel <br/> InitRD <br/> Userspace | S0_G3_S0 & S0_S5_S0 | All the extra time spent will be added to `D_unmeasured`                                                           |

*If host can properly send the `D_total` even when unexpected power cycle
happens in the serving stage, then `D_total` & `D_unmeasured` will still be
correct.

## Alternatives Considered

### Can this be done with host notification only?

Sending the notification only will make BootTimeMonitor lost the capability of
receiving additional duration because the host must send the notification when
the event happens. While if we allow host to directly send a duration to BMC,
then host can defer the send until a proper time.

### Can this be done with host sending duration only?

It's not that easy for the host to know every stage duration. \
Especially some of the durations can be computed by BMC only. (e.g.
`D_unmeasured`)

### Can we merge this into **google-misc/metrics-ipmi-blobs**?

Not feasible. metrics-ipmi-blobs will be executed only when it is called. But
the BootTimeMonitor needs to monitor host status to get some timepoints. \
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
BootTimeMonitor. I think putting BootTimeMonitor into this repo might not be
appropriate.

## Impacts

None

### Organizational

Does this repository require a new repository? (Yes, No)

- Probably yes.

Who will be the initial maintainer(s) of this repository?

- TBD

## Testing

If the host has the corresponding supports (e.g. sending the IPMI command at
correct timepoint), then we can test this feature by triggering a reboot and
check if DBus has the correct result.

If the host hasn't supported this feature yet. We can send IPMI OEM command from
the BMC with a proper sequence to simulate host's behaviors. Then check if the
result is correct or not in the end.
