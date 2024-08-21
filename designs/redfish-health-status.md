# Redfish Health status

Author: Akshat Jain <akshatzen@google.com>
Other contributors: Tom Chan <kpjhan@google.com>, Yushi Sun <yushis@google.com>

Created: Aug 20, 2024

## Problem Description

BMC has access to many health signals via GPIOs and various sensors. Any user
or monitoring software would like to read the redfish resource health status.


## Background and References

GPIOs provide useful system health information about the HW problem. In order
to surface these health signals to BMCWeb (Redfish), we will define a D-bus
interface which be queried by BMCWeb to provide Health status of resources.

phosphor-gpio-monitor will monitor GPIOs and update the D-bus interface to
reflect the resource status.

## Requirements

1. Any monitoring daemon e.g. phosphor-gpio-monitor that provides health status
   needs to update the health status based on the defined D-bus interface.
2. BMCWeb will need to read the status property of the defined interface to
   infer the resource health status.

## Proposed Design

We will add another daemon `gpioHealthMon` in phosphor-gpio-monito to monitor
the health of resources via GPIO and surface the failure condition in the
“Status:Health” property of Redfish resources.

BMC’s high level behavior will be as follows:
If the HealthMonitor service detects a problem, then bmcweb will provide
problem details in the Status:HealthRollup field of the corresponding
redfish resources.

Status:Health property value will indicate health state of the resource:
“OK” - indicates Normal health.
“Critical” - indicates a critical condition that requires immediate attention.
“Warning” - indicates a condition that requires attention.

Conditions array will provide additional information when Health != OK.

Example below provides sample redfish status for Fan0 fault condition for
Baseboard chassis:

```
"@odata.id": "/redfish/v1/Chassis/Baseboard/ThermalSubsystem/Fans/Fan0",
"@odata.type": "#Chassis.v1_15_0.Chassis",
"Id": "Baseboard",
"Name": "Baseboard",
"Status": {
    "Health": "Critical",
    "Conditions": [
      {
        "MessageId": "ResourceEvent.1.0.ResourceErrorsDetected",
        "Message": "The resource property Fan0 has detected errors of type eFuse Failure.",
        "MessageArgs": ["Fan0", "eFuse Failure"],
        "Timestamp": "2022-3-16T14:45:00Z",
        "Severity": "Critical",
      }
    ]
    "State": "Enabled"
}
```

### Flow of events

                                                    Query resource
                                                               A
                                                               |
 +--------------------------------------------------------------------------+
 | BMC                                                         |            |
 |                                                             |            |
 |  +-------------------------------------------------------------------+   |
 |  | BMCWeb                                                   |        |   |
 |  |   +-----------------------------------------------------------+   |   |
 |  |   | Redfish-core/lib/health.hpp                          |    |   |   |
 |  |   |                                                      |    |   |   |
 |  |   |                                                      |    |   |   |
 |  |   |      +----------------------------------+            |    |   |   |
 |  |   |      |  Redfish Resource to             |<-----------+    |   |   |
 |  |   |      |  Entity manager entity map       |                 |   |   |
 |  |   |      +----------------------------------+                 |   |   |
 |  |   |           A                                               |   |   |
 |  |   |           |        +---------------------------------+    |   |   |
 |  |   |           |        | Entity health status Cache      |    |   |   |
 |  |   |           |        +---------------------------------+    |   |   |
 |  |   |           |                        A                      |   |   |
 |  |   |           |                        |                      |   |   |
 |  |   |           |         +---------------------------------+   |   |   |
 |  |   |           |         | Signal handler:                 |   |   |   |
 |  |   |           |         | statusPropertyChanged           |   |   |   |
 |  |   |           |         +---------------------------------+   |   |   |
 |  |   |           |                                         A     |   |   |
 |  |   |           |                                         |     |   |   |
 |  |   |     +-----------------------------------------+     |     |   |   |
 |  |   |     | Signal handler: interface added         |     |     |   |   |
 |  |   |     | xyz.openbmc_project.Configuration.      |     |     |   |   |
 |  |   |     | HealthEvaluationCriteria                |     |     |   |   |
 |  |   |     +-----------------------------------------+     |     |   |   |
 |  |   |                 A                                   |     |   |   |
 |  |   +-----------------|-----------------------------------|-----+   |   |
 |  |                     |                                   |         |   |
 |  +---------------------|-----------------------------------|---------+   |
 |                 Signal |                                   | Signal      |
 |  +---------------------|-----------------------------------|---------+   |
 |  | D-bus               |                                   |         |   |
 |  |    +---------------------------+  +----------------------------+  |   |
 |  |    | EntityManager's           |  | phosphor-gpio-monitor's    |  |   |
 |  |    | Health evaluation criteria|  | Health status objects      |  |   |
 |  |    | objects                   |  |                            |  |   |
 |  |    +---------------------------+  +----------------------------+  |   |
 |  |        A             |                                  A         |   |
 |  +--------|-------------|----------------------------------|---------+   |
 |           |             |                                  |             |
 |           |             +---------+                        +------+      |
 |           |                       |                               |      |
 |  +-------------------+   +--------|-------------------------------|--+   |
 |  |  EntityManager    |   |        |     phosphor-gpio-monitor     |  |   |
 |  |                   |   |        V                               |  |   |
 |  |    +------------+ |   |  +-----------------------------------+ |  |   |
 |  |    | Json config| |   |  | Signal handler: interface added   | |  |   |
 |  |    +------------+ |   |  | xyz.openbmc_project.Configuration | |  |   |
 |  +-------------------+   |  | HealthEvaluationCriteria          | |  |   |
 |                          |  +-----------------------------------+ |  |   |
 |                          |                                        |  |   |
 |                          |                  +----------------------+ |   |
 |                          |                  | GPIO event handler   | |   |
 |                          |                  +----------------------+ |   |
 |                          +-------------------------------------------+   |
 |                                                                          |
 +--------------------------------------------------------------------------+

