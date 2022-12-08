# BMC Service Failure Debug and Recovery

Author: Andrew Jeffery <andrew@aj.id.au> @arj

Other contributors: Andrew Geissler <geissonator@yahoo.com> @geissonator

Created: 6th May 2021

## Problem Description

The capability to debug critical failures of the BMC firmware is essential to
meet the reliability and serviceability claims made for some platforms.

This design addresses a few classes of failures:

- A class of failure exists where a BMC service has entered a failed state but
  the BMC is still operational in a degraded mode.
- A class of failure exists under which we can attempt debug data collection
  despite being unable to communicate with the BMC via standard protocols.

This proposal argues for and proposes a software-driven debug data capture and
recovery of a failed BMC.

## Background and References

By necessity, BMCs are not self-contained systems. BMCs exist to service the
needs of both the host system by providing in-band platform services such as
thermal and power management as well as system operators by providing
out-of-band system management interfaces such as error reporting, platform
telemetry and firmware management.

As such, failures of BMC subsystems may impact external consumers.

The BMC firmware stack is not trivial, in the sense that common implementations
are usually a domain-specific Linux distributions with complex or highly coupled
relationships to platform subsystems.

Complexity and coupling drive concern around the risk of critical failures in
the BMC firmware. The BMC firmware design should provide for resilience and
recovery in the face of well-defined error conditions, but the need to mitigate
ill-defined error conditions or entering unintended software states remains.

The ability for a system to recover in the face of an error condition depends on
its ability to detect the failure. Thus, error conditions can be assigned to
various classes based on the ability to externally observe the error:

1. Continued operation: The services detects the error and performs the actions
   required to return to its operating state

2. Graceful exit: The service detects an error it cannot recover from, but
   gracefully cleans up its resources before exiting with an appropriate exit
   status

3. Crash: The service detects it is an unintended software state and exits
   immediately, failing to gracefully clean up its resources before exiting

4. Unresponsive: The service fails to detect it cannot make progress and
   continues to run but is unresponsive

As the state transformations to enter the ill-defined or unintended software
state are unanticipated, the actions required to gracefully return to an
expected state are also not well defined. The general approaches to recover a
system or service to a known state in the face of entering an unknown state are:

1. Restart the affected service
2. Restart the affected set of services
3. Restart all services

In the face of continued operation due to internal recovery a service restart is
unnecessary, while in the case of a unresponsive service the need to restart
cannot be detected by service state alone. Implementation of resiliency by way
of service restarts via a service manager is only possible in the face of a
graceful exit or application crash. Handling of services that have entered an
unresponsive state can only begin upon receiving external input.

Like error conditions, services exposed by the BMC can be divided into several
external interface classes:

1. Providers of platform data
2. Providers of platform data transports

Examples of the first are applications that expose various platform sensors or
provide data about the firmware itself. Failure of the first class of
applications usually yields a system that can continue to operate in a reduced
capacity.

Examples of the second are the operating system itself and applications that
implement IPMI, HTTPS (e.g. for Redfish), MCTP and PLDM. This second class also
covers implementation-specific data transports such as D-Bus, which requires a
broker service. Failure of a platform data transport may result in one or all
external interfaces becoming unresponsive and be viewed as a critical failure of
the BMC.

Like error conditions and services, the BMC's external interfaces can be divided
into several classes:

1. Out-of-band interfaces: Remote, operator-driven platform management
2. In-band interfaces: Local, host-firmware-driven platform management

Failures of platform data transports generally leave out-of-band interfaces
unresponsive to the point that the BMC cannot be recovered except via external
means, usually by issuing a (disruptive) AC power cycle. On the other hand, if
the host can detect the BMC is unresponsive on the in-band interface(s), an
appropriate platform design can enable the host to reset the BMC without
disrupting its own operation.

### Analysis of eBMC Error State Management and Mitigation Mechanisms

Assessing OpenBMC userspace with respect to the error classes outlined above,
the system manages and mitigates error conditions as follows:

| Condition           | Mechanism                                           |
| ------------------- | --------------------------------------------------- |
| Continued operation | Application-specific error handling                 |
| Graceful exit       | Application-specific error handling                 |
| Crash               | Signal, unhandled exceptions, `assert()`, `abort()` |
| Unresponsive        | None                                                |

These mechanisms inform systemd (the service manager) of an event, which it
handles according to the restart policy encoded in the unit file for the
service.

OpenBMC has a default behavior for all systemd services. That default is to
allow an OpenBMC systemd service to restart twice every 30 seconds. If a service
restarts more then twice within 30 seconds then that service will be considered
to be in a failed state by systemd and not restarted again until a BMC reboot.

Assessing the OpenBMC operating system with respect to the error classes, it
manages and mitigates error conditions as follows:

