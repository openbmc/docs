# phosphor-audit

Author:
  Ivan Mikhaylov, [i.mikhaylov@yadro.com](mailto:i.mikhaylov@yadro.com)

Primary assignee:
  Ivan Mikhaylov, [i.mikhaylov@yadro.com](mailto:i.mikhaylov@yadro.com)
  George Liu, [liuxiwei@inspur.com](mailto:liuxiwei@inspur.com)

Other contributors:
  Alexander Amelkin, [a.amelkin@yadro.com](mailto:a.amelkin@yadro.com)
  Alexander Filippov, [a.filippov@yadro.com](mailto:a.filippov@yadro.com)

Created:
  2019-07-23

Updated:
  2021-08-19

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

## Assumptions

This design assumes that an end user is never given a direct access to the
system shell. The shell allows for direct manipulation of user database
(add/remove users, change passwords) and system configuration (network scripts,
etc.), and it doesn't seem feasible to track such user actions taken within the
shell. This design assumes that all user interaction with OpenBMC is limited to
controlled interfaces served by other Phosphor OpenBMC components interacting
via D-Bus.

## Requirements

 * Provide a unified method of logging user actions independent of the user
   interface, where possible user actions are:
   * Redfish/REST PUT/POST/DELETE/PATCH
   * IPMI
   * PAM
   * PLDM
   * Any other suitable service
 * Provide a way to configure system response actions taken upon certain user
   actions, where possible response actions are:
   * Log an event
   * Notify an administrator or an arbitrary notification receiver
   * Run an arbitrary command
 * Provide a way to configure notification receivers:
   * E-mail
   * SNMP
   * Instant messengers
   * D-Bus

## Proposed Design

Send audit data to `phosphor-logging` by user interface, and send D-Bus to
`phosphor-audit` according to the event type (if it is an audit event), and
`phosphor-audit` processes the request according to the configuration.

Create a sub-repo(`phosphor-audit`) in the phosphor-logging repo.
The advantage of this is that all log-related functions are integrated into one
repo (`phosphor-logging`).

The phosphor-audit service represents a service that provides user activity
tracking and corresponding action taking in response of user actions.

The key benefit of using phosphor-audit is that all action handling will be kept
inside this project instead of spreading it across multiple dedicated interface
services with a risk of missing a handler for some action in one of them and
bloating the codebase.

The component diagram below shows the example of service overview.

```ascii
  +----------------+  logging::report                                           +-----------------+
  |    IPMI NET    +-----+                                                      | action          |
  +----------------+     |                                                      | +-------------+ |
                         |                                                      | |   logging   | |
  +----------------+     |                                                      | +-------------+ |
  |   IPMI HOST    +-----+    +------------------+         +-------------+      |                 |
  +----------------+     |    | phosphor-logging |  D-Bus  |  audit      |      | +-------------+ |
                         +---->+     report       +-------->+ service     +---->| |   command   | |
  +----------------+     |    |                  |         |             |      | +-------------+ |
  |  RedFish/REST  +-----+    +------------------+         +-------------+      |                 |
  +----------------+     |                                                      | +-------------+ |
                         |                                                      | |   notify    | |
  +----------------+     |                                                      | +-------------+ |
  |  any service   +-----+                                                      |                 |
  +----------------+                                                            | +-------------+ |
                                                                                | |     ...     | |
                                                                                | +-------------+ |
                                                                                +-----------------+
```

The audit event from diagram generated by an application to track user activity.
The application calls the report (or other API) method to send logs to
`phosphor-logging`, and `phosphor-logging` sends 'signals' to audit service via
D-Bus. What is happening next in audit service's handler depends on user requirements
and needs. It is possible to just store logs, run arbitrary command or notify someone
in handler or we can do all of the above and all of this can be optional.

**Phosphor logging call**
`phosphor-logging::report` will determine the type of incoming data.
If it is an audit log (AUDIT_MESSAGE_ID), it is directly sent to
the audit service via audit-event.

**Audit event call**

