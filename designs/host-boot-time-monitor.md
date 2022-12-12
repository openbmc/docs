# HostRebootTimeMonitor

Author: Michael Shen <gpgpgp@google.com>

Other contributors: Medicine Yeh <medicineyeh@google.com>

Created: December 12, 2022

## Problem Description

In the server area boot time is one of the key factors of the performance.
Having a detailed reboot time measurement provides us a clear data of the time
cost of each reboot [stage](#definition-of-each-stage). Although the host can
collect the boot time breakdown with systemd, the shutdown time for both
user services and OS kernel are still missing. Especially, the later one is
impossible to be measured by the host itself. In addition, when a reboot or
power cycle happens during a reboot flow, there is no way to recover the exact
reboot duration on each stage because the current standard does not have the
context of a reboot flow.

This design provides an out-of-band solution to monitor both hardware and
software events from the hosts and breakdown the boot time by its stages. Also,
the proposed monitoring service can record the exact events happened during the
whole reboot flow and provide this information for triaging the root cause of a
long reboot time.

## Requirements

HostRebootTimeMonitor should

1.  Breakdown the reboot flow into individual **stages**. A reboot flow starts
    from the end of serving state and ends when the OS enters any of the
    interaction mode (serving state), e.g. multi-user mode and graphical mode.
2.  Each stage should be seamless and the summation of stage durations should
    be the wall clock total reboot duration.
3.  Report the real reboot flow with its timestamp and durations even there
    are extra reboot/power cycle events happened in the whole reboot flow.
    There are some example cases in the [following section]
    (#reboot-flow-examples).
4.  Have a special ToS5 stage to record the time transition from the last
    shutdown stage to the S5 state. Typically, this represents the duration of
    the last mile like kernel shutdown time which cannot be self-measured
    precisely at the boundary.
5.  Allow users to store the actual shutdown logic in the reboot flow. A
    shutdown logic can be either halt, poweroff, reboot, or kexec.
6.  Allow users to save extra (arbitrary) durations so that a user can monitor
    any durations they are interested in.
7.  Last host reboot information needs to be persisted until the next host
    reboot flow starts.
8.  Support multi-host, i.e. each host should have its own data.

## Proposed Design

Here is an example of a standard reboot flow that demonstrates the concepts. The
design relies on both host (hardware) events and IPMI commands sent from the
hosts to facilitate the reboot time breakdown and monitoring in a reboot flow.

```
            reboot flow start                                                          reboot flow ends
                    |                                                                          |
                    |                                                                          |
                    |                                                                          |
                    | Userspace |      |     |          |        |        |        |           |
Stage       Serving | Shutdown  | ToS5 | Off | Firmware | Loader | Kernel | InitRD | Userspace | Serving
           ---------+------------------+-----+----------+--------+--------+--------+-----------+----------
                    |           |      |     |          |        |        |        |           |
                    |           |      |     |          |        |        |        |           |
                    |           |      |     |          |        |        |        |           |
                    |  T_last_down_end |    T_S0        |  T_loader_end   |   T_initrd_end     |
                    |                  |                |                 |                    |
Timestamp           |                  |                |                 |                    |
                    |                  |                |                 |                    |
              T_serving_end           T_S5            T_fw_end        T_kernel_end          T_user_end
                                                                                            T_serving_start

```

*   `T_<timestamp>` represent the timestamp of a [notification command](#ipmi-oem-commands).

### Reboot flow

Firstly, one of the most important concept is that there is a definition of
start and end of a reboot flow. These two events are required in any reboot
flow. If any is missing, the data is undefined.
*   start event: `T_serving_end`, `T_S5` and `T_S0` are all the trigger of start
    becasue a reboot might be triggered by any uncleaned power cycle events,
    e.g. `T_S5` is for host power cycle and `T_S0` is for the whole machine.
*   end event: `T_serving_start` is the only end condition to handle cases
    that the end condition is different.

After a `start` event is triggered, the monitoring service will enter the
monitoring mode and record all the collected events until reaching
the `end` event. Note that, in monitoring mode, the monitoring service
ignores all `start` events.

### Duration of each stage

The reboot time breakdown problem can be generalized to counting the duration
spent on each state in a state machine. For example, here is a state transition
with correlated duration denoted by its name. The duration in state A is 5s,
state B is 10s, and state C is 16s.

```
Start --> A(1s) -(0.1s)-> B(2s) -(0.1s)-> A(4s) -(0.1s)-> B(8s) -(0.1s)-> C(16s) --> End
```

However, the state transition might not be free. In the example above, the
state transition time of A is `0.1s` while B is `0.1s+0.1s=0.2s`.

To build up the stage transition flow as above, the service record all the
states in a reboot flow and calculate the duration and transition time by
*   case 1: If the duration of B is given, denoted as `D_B`, that duration will
    used directly. In addition, HostRebootTimeMonitor will generate a new stage
    `ToB` to record the duration of the transition time as
    `(T_end_of_B - T_end_of_A) - D_B`
*   case 2: If the duration of B is 0, i.e. undefined, duration of B is
    `T_end_of_B - T_end_of_A`. In this case, we assume the transition time is
    ignorable and do not record it.

There are some alternatives but non of them can provide the same feature:
*   host reports begin/end pair of each stage:
    This does not represent the actual transition time because not all the
    stage can send IPMI commands at the very beginning and end of the code.
    For example, the at the end of the userspace, there is no file system
    available but there might still be some services need to be turned off.
    Another example is that early part of BIOS code is controlled by the CPU
    vendor and there is no source code. There is no way to send IPMI commands.
*   host just sends only end notification without durations:
    This is impossible to measure the stage transition time.

### Special cases of reboot duration

#### ToS5

During a reboot flow, there are shutdown process and boot up process.
These two are very similar but different because of requirement 4.

To calculate the duration of `ToS5` stage, the IPMI command has a flag to
indicate whether this is a notification during shutdown or boot up. The
duration of `ToS5` is defined as:

```
T_S5_state - T_last_notification_command
```

#### Off

Off is another special stage to calculate how long the host has been put off.
This is defined by two host hardware events instead of IPMI commands.
```
T_S0_state - T_S5_state
```

#### Unknown stage

After an end notification is received, there is no indication on which stage
is current stage. If an unexpected reboot happens and the host starts over
from the firmware stage, we define the current one as unknown stage.

Here is one example flow:
```
T_end_of_A -> Unknown -> T_S5
```

The design relies on the users to recover the real stage name of `Unknown` by
its adjacent stages. Users can also interpret it as an warning that there is
something wrong happened on the machine.

Alternatives:
*   Adding a begin notification in each stage is a possible solution. However,
    some components are impossible to send IPMI commands at the beginning of
    the codes, like BIOS. If an unexpected reboot happens before the begin
    notification, that duration is still misclassified.

### Store custom durations

All the durations reported from set duration commands will be stored and
reported. HostBootTimeMonitor does not use it for any other purposes.


## Reboot flow examples
-   Tray power-cycle process:
    `S5->Off->S0->Firmware->Loader->Kernel->InitRD->Userspace->Serving`
-   Non-graceful reboot process:
    `Serving->Off->Firmware->Loader->Kernel->InitRD->Userspace->Serving`
-   Graceful reboot process: `Serving->Userspace
    Shutdown->ToS5->Off->S0->Firmware->Loader->Kernel->InitRD->Userspace->Serving`
-   An unexpected reboot happens during the boot process: `Serving->Userspace
    Shutdown->ToS5->Off->S0->Unknown->ToS5->Off->S0->Firmware->Loader->Kernel->InitRD->Userspace->Serving`
-   Kexec reboot:
    `Serving->Userspace Shutdown->Kernel->InitRD->Userspace->Serving`
-   Kernel crash happened during a normal reboot:
    `Serving->Userspace Shutdown->Kdump->ToS5->Off->S0->Firmware->Loader->Kernel->InitRD->Userspace->Serving`

### IPMI OEM commands

Notify command uses free format names instead of an enum for flexibility of
adding new stage in the reboot flow.

| Command                                   | Bytes                                                                                                                                                             |
| ----------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Notify (To notify BMC to tag a timestamp) | <netfn+cmd+oem (5 bytes)>* + <subcommand for notification (1 byte)> + <flag for this notification (1 byte)> + <duration in ms (8 bytes)> + <length of duration name (1 byte)> + <duration name (n bytes)> |
|                                           | e.g. 0x2e 0x32 0x79 0x2b 0x00 0x20 0x00 0xa0 0xbb 0x0d 0x00 0x00 0x00 0x00 0x00 0x04 0x55 0x73 0x65 0x72 (Flag = 1 (boot flow) Duration = 15 mins, Name = "User" in this example)           |
| Set duration                              | <netfn+cmd+oem (5 bytes)>* + <subcommand for set duration (1 byte)> + <duration in ms (8 bytes)> + <length of duration name (1 byte)> + <duration name (n bytes)> |
|                                           | e.g. 0x2e 0x32 0x79 0x2b 0x00 0x21 0xa0 0xbb 0x0d 0x00 0x00 0x00 0x00 0x00 0x04 0x44 0x48 0x43 0x50 (Duration = 15 mins, Name = "DHCP" in this example)           |
| Set reboot type                           | <netfn+cmd+oem (5 bytes)>* + <subcommand for set duration (1 byte)> + <duration in ms (8 bytes)> + <length of duration name (1 byte)> + <duration name (n bytes)> |
|                                           | e.g. 0x2e 0x32 0x79 0x2b 0x00 0x22 0x00 (Reboot type=0x00)                                                                                                        |