| Condition           | Mechanism                              |
| ------------------- | -------------------------------------- |
| Continued operation | ramoops, ftrace, `printk()`            |
| Graceful exit       | System reboot                          |
| Crash               | kdump or ramoops                       |
| Unresponsive        | `hardlockup_panic`, `softlockup_panic` |

Crash conditions in the Linux kernel trigger panics, which are handled by kdump
(though may be handled by ramoops until kdump support is integrated). Kernel
lockup conditions can be configured to trigger panics, which in-turn trigger
either ramoops or kdump.

### Synthesis

In the context of the information above, handling of application lock-up error
conditions is not provided. For applications in the platform-data-provider class
of external interfaces, the system will continue to operate with reduced
functionality. For applications in the platform-data-transport-provider class,
this represents a critical failure of the firmware that must have accompanying
debug data.

## Handling platform-data-transport-provider failures

### Requirements

#### Recovery Mechanisms

The ability for external consumers to control the recovery behaviour of BMC
services is usually coarse, the nuanced handling is left to the BMC
implementation. Where available the options for external consumer tend to be, in
ascending order of severity:

| Severity | BMC Recovery Mechanism  | Used for                                                              |
| -------- | ----------------------- | --------------------------------------------------------------------- |
| 1        | Graceful reboot request | Normal circumstances or recovery from platform data provider failures |
| 2        | Forceful reboot request | Recovery from unresponsive platform data transport providers          |
| 3        | External hardware reset | Unresponsive operating system                                         |

Of course it's not possible to issue these requests over interfaces that are
unresponsive. A robust platform design should be capable of issuing all three
restart requests over separate interfaces to minimise the impact of any one
interface becoming unresponsive. Further, the more severe the reset type, the
fewer dependencies should be in its execution path.

Given the out-of-band path is often limited to just the network, it's not
feasible for the BMC to provide any of the above in the event of some kind of
network or relevant data transport failure. The considerations here are
therefore limited to recovery of unresponsive in-band interfaces.

The need to escalate above mechanism 1 should come with data that captures why
it was necessary, i.e. dumps for services that failed in the path for 1.
However, by escalating straight to 3, the BMC will necessarily miss out on
capturing a debug dump because there is no opportunity for software to intervene
in the reset. Therefore, mechanism 2 should exist in the system design and its
implementation should capture any appropriate data needed to debug the need to
reboot and the inability to execute on approach 1.

The need to escalate to 3 would indicate that the BMC's own mechanisms for
detecting a kernel lockup have failed. Had they not failed, we would have
ramoops or kdump data to analyse. As data cannot be captured with an escalation
to method 3 the need to invoke it will require its own specialised debug
experience. Given this and the kernel's own lockup detection and data collection
mechanism, support for 2 can be implemented in BMC userspace.

Mechanism 1 is typically initiated by the usual in-band interfaces, either IPMI
or PLDM. In order to avoid these in the implementation of mechanism 2, the host
needs an interface to the BMC that is dedicated to the role of BMC recovery,
with minimal dependencies on the BMC side for initiating the dump collection and
reboot. At its core, all that is needed is the ability to trigger a BMC IRQ,
which could be as simple as monitoring a GPIO.

#### Behavioural Requirements for Recovery Mechanism 2

The system behaviour requirement for the mechanism is:

1. The BMC executes collection of debug data and then reboots once it observes a
   recovery message from the host

It's desirable that:

1. The host has some indication that the recovery process has been activated
2. The host has some indication that a BMC reset has taken place

It's necessary that:

1. The host make use of a timeout to escalate to recovery mechanism 3 as it's
   possible the BMC will be unresponsive to recovery mechanism 2

#### Analysis of BMC Recovery Mechanisms for Power10 Platforms

The implementation of recovery mechanism 1 is already accounted for in the
in-band protocols between the host and the BMC and so is considered resolved for
the purpose of the discussion.

To address recovery mechanism 3, the Power10 platform designs wire up a GPIO
driven by the host to the BMC's EXTRST pin. If the host firmware detects that
the BMC has become unresponsive to its escalating recovery requests, it can
drive the hardware to forcefully reset the BMC.

However, host-side GPIOs are in short supply, and we do not have a dedicated pin
to implement recovery mechanism 2 in the platform designs.

#### Analysis of Implementation Methods on Power10 Platforms

The implementation of recovery mechanism 2 is limited to using existing
interfaces between the host and the BMC. These largely consist of:

1. FSI
2. LPC
3. PCIe

FSI is inappropriate because the host is the peripheral in its relationship with
the BMC. If the BMC has become unresponsive, it is possible it's in a state
where it would not accept FSI traffic (which it needs to drive in the first
place) and we would need an mechanism architected into FSI for the BMC to
recognise it is in a bad state. PCIe and LPC are preferable by comparison as the
BMC is the peripheral in this relationship, with the host driving cycles into it
over either interface. Comparatively, PCIe is more complex than LPC, so an
LPC-based approach is preferred.

