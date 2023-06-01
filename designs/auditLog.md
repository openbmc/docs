# Audit Logging

Author: Janet Adkins, jeaaustx#1116

Other contributors: Gunnar Mills, GunnarM#7346, & Manojkiran Eda,
manojkiran#7205

Created: May 22, 2023

## Problem Description

The Redfish API provides interfaces to modify the OpenBMC resources. When
something goes wrong with the OpenBMC knowing the history of modifications can
simplify the debugging process. Additionally, security best-practices require
maintaining a log of actions to prevent or diagnose potential security
vulnerabilities. The
[U.S. Government Approved Protection Profile - Protection Profile for Virtualization Version 1.1](https://www.niap-ccevs.org/Profile/Info.cfm?PPID=456&id=456)
defines security requirements for virtualization environments.

This design preserves a history of modifications using Redfish API and provides
a mechanism to make it available to a system administrator.

The design focuses on only the Redfish API entry points. Other entry points
including IPMI and ssh are not included. Future work could extend the auditing
to include these entry points as well.

## Background and References

There have been at least three prior attempts to include audit logging of bmcweb
events in the upstream OpenBMC. The conversations around these raised different
issues and none have been adopted.

### **Crow Middleware Approach**

This approach was abandoned. From comments it appears mainly due to use of
middleware and this approach being considered a short-term solution. See
[REST and Redfish Traffic Logging](https://gerrit.openbmc.org/c/openbmc/bmcweb/+/22699)
for history.

### **Independent Backend Logger**

This approach was abandoned. There were many questions and comments to be
resolved on the design. The idea seems to have been abandoned mainly due to
inaction of addressing the issues. See
[phosphor-audit Design](https://gerrit.openbmc.org/c/openbmc/docs/+/46023) for
community conversation on design issues and
[openbmc/docs/designs/phosphor-audit.md](https://github.com/openbmc/docs/blob/master/designs/phosphor-audit.md)
for design details.

### **Use auditd**

This approach is still open and is the starting point for this design.
Manojkiran Eda proposed using the Linux auditd service as the backend for
gathering and analyzing the log data. See
[Add audit events support in bmcweb](https://gerrit.openbmc.org/c/openbmc/bmcweb/+/55532)
for history.

The [OpenSwitch project](https://www.openswitch.net/about/) uses auditd in a
very similar manner to save a record of events. The OpenSwitch
[Audit Log How-to Guide for OpenSwitch Developers](https://github.com/gitname/ops-docs/blob/master/AuditLogHowToGuide.md)
document describes the usage.

Within OpenBMC there are two existing recipes which can be configured to use the
auditd for userspace logging.

- poky/meta/recipes-core/dbus - Used with SELinux and AppArmor. (Ref.
  [apparmor.c](https://gitlab.freedesktop.org/dbus/dbus/-/blob/master/bus/apparmor.c),
  [audit.c](https://gitlab.freedesktop.org/dbus/dbus/-/blob/master/bus/audit.c),
  [selinux.c](https://gitlab.freedesktop.org/dbus/dbus/-/blob/master/bus/selinux.c))
- meta-openembedded/meta-networking/recipes-connectivity/networkmanager - Used
  with SELinux and user-configurable. Network manager logs events and if enabled
  also logs using auditd. (Ref. src/core/nm-audit-manager.c)

## Requirements

This design utilizes the Linux auditd subsystem to record events coming through
the bmcweb. Additionally, the audit log events can be retrieved by the client
using a Redfish API. Doing so allows a GUI (graphical user interface) such as
webui-vue to display the audit events and to provide additional function to
allow filtering of the events based on time of the event, Redfish schema, or
other helpful event detail.

## Proposed Design

Redfish DELETE, PATCH, POST, and PUT events associated to a user are captured
using the auditd kernel subsystem. Adding a callout within _completeRequest()_
provides a convenient single location to capture these events. Future events
will also be easily handled in the same manner.

Both failed and successful user login events are recorded using auditd. In order
to record both types, the callout to record these events is done on return from
the _pamAuthenticateUser()_ call rather than in _completeRequest()_.

The audit logs are included with a user type dreport dump. The audit logs can be
analyzed using the Linux auditd utilities such as ausearch.

A meson build option is added to control whether the event auditing is turned on
or not. The option is disabled by default. When disabled the audit entry points
are not compiled into the bmcweb binary.

There is an existing Redfish Schema which provides the elements needed to
communicate with the client for accessing the audit log events. The
[LogService Schema](https://redfish.dmtf.org/schemas/v1/LogService.v1_4_0.json)
provides the structure needed. The _Security_ type for _LogPurpose_ is a good
fit for the data tracked by the audit log.

```
"@odata.id" : "/redfish/v1/Systems/system/LogServices/AuditLog"
"LogPurpose" : "Security"
"LogDiagnosticDataType" : "Manager"
```

The _#LogService.CollectDiagnosticData_ action of the LogServices schema is used
to extract the log records. There is an existing Redfish Schema for the format
of extracted log entries. The
[LogEntry Schema](https://redfish.dmtf.org/schemas/v1/LogEntry.v1_14_0.json)
provides the structure needed.

```
// Each audit log entry has a unique audit event id. This can be used here.
"@odata.id" : "/redfish/v1/Systems/system/LogServices/AuditLog/Entries/<auditEventID>"
"Id" : "<auditEventID>"
"EntryType" : "Event"
"EventTimestamp" : "<Extracted timestamp from audit log entry>"
"Message" : "<Extracted message from audit log entry>"
"MessageId" : "<New MessageId for openbmc_message_registry>"
```

The
[SPEC Audit Event Parsing Library](https://github.com/linux-audit/audit-documentation/wiki/SPEC-Audit-Event-Parsing-Library)
provides utility functions meant to be used by external applications. Bmcweb
will parse the audit log entries into the JSON LogEntry Redfish format using the
utility functions of this library.

## Alternatives Considered

The alternative solutions covered in the background section shared the common
requirement of implementing a backend scheme for recording bmcweb events. The
Linux auditd subsystem provides the backend support which will save the effort
required to implement a new scheme. Additionally, it can be configured to adjust
how often the records are written to disk along with other options. These
configuration options give individual OpenBMC distributions the ability to
tailor the auditing to their individual needs. The
[SUSE: Understanding Linux Audit](https://documentation.suse.com/sles/12-SP4/html/SLES-all/cha-audit-comp.html)
article describes these options well.

## Impacts

An image built of a prototype using this method results in an image size 340K
larger than the image built without enabling auditing.

Minimal performance impacts have been observed with a prototype using this
method.

### Redfish Validator Performance Impact

The Redfish Validator was run four times each against an image built with Audit
Logging enabled and again disabled using the same hardware for all runs. Minimal
logging was recorded as the validator is mostly doing GET operations which are
not logged. The statistics across the runs show neglible performance impact:

```
| mm:ss.nnn | Mean      | Median    | Minimum   | Maximum   |
| --------- | --------- | --------- | --------- | --------- |
| Disabled  | 06:15.471 | 06:14.209 | 06:12.651 | 06:20.816 |
| Enabled   | 06:14.884 | 06:13.895 | 06:10.000 | 06:21.747 |
```

A PATCH Redfish operation was run 20 times each against an image built with
Audit Logging enabled and again disabled using the same hardware for all runs.
The PATCH operation does get logged when enabled. The statistics across the runs
show occasional minimal performance impact:

```
| Real (s) | Mean  | Median | Minimum | Maximum | Mode  |
| -------- | ----- | ------ | ------- | ------- | ----- |
| Disabled | 0.091 | 0.091  | 0.087   | 0.096   | 0.092 |
| Enabled  | 0.091 | 0.091  | 0.087   | 0.096   | 0.088 |

| User (s) | Mean  | Median | Minimum | Maximum | Mode  |
| -------- | ----- | ------ | ------- | ------- | ----- |
| Disabled | 0.009 | 0.009  | 0.005   | 0.016   | 0.004 |
| Enabled  | 0.009 | 0.008  | 0.000   | 0.017   | 0.010 |

| Sys (s)  | Mean  | Median | Minimum | Maximum | Mode  |
| -------- | ----- | ------ | ------- | ------- | ----- |
| Disabled | 0.007 | 0.007  | 0.000   | 0.012   | 0.006 |
| Enabled  | 0.007 | 0.007  | 0.000   | 0.014   | 0.005 |
```

A POST Redfish operation was run 20 times each against an image built with Audit
Logging enabled and again disabled using the same hardware for all runs. The
POST operation does get logged when enabled. The statistics across the runs show
minimal performance impact:

```
| Real (s) | Mean  | Median | Minimum | Maximum | Mode  |
|----------|-------|--------|---------|---------|-------|
| Disabled | 0.095 | 0.094  | 0.092   | 0.103   | 0.093 |
| Enabled  | 0.097 | 0.097  | 0.094   | 0.104   | 0.095 |

| User (s) | Mean  | Median | Minimum | Maximum | Mode  |
| -------- | ----- | ------ | ------- | ------- | ----- |
| Disabled | 0.010 | 0.009  | 0.005   | 0.015   | 0.009 |
| Enabled  | 0.008 | 0.009  | 0.000   | 0.011   | 0.009 |

| Sys (s)  | Mean  | Median | Minimum | Maximum | Mode  |
| -------- | ----- | ------ | ------- | ------- | ----- |
| Disabled | 0.005 | 0.005  | 0.000   | 0.010   | 0.005 |
| Enabled  | 0.007 | 0.005  | 0.004   | 0.013   | 0.005 |
```

## Organizational

- Does this repository require a new repository? No
- Which repositories are expected to be modified to execute this design? bmcweb

## Testing

Existing tests shall still pass as if this feature is not introduced.

New unit tests:

- log_services_dump_test.cpp: Add additional test for new AuditLog LogServices
  type. Similar testing as is currently done for other types.
- Add tests for all new utility functions created for creating or parsing audit
  log entries.

New Robot Framework tests for verifying logging of Redfish events:

- Login success logged
- Login failure logged
- Logout logged
- Redfish GET events are not logged
- Sample Redfish DELETE event logged
- Sample Redfish PATCH event logged
- Sample Redfish POST event logged
- Sample Redfish PUT event logged
- #LogService.CollectDiagnosticData returns logged events
