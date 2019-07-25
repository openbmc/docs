# phosphor-audit

Author:
  Ivan Mikhaylov, [i.mikhaylov@yadro.com](mailto:i.mikhaylov@yadro.com)

Primary assignee:
  Ivan Mikhaylov, [i.mikhaylov@yadro.com](mailto:i.mikhaylov@yadro.com)

Other contributors:
  Alexander Amelkin, [a.amelkin@yadro.com](mailto:a.amelkin@yadro.com)
  Alexander Filippov, [a.filippov@yadro.com](mailto:a.filippov@yadro.com)

Created:
  23.07.2019

## Problem Description

End users of OpenBMC may take actions that change the system state and/or
configuration. Such actions may be taken using any of the numerous interfaces
provided by OpenBMC. That includes RedFish, IPMI, ssh or serial console shell,
and other interfaces, including the future ones.

Consequences of those actions may sometimes be harmful and an investigation may
be conducted in order to find out the person responsible for the unwelcome
changes. Currently, most changes leave no trace in OpenBMC logs, which hampers
the aforementioned investigation.

It is required to develop a mechanism that would allow for tracking such
user activity, logging it, and taking certain actions if necessary.

## Background and References

YADRO had an internal solution for the problem. It was only applicable to an
outdated version of OpenBMC and needed a redesign. There was also a parallel
effort by IBM that can be found here:
[REST and Redfish Traffic Logging](https://gerrit.openbmc-project.xyz/c/openbmc/bmcweb/+/22699)

## Requirements

 * Provide a unified method of logging user actions independent of the user
   interface
 * Provide a way to configure system response actions taken upon certain user
   actions:
   * Log an event via phosphor-logging
   * Notify an administrator or an arbitrary notification receiver
   * Run an arbitrary command
 * Provide a way to configure notification receivers:
   * E-mail
   * SNMP

## Assumptions

This design assumes that an end user is never given a direct access to the
system shell. The shell allows for direct manipulation of user database
(add/remove users, change passwords) and system configuration (network scripts,
etc.), and it doesn't seem feasible to track such user actions taken within the
shell. This design assumes that all user interaction with OpenBMC is limited to
controlled interfaces served by other Phosphor OpenBMC components interacting
via d-bus.

## Proposed Design

The main idea is to catch dbus requests sent by user interfaces, then handle the
request according to the configuration. In future, support for flexible policies
may be implemented that would allow for better flexibility in handling and
tracking.

The key benefit of using phosphor-audit is that all action handling will be kept
inside this project instead of spreading it across multiple dedicated interface
services with a risk of missing a handler for some action in one of them and
bloating the codebase.

The component diagram below shows the service overview.

```ascii
  +----------------+  audit event                           +-----------------+
  |    IPMI NET    +-----------+                            | action          |
  +----------------+           |                            | +-------------+ |
                               |                            | |    ph-log   | |
  +----------------+           |                            | +-------------+ |
  |   IPMI HOST    +-----------+      +--------------+      |                 |
  +----------------+           |      |    audit     |      | +-------------+ |
                               +----->+   service    +----->| |   command   | |
  +----------------+           |      |              |      | +-------------+ |
  |  RedFish/REST  +-----------+      +--------------+      |                 |
  +----------------+           |                            | +-------------+ |
                               |                            | |   notify    | |
  +----------------+           |                            | +-------------+ |
  | any UI service +-----------+                            |                 |
  +----------------+                                        | +-------------+ |
                                                            | |     ...     | |
                                                            | +-------------+ |
                                                            +-----------------+
```

## Alternatives Considered

Processing user requests in each dedicated interface service and logging
them separately for each of the interfaces. Scattered handling looks like
an error-prone and rigid approach.

## Impacts

Improves system manageability and security.

## Testing
TBD