The host already makes use of several LPC peripherals exposed from the BMC:

1. Mapped LPC FW cycles
2. iBT for IPMI
3. The VUARTs for system and debug consoles
4. A KCS device for a vendor-defined MCTP LPC binding

The host could take advantage of any of the following LPC peripherals for
implementing recovery mechanism 2:

1. The SuperIO-based iLPC2AHB bridge
2. The LPC mailbox
3. An otherwise unused KCS device

In ASPEED BMC SoCs prior to the AST2600 the LPC mailbox required configuration
via the SuperIO device, which exposes the unrestricted iLPC2AHB backdoor into
the BMC's physical address space. The iLPC2AHB capability could not be mitigated
without disabling SuperIO support entirely, and so the ability to use the
mailbox went with it. This security issue is resolved in the AST2600 design, so
the mailbox could be used in the Power10 platforms, but we have lower-complexity
alternatives for generating an IRQ on the BMC. We could use the iLPC2AHB from
the host to drive one of the watchdogs in the BMC to trigger a reset, but this
exposes a stability risk due to the unrestricted power of the interface, let
alone the security implications, and like the mailbox is more complex than the
alternatives.

This draws us towards the use of a KCS device, which is best aligned with the
simple need of generating an IRQ on the BMC. AST2600 has at least 4 KCS devices
of which one is already in use for IBM's vendor-defined MCTP LPC binding leaving
at least 3 from which to choose.

### Proposed Design

The proposed design is for a simple daemon started at BMC boot to invoke the
desired crash dump handler according to the system policy upon receiving the
external signal. The implementation should have no IPC dependencies or
interactions with `init`, as the reason for invoking the recovery mechanism is
unknown and any of these interfaces might be unresponsive.

A trivial implementation of the daemon is

```sh
dd if=$path bs=1 count=1
echo c > /proc/sysrq-trigger
```

For systems with kdump enabled, this will result in a kernel crash dump
collection and the BMC being rebooted.

A more elegant implementation might be to invoke `kexec` directly, but this
requires the support is already available on the platform.

Other activities in userspace might be feasible if it can be assumed that
whatever failure has occurred will not prevent debug data collection, but no
statement about this can be made in general.

#### A Idealised KCS-based Protocol for Power10 Platforms

The proposed implementation provides for both the required and desired
behaviours outlined in the requirements section above.

The host and BMC protocol operates as follows, starting with the BMC application
invoked during the boot process:

1. Set the `Ready` bit in STR

2. Wait for an `IBF` interrupt

3. Read `IDR`. The hardware clears IBF as a result

4. If the read value is 0x44 (`D` for "Debug") then execute the debug dump
   collection process and reboot. Otherwise,

5. Go to step 2.

On the host:

1. If the `Ready` bit in STR is clear, escalate to recovery mechanism 3.
   Otherwise,

2. If the `IBF` bit in STR is set, escalate to recovery mechanism 3. Otherwise,

3. Start an escalation timer

4. Write 0x44 (`D` for "Debug") to the Input Data Register (IDR). The hardware
   sets IBF as a result

5. If `IBF` clears before expiry, restart the escalation timer

6. If an STR read generates an LPC SYNC No Response abort, or `Ready` clears
   before expiry, restart the escalation timer

7. If `Ready` becomes set before expiry, disarm the escalation timer. Recovery
   is complete. Otherwise,

8. Escalate to recovery mechanism 3 if the escalation timer expires at any point

A SerIRQ is unnecessary for correct operation of the protocol. The BMC-side
implementation is not required to emit one and the host implementation must
behave correctly without one. Recovery is only necessary if other paths have
failed, so STR can be read by the host when it decides recovery is required, and
by read by time-based polling thereafter.

The host must be prepared to handle LPC SYNC errors when accessing the KCS
device IO addresses, particularly "No Response" aborts. It is not guaranteed
that the KCS device will remain available during BMC resets.

As STR is polled by the host it's not necessary for the BMC to write to ODR. The
protocol only requires the host to write to IDR and periodically poll STR for
changes to IBF and Ready state. This removes bi-directional dependencies.

The uni-directional writes and the lack of SerIRQ reduce the features required
for correct operation of the protocol and thus the surface area for failure of
the recovery protocol.

The layout of the KCS Status Register (STR) is as follows:

| Bit | Owner    | Definition               |
| --- | -------- | ------------------------ |
| 7   | Software |                          |
| 6   | Software |                          |
| 5   | Software |                          |
| 4   | Software | Ready                    |
| 3   | Hardware | Command / Data           |
| 2   | Software |                          |
| 1   | Hardware | Input Buffer Full (IBF)  |
| 0   | Hardware | Output Buffer Full (OBF) |

