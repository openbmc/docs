# Audit Logging

Author: Janet Adkins, jeaaustx

Other contributors: Gunnar Mills, GunnarM, & Manojkiran Eda, manojkiran

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

The design focuses on only the Redfish API entry points. No further updates are
planned for the IPMI specification per the
[IPMI Promoter announcement](https://www.intel.com/content/www/us/en/products/docs/servers/ipmi/ipmi-home.html).
Design details of IPMI entry points are not included since the community, the
industry, and IBM are all slowly moving away from it. The Linux auditd subsystem
can be configured to audit ssh so its details are not included in this design.
An implementation of the design could include support for these entry points.

## Background and References

This design proposes using the Linux kernel auditd subsystem for recording an
audit trail of events in bmcweb. See the
[Linux Kernel Audit Subsystem README file](https://github.com/linux-audit/audit-kernel#readme)
for details and links to current documentation.

The [OpenSwitch project](https://www.openswitch.net/about/) uses auditd in a
very similar manner to save a record of events including with its REST API. The
OpenSwitch
[AuditLogHowToGuide](https://github.com/gitname/ops-docs/blob/master/AuditLogHowToGuide.md)
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

An audit trail is captured to support access tracking and serviceability of
OpenBMC. The data logged requires the following elements:

- Log entry includes timestamp, user, network address, action performed and
  success or failure result.
- A single log captures login, logout, and modifying events.
- The logged data must not include protected information (e.g. passwords).
- The log entries can be extracted for analysis both on the runtime OpenBMC and
  remotely to another server.
- The logged data is preserved over long periods of time. E.g. Preservation of 6
  months of audit log data maintained.

The mechanism to create this audit trail has the following requirements:

- Simple to add logging entry points.
- Enablement of the logging is a compile-time option.
- The amount of data retained by the log can be configured.
- Minimal performance impact when logging is enabled.
- Small affect on the binary size.

## Proposed Design

In order to support all of the requirements three pieces are needed:

1. A backend service to store and process the audit entries,
2. the callouts to capture the bmcweb events in the audit trail, and
3. dump services to extract audit entries.

### Audit Backend Service

A new D-Bus service (xyz.openbmc_project.logging.AuditLog) of phosphor-logging
handles D-Bus requests to log events as well as D-Bus requests to gather the
recorded logged events. This service is a wrapper around the Linux auditd
service.

Manojkiran Eda proposed using the Linux auditd service as the backend for
gathering and analyzing the log data. See
[Add audit events in bmcweb](https://gerrit.openbmc.org/c/openbmc/bmcweb/+/55532)
for history. As Manojkiran Eda noted in the referenced proof of concept the
Linux Kernel Audit Subsystem provides many benefits:

- It is well maintained.
- It is a proven solution for capturing and streaming audit logs.
- It has a readily available package and recipe in Yocto.
- It provides much flexibility and many options for configuration.

Another benefit of enabling and using the Linux auditd subsystem is that the
configuration can be extended to log other events outside of bmcweb. For example
details for auditing ssh login sessions are not included as part of this design.
However the Linux auditd subsystem can be configured to log ssh sessions. This
answer in StackExchange on the
[Logging SSH access attempts](https://unix.stackexchange.com/a/355962) provides
details and notes the logging is done automatically in a SELinux environment.

The Linux auditd subsystem also provides a mechanism for adding services to
handle audit events during realtime via
[audispd](https://linux.die.net/man/8/audispd). This provides the means to
extend the handling of the audit events in the future. (E.g.
[Sending email when audisp program sees event](http://security-plus-data-science.blogspot.com/2017/04/sending-email-when-audisp-program-sees.html).)

#### AuditLog Methods

```
- name: LogEvent
  description: Record new event in log
  parameters:
    - name: operation
      type: string
      description: The operation being performed. E.g. Redfish path
    - name: username
      type: string
      description: The user account name performing the operation.
    - name: ipAddress
      type: string
      description: The network address for the user.
    - name: hostname
      type: string
      description: The hostname of the BMC.
    - name: result
      type: enum[success|fail]
      description: The result of the operation.
    - name: detailData
      type: string
      description: Additional data describing the operation.
```

```
- name: GetEntry
  description: Parses previously recorded log events into JSON LogEntry Redfish format.
  returns:
    - name: logFd
      type: unixfd
      description: The read-only file descriptor of the JSON formatted log events.
```

##### GetEntry Details

The
[SPEC Audit Event Parsing Library](https://github.com/linux-audit/audit-documentation/wiki/SPEC-Audit-Event-Parsing-Library)
provides utility functions meant to be used by external applications. The
_GetEntry_ method will parse the audit log entries into the JSON LogEntry
Redfish format using the utility functions of this library. The audit log events
can be retrieved by a client using a Redfish API. Doing so allows a GUI
(graphical user interface) such as webui-vue to display the audit events and to
provide additional function to allow filtering of the events based on time of
the event, Redfish schema, or other helpful event detail.

There is an existing Redfish Schema which provides the elements needed to
communicate with the client for accessing the audit log events. The
[LogService Schema](https://redfish.dmtf.org/schemas/v1/LogService.v1_4_0.json)
provides the structure needed. The _Security_ type for _LogPurpose_ is a good
fit for the data tracked by the audit log.

```
"@odata.id" : "/redfish/v1/Managers/bmc/LogServices/AuditLog"
"LogPurpose" : "Security"
"LogDiagnosticDataType" : "Manager"
```

The _#LogService.CollectDiagnosticData_ action of the LogServices schema is used
to extract the log records. There is an existing Redfish Schema for the format
of extracted log entries. The
[LogEntry Schema](https://redfish.dmtf.org/schemas/v1/LogEntry.v1_14_0.json)
provides the structure needed. The use of query parameters will allow for
receiving subsets of the entries.

```
// URI for extracting the log records
"@odata.id" : "/redfish/v1/Managers/bmc/LogServices/AuditLog/Actions/LogService.CollectDiagnosticData/"

// Each audit log entry has a unique audit event id. This can be used here.
"@odata.id" : "/redfish/v1/Managers/bmc/LogServices/AuditLog/Entries/<auditEventID>"
"Id" : "<auditEventID>"
"EntryType" : "Event"
"EventTimestamp" : "<Extracted timestamp from audit log entry>"
"Message" : "<Extracted message from audit log entry>"
"MessageId" : "<New MessageId for openbmc_message_registry>"
```

The _GetEntry_ method of the D-Bus xyz.openbmc_project.logging.AuditLog service
returns a file descriptor for a file containing the parsed entries. This is
similar to [Entry::getEntry()]
(https://github.com/openbmc/phosphor-logging/blob/19257fd2bc9243e0b6877e5ac2b223d21762b068/elog_entry.cpp#L75).
Bmcweb will read this file to build the response. Accessing the file is similar
to
[redfish::requestRoutesDBusEventLogEntryDownload()](https://github.com/openbmc/bmcweb/blob/2c6ffdb08b2207ff7c31041f77cc3755508d45c4/redfish-core/lib/log_services.hpp#L2003).
A file descriptor is returned due to the number of entries that might be part of
the log.

### Bmcweb Event Capture

Redfish DELETE, PATCH, POST, and PUT events associated to a user are captured
using the auditd kernel subsystem. Adding a callout within _completeRequest()_
provides a convenient single location to capture these events. Future events
will also be easily handled in the same manner.

Both failed and successful user login events are recorded using auditd. In order
to record both types, the callout to record these events is done on return from
attempting authentication (currently _pamAuthenticateUser()_) rather than in
_completeRequest()_.

A meson build option is added to control whether the event auditing is turned on
or not when bmcweb is compiled. The option is disabled by default. When disabled
the audit entry points are not compiled into the bmcweb binary.

The following diagram illustrates the flow for capturing the events.

1. Redfish event request is routed to bmcweb to process.
2. After authentication attempt for login event, or after event processing is
   completed the bmcweb will asynchronously call the _LogEvent_ method to record
   the audit data.
3. The AuditLog service method _LogEvent_ packages the event data into the
   format needed for the audit subsystem.
4. Using a NETLINK_AUDIT socket connection the audit event data is transferred
   to the kernel's audit subsystem for recording.
5. The audit subsystem records the data internally and returns to the caller.
   This in turn returns to bmcweb allowing the finalization of the response.
6. The auditd daemon monitors incoming audit events asynchronously from the
   sender of the events.
7. The auditd daemon records the incoming audit events to disk under
   /var/log/audit

```
+-------------------------------------------------+
|              Redfish Request                    | (1)
+----+-------------------+------------------------+
     |                   |
     |                   |
+----+-------------------+------------------------+
|    |                   |               bmcweb   | (2)
|  +-v-----+      +------v-----+                  |
|  | login |      | handle()   |                  |
|  +-+-----+      +-----+------+                  |
|    |                  |                         |
|    |            +-----v-------------+           |
|    |            | completeRequest() |           |
|    |            ++------------------+           |
|    |             |                              |
|    |             |                              |
+----+-------------+------------------------------+
     |             |
     | D-Bus       |
     | async       |
     |             |
+----+-------------+------------------------------+
|    |             |                              | (3)
| +--v-------------v---+       Audit  Service     |
| |LogEvent()          |                          |
| |                    |                          |
| +---+--------------^-+                          |
|     |              |                            |
|     |              |                            |
|     |              |                            |
+-----+--------------+----------------------------+
      |              |
      |NETLINK_AUDIT |
      | (socket)     |
+-----+--------------+------------------------------+
|     | (4)          | (5)             Linux kernel |           +-----------------------+
| +---+--------------+--------------+               |           |                       |
| |   |              |              |               |           |  auditd Daemon        |
| | +-v--------------+--------+     |               |   async   |                       |
| | |audit_log_user_message() |     +---------------+-----------v                       |
| | +-------------------------+     |     (6)       |           |                       |
| |                                 |               |           +---------+-------------+
| |                  audit subsytem |               |                     |
| |                                 |               |                     | (7)
| |                                 |               |                     |
| +---------------------------------+               |               +-----v-------+
|                                                   |               |             |
|                                                   |               |  audit.log  |
|                                                   |               |             |
+---------------------------------------------------+               +-------------+
```

### Audit Dump Service

The
[phosphor-debug-collector shell script dreport](https://github.com/openbmc/phosphor-debug-collector/blob/master/tools/dreport.d/README.md)
includes the audit logs as part of a user type dump. The audit logs can be
analyzed using the Linux auditd utilities such as ausearch on a remote Linux
system.

## Alternatives Considered

The requirement of preserving the log data over long periods of time precludes
an in-memory only solution. Additionally using any of the existing logging tools
is problematic with the amount of data to be preserved for an audit trail.

There have been at least two prior attempts to include audit logging of bmcweb
events in the upstream OpenBMC. The conversations around these raised different
issues and neither have been implemented.

### **Crow Middleware Approach**

This approach was abandoned. From comments it appears mainly due to use of
middleware and this approach being considered a short-term solution. See
[REST and Redfish Traffic Logging](https://gerrit.openbmc.org/c/openbmc/bmcweb/+/22699)
for history.

### **Independent Backend Logger**

This approach has never been implemented. A version of this design was reviewed
and merged. See
[phosphor-audit.md](https://github.com/openbmc/docs/blob/master/designs/phosphor-audit.md)
for design details, and
[23870: design: add phosphor-audit design doc](https://gerrit.openbmc.org/c/openbmc/docs/+/23870)
for review history.

Later an update was proposed for this design. There were many unresolved
comments on the updated design. Many of these were specific to the configuration
and implementation of the new logger backend service. The update seems to have
been abandoned mainly due to inaction of addressing the issues. See
[46023: design: update phosphor-audit design doc](https://gerrit.openbmc.org/c/openbmc/docs/+/46023)
for community conversation on design issues.

Both of these alternative solutions shared the common approach of implementing a
backend subsystem for recording auditable events. This would require the
addition of the IP address and user into the bmcweb D-Bus requests in order to
log this information. It would also require the addition of logging by the PAM
module or any other authentication replacement.

Centering the audit logging within bmcweb is cleaner as it already has all of
the information needed to be logged.

The Linux auditd subsystem provides the backend subsystem which will save the
effort required to implement and maintain a new subsystem. Additionally, it can
be configured to adjust how often the records are written to disk along with
other options. These configuration options give individual OpenBMC distributions
the ability to tailor the auditing to their individual needs. The
[SUSE: Understanding Linux Audit](https://documentation.suse.com/sles/12-SP4/html/SLES-all/cha-audit-comp.html)
article describes these options well.

This design initially considered calling the auditd subsystem functions directly
from the bmcweb. There are concerns over bmcweb not being able to process any
other requests while waiting for the auditd subsystem to respond. Addition of
D-Bus service to interface with the auditd subsystem is added to alleviate these
concerns. The trade off with this approach is an additional D-Bus call needed to
capture the events instead of a direct call to the kernel.

## Impacts

A prototype was created to perform the logging using auditd and calling it
directly within bmcweb. The resulting image built results in an image size 340K
larger than the image built without enabling auditing.

Minimal performance impacts have been observed using this prototype.

### Redfish Validator Performance Impact

The Redfish Validator was run four times each against an image built with Audit
Logging enabled and again disabled using the same hardware for all runs. Minimal
logging was recorded as the validator is mostly doing GET operations which are
not logged, just the session connections were logged. The statistics across the
runs demonstrate the logging does not have a performance impact on non-logged
events within bmcweb:

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
- Which repositories are expected to be modified to execute this design? bmcweb,
  phosphor-debug-collector, and phosphor-logging

## Testing

Existing tests shall still pass as if this feature is not introduced.

New unit tests:

bmcweb CI:

- log_services_dump_test.cpp: Add additional test for new AuditLog LogServices
  type. Similar testing as is currently done for other types.
- Add tests for all new utility functions created for handling audit log
  entries.

phosphor-logging CI:

- Add tests for new AuditLog methods

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