### Entity Manager

Entity manager will populate FRUs and corresponding health evaluation criteria
objects in D-bus.

To do that entity manager configuration will contain an “exposes” record
providing health evaluation criteria for each entity. Each entity can have
multiple health evaluation criterias.

Name: HealthCriteriaID - Identifier for the health criteria should be unique
for a gBMC instance.
GpioLineName: GPIO line name
GpioHealthyPolarity: Polarity of the GPIO that indicates the entity is healthy
HealthSeverity: Used to set Health field in redfish resource’s Status:Health
FailureMessageId: MessageId to be used in the Status:Conditions[].
OkMessageId: MessageId to be used in redfish events when resource recovers.
MessageArgs: Arguments for the redfish event message
Resolution: String providing resolution recommendation
EntityName: Same as Entity manager Name field. E.g. “Fan0”
EntityType: Same as Entity manager type field. E.g. “fan”
Type: Will always be HealthEvaluationCriteria for our Exposes record.

Example:
```
    "Exposes": [
        {
            "Name": "FAN0_EFUSE_PROBLEM",
            "GpioLineName": "PGOOD_12V_FAN0_EFUSE",
            "GpioHealthyPolarity": "Rising",
            "HealthSeverity": "CRITICAL",
            "FailureMessageId": "ResourceEvent.1.0.ResourceErrorsDetected",
            "OkMessageId": "ResourceEvent.1.0.ResourceErrorsCorrected",
            "MessageArgs": [“Fan0”, “Efuse failure”],
            “Resolution” : “Replace”,
            "RelatedEntityName": "Fan0",
            "RelatedEntityType": "fan",
            "Type": "HealthEvaluationCriteria"
        }
     ]
```

### phosphor-gpio-monitor

Algorithm:
1. Upon service initialization the phosphor-gpio-monitor will wait for objects
with xyz.openbmc_project.Configuration.HealthEvaluationCriteria interface to be
populated by Entity manager.
2. For each objects with
   xyz.openbmc_project.Configuration.HealthEvaluationCriteria interface is
   discovered expose a health status object with Healthy and toggle_count
   properties and xyz.openbmc_project.HealthMonitor.GetStatus interface.
3. phosphor-gpio-monitor will poll each GPIO once and update the initial state
   of health on D-bus.
4. Start monitoring GPIO events. Then wait for GPIO events to occur and update
   the health status.  When a GPIO event is observed, it will emit signal
   statusPropertyChanged.


Health status object properties:
1. Healthy: (string) property indicating healthy status, `true` is healthy,
   `false` is unhealthy. Upon healthy property change the service will emit a signal
   “healthyPropertyChanged”.
2. toggle_count: (integer) property indicating count of state change events.
   The purpose is to identify flaky signals.

### BMCWeb

Algorithm:
1. BMCWeb will first find all HealthEvaluationCriteria by reading once and then
   subscribing to interface added signal for
   xyz.openbmc_project.Configuration.HealthEvaluationCriteria interface.
2. For each HealthEvaluationCriteria BMCWeb will find objects providing
   xyz.openbmc_project. HealthMonitor.GetStatus interface and keep the status
   updated via property change signal healthyPropertyChanged.  BMCWeb will keep
   the two caches updated by D-bus signal handling:
   2.1. Resource to Entity manager entity map for each HealthEvaluationCriteria
   2.2. Resource Health status information
3. Upon First Redfish resource query, for a Redfish resource associated with
   Entities that have associated HealthEvaluationCriteria a new mapping will be
   created and a D-bus read will be done to check the health status.
5. For subsequently redfish queries there will be no D-bus read, all the
   the health status will be provided from the cache (updated via signal
   handling).
6. A GPIO event will cause the statusPropertyChanged signal to be handled in
   gBMCWeb.

## Alternatives Considered

Considered following alternatives:

1. gpiosensor: a dedicated daemon to report gpio status to dbus
Ref: https://gerrit.openbmc.org/c/openbmc/dbus-sensors/+/45997

Although this project is specifically sensing GPIO and surfaces it over Dbus
but seems to be not accepted yet.

2. host-error-monitor
Ref: https://github.com/openbmc/host-error-monitor

Not suitable for GPIO monitoring.

3. phosphor-health-monitor
Ref: https://github.com/openbmc/phosphor-health-monitor

Not suitable for GPIO monitoring.

## Impacts

No new repo will be created.
BMCWeb health status was removed, but would need to added again.

Ref: 70450: Remove redfish-health-populate
https://gerrit.openbmc.org/c/openbmc/bmcweb/+/70450

phosphor-gpio-monitor repo will add a new daemon for this proposal.

## Organizational

Does this repository require a new repository? No
Who will be the initial maintainer(s) of this repository? Not applicable
Which repositories are expected to be modified to execute this design?
bmcweb, phosphor-dbus-interfaces and phosphor-gpio-monitor

## Testing

We will leverage phosphor-gpio-monitor's unit tests.