#### A Real-World Implementation of the KCS Protocol for Power10 Platforms

Implementing the protocol described above in userspace is challenging due to
available kernel interfaces[1], and implementing the behaviour in the kernel
falls afoul of the defacto "mechanism, not policy" rule of kernel support.

Realistically, on the host side the only requirements are the use of a timer and
writing the appropriate value to the Input Data Register (IDR). All the proposed
status bits can be ignored. With this in mind, the BMC's implementation can be
reduced to reading an appropriate value from IDR. Reducing requirements on the
BMC's behaviour in this way allows the use of the `serio_raw` driver (which has
the restriction that userspace can't access the status value).

[1]
https://lore.kernel.org/lkml/37e75b07-a5c6-422f-84b3-54f2bea0b917@www.fastmail.com/

#### Prototype Implementation Supporting Power10 Platforms

A concrete implementation of the proposal's userspace daemon is available on
Github:

https://github.com/amboar/debug-trigger/

Deployment requires additional kernel support in the form of patches at [2].

[2]
https://github.com/amboar/linux/compare/2dbb5aeba6e55e2a97e150f8371ffc1cc4d18180...for/openbmc/kcs-raw

### Alternatives Considered

See the discussion in Background.

### Impacts

The proposal has some security implications. The mechanism provides an
unauthenticated means for the host firmware to crash and/or reboot the BMC,
which can itself become a concern for stability and availability. Use of this
feature requires that the host firmware is trusted, that is, that the host and
BMC firmware must be in the same trust domain. If a platform concept requires
that the BMC and host firmware remain in disjoint trust domains then this
feature must not be provided by the BMC.

As the feature might provide surprising system behaviour, there is an impact on
documentation for systems deploying this design: The mechanism must be
documented in such a way that rebooting the BMC in these circumstances isn't
surprising.

Developers are impacted in the sense that they may have access to better debug
data than might otherwise be possible. There are no obvious developer-specific
drawbacks.

Due to simplicity being a design-point of the proposal, there are no significant
API, performance or upgradability impacts.

### Testing

Generally, testing this feature requires complex interactions with host firmware
and platform-specific mechanisms for triggering the reboot behaviour.

For Power10 platforms this feature may be safely tested under QEMU by scripting
the monitor to inject values on the appropriate KCS device. Implementing this
for automated testing may need explicit support in CI.

## Handling platform-data-provider failures

### Requirements

As noted above, these types of failures usually yield a system that can continue
to operate in a reduced capacity. The desired behavior in this scenario can vary
from system to system so the requirements in this area need to be flexible
enough to allow system owners to configure their desired behavior.

The requirements for OpenBMC when a platform-data-provider service enters a
failure state are that the BMC:

- Logs an error indicating a service has failed
- Collects a BMC dump
- Changes BMC state (CurrentBMCState) to indicate a degraded mode of the BMC
- Allow system owners to customize other behaviors (i.e. BMC reboot)

### Proposed Design

This will build upon the existing [target-fail-monitoring][1] design. The
monitor service will be enhanced to also take json file(s) which list critical
services to monitor.

Define a "obmc-bmc-service-quiesce.target". System owners can install any other
services they wish in this new target.

phosphor-bmc-state-manager will monitor this target and enter a `Quiesced` state
when it is started. This state will be reported externally via the Redfish API
under redfish/v1/Managers/bmc status property.

This would look like the following:

- In a services-to-monitor configuration file, add all critical services
- The state-manager service-monitor will subscribe to signals for service
  failures and do the following when one fails from within the configuration
  file:
  - Log error with service failure information
  - Request a BMC dump
  - Start obmc-bmc-service-quiesce.target
- BMC state manager detects obmc-bmc-service-quiesce.target has started and puts
  the BMC state into Quiesced
- bmcweb looks at BMC state to return appropriate state to external clients

[1]:
  https://github.com/openbmc/docs/blob/master/designs/target-fail-monitoring.md

### Alternatives Considered

One simpler option would be to just have the OnFailure result in a BMC reboot
but historically this has caused more problems then it solves:

- Rarely does a BMC reboot fix a service that was not fixed by simply restarting
  it.
- A BMC that continuously reboots itself due to a service failure is very
  difficult to debug.
- Some BMC's only allow a certain amount of reboots so eventually the BMC ends
  up stuck in the boot loader which is inaccessible unless special debug cables
  are available so for all intents and purposes your system is now unusable.

### Impacts

Currently nothing happens when a service enters the fail state. The changes
proposed in this document will ensure an error is logged a dump is collected,
and the external BMC state reflects the failure when this occurs.

### Testing

A variety of service should be put into the fail state and the tester should
ensure the appropriate error is logged, dump is collected, and BMC state is
changed to reflect this.