After the audit service receives the signal, it should preprocess the data first.
If the request is filtered out, it will be dropped at this moment and will
no longer be processed. After the filter check, the audit service decides
the next step. Also, it caches list of possible commands (blacklist or whitelist)
and status of its service (disabled or enabled).

 > `audit_event(type, rc, user, host, message, intf)`
 > *  type - type of event source : IPMI, REST, PAM, etc.
 > *  rc   - return code of the handler event (status, rc, etc.)
 > *  user - the user account on behalf of which the event was processed.
 >           depends on context, NA/None in case of user inaccessibility.
 > *  source - identifier of the host that the event has originated from. This can
 >     be literally "host" for events originating from the local host (via locally
 >     connected IPMI), or an IP address or a hostname of a remote host.
 > *  message - any supplementary data that can help better identify the event
 >      (e.g., some first bytes of the IPMI command data).
 > *  intf - D-Bus interface for current events.

Service itself can control flow of events with configuration on its side.

Pseudocode for example:

    audit_event(NET_IPMI, "access denied"(rc=-1), "qwerty223", "192.168.0.1",
                          "ipmi cmd", "xyz.openbmc_project.Event.Foo1")
    audit_event(REST, "login successful"(rc=200), "qwerty223", "192.168.0.1",
                          "rest login", "xyz.openbmc_project.Event.Foo2")
    audit_event(HOST_IPMI, "shutting down the host"(rc=0), NULL, NULL,
                       "host poweroff",, "xyz.openbmc_project.Event.Foo3")

`audit_event(blob_data)`
Blob can be described as structure:

    struct blob_audit
    {
        uint8_t type;
        int32_t rc;
        char *user;
        sockaddr_in6 *addr;
        char *message;
        char *intf;
    }

When the call reaches the server destination via D-Bus, the server already knows
that the call should be processed via predefined list of actions which are set
in the server configuration.

Step by step execution of call:
 * client's layer
    1. Send data to the `phosphor-logging` service via the
       `phosphor-logging::report` method
    2. `phosphor-logging` check whether the data type is audit log (AUDIT_MESSAGE_ID)
    3. If it is an audit log, send the data to the audit service via D-Bus
 * server's layer
    1. accept D-Bus request
    2. perform preprocessing operations
    3. goes through list of actions for each services

How the checks will be processed at client's layer:
 1. The client does not do any checks, and even the client does not know whether
    the log currently sent belongs to audit log or redfish log.

## Service configuration

The configuration structure can be described as tree with set of options,
as example of structure:

```
[IPMI]
   [Enabled]
   [Whitelist]
     [xyz.openbmc_project.Event.Foo1]
     [xyz.openbmc_project.Event.Foo2]
   [Actions]
     [Notify type1] [Recipient]
     [Notify type2] [Recipient]
     [Notify type3] [Recipient]
     [Logging type] [Options]
     [Exec] [ExternalCommand]
[REST]
   [Disabled]
   [Blacklist]
     [xyz.openbmc_project.Event.Foo3]
     [xyz.openbmc_project.Event.Foo4]
   [Actions]
     [Notify type2] [Recipient]
     [Logging type] [Options]
```

Options can be updated via D-Bus properties. The audit service listens changes
on configuration file and emit 'PropertiesChanged' signal with changed details.

* The allowlist and blocklist

 > Possible list of requests which have to be filtered and processed.
 > 'AllowList' filters possible requests which can be processed.
 > 'BlockList' blocks only exact requests.

* Enable/disable the event processing for directed services, where the directed
  service is any suitable services which can use audit service.

 > Each audit processing type can be disabled or enabled at runtime via
 > config file or D-Bus property.

* Notification setup via SNMP/E-mail/Instant messengers/D-Bus

 > The end recipient notification system with different transports.

* Logging

 > phosphor-logging, journald or anything else suitable for.

* User actions

 > Running a command as consequenced action.

## Workflow

An example of possible flow:

```ascii
           +----------------+
           |   NET   IPMI   |
           |    REQUEST     |
           +----------------+
                   |
 +--------------------------------------------------------------------------+
 |         +-------v--------------+                       phosphor-logging  |
 |         | report<IPMIEVENT>(); |                                         |
 |         +----------------------+                                         |
 |                 |                                                        |
 |     NO  +-------v--------+   YES  +---------------------------+          |
 |   +-----|    Is audit    +------->|  audit_event<NET_IPMI>()  |          |
 |   |      +----------------+        +---------------------------+         |
 |   |              |                              |                        |
 |   |              |                              |                        |
 |   |      +-------v--------+                     |                        |
 |   |      |   Do other     |                     |                        |
 |   |      |  operations    |                     |                        |
 |   +------|                |                     |                        |
 |          +----------------+                     |                        |
 +--------------------------------------------------------------------------+
                                                   |
                                                   |
 +--------------------------------------------------------------------------+
 |                  +------------------------------+                        |
 |                  |                                        Audit Service  |
 |                  |                                                       |
 |                  |                                                       |
 |                  |                                                       |
 |            +-----v------+                                                |
 |        NO  | Is logging |        YES                                     |
 |     +------+  enabled   +--------------------+                           |
 |     |      | for  type? |                    |                           |
 |     |      +------------+            +-------v-----+                     |
 |     |                           NO   | Is request  |   YES               |
 |     |                       +--------+    type     +--------+            |
 |     |                       |        |  filtered?  |        |            |
 |     |                       |        +-------------+        |            |
 |     |                       |                               |            |
 |     |               +-------v-------+                       |            |
 |     |               |    Notify     |                       |            |
 |     |               | Administrator |                       |            |
 |     |               +---------------+                       |            |
 |     |                       |                               |            |
 |     |               +-------v-------+                       |            |
 |     |               |   Log Event   |                       |            |
 |     |               +---------------+                       |            |
 |     |                       |                               |            |
 |     |               +-------v-------+                       |            |
 |     |               |     User      |                       |            |
 |     |               |    actions    |                       |            |
 |     |               +---------------+                       |            |
 |     |                       |                               |            |
 |     |               +-------v-------+                       |            |
 |     +-------------->|      End      |<----------------------+            |
 |                     +---------------+                                    |
 |                                                                          |
 +--------------------------------------------------------------------------+
```

## Notification mechanisms

The unified model for reporting accidents to the end user, where the transport can be:

* E-mail

  > Sending a note to directed recipient which set in configuration via
  > sendmail or anything else.

* SNMP

  > Sending a notification via SNMP trap messages to directed recipient which
  > set in configuration.

* Instant messengers

  > Sending a notification to directed recipient which set in configuration via
  > jabber/sametime/gtalk/etc.

* D-Bus

  > Notify the other service which set in configuration via 'method_call' or
  > 'signal'.

Notifications will be skipped in case if there is no any of above configuration
rules is set inside configuration. It is possible to pick up rules at runtime.

## User Actions

 * Exec application via 'system' call.
 * The code for directed handling type inside handler itself.
   As example for 'net ipmi' in case of unsuccessful user login inside handler:
   * Sends a notification to administrator.
   * echo heartbeat > /sys/class/leds/alarm_red/trigger

## Alternatives Considered

Processing user requests in each dedicated interface service and logging
them separately for each of the interfaces. Scattered handling looks like
an error-prone and rigid approach.

## Impacts

Improves system manageability and security.

Impacts when phosphor-audit is not enabled:
 - Many services will have slightly larger code size and longer CPU path length
   due to invocations of audit_event().
 - Increased D-Bus traffic.

Impacts when phosphor-audit is enabled:
All of the above, plus:
 - Additional BMC processor time needed to handle audit events.
 - Additional BMC flash storage needed to store logged events.
 - Additional outbound network traffic to notify users.
 - Additional space for notification libraries.

## Testing

`dbus-send` as command-line tool for generating audit events.

Scenarios:
 - For each supported service (such as Redfish, net IPMI, host IPMI, PLDM), create audit events, and validate they get logged.
 - Ensure message-type and request-type filtering works as expected.
 - Ensure basic notification actions work as expected (log, command, notify).
 - When continuously generating audit-events, change the phosphor-audit service's configuration, and validate no audit events are lost, and the new configuration takes effect.
