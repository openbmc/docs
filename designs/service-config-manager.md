# Service Config Manager Redesign

Author: Andrew Geissler (geissonator)

Other contributors: None

Created: May 15th, 2024

## Problem Description

The [service-config-manager][service-config-manager] provides D-Bus [APIs][APIs]
that are basically wrappers around the systemd D-Bus APIs to enable and disable
services. This provides minimal benefit and no real abstraction from systemd or
the details of enabling and disabling services.

For example, to disable the IPMI service, the caller must iterate through all
D-Bus service-config-manager objects, looking for hard coded IPMI wild card
service names under the `/xyz/openbmc_project/control/service` object path and
then set the `Running` and `Enabled` attributes on each object found. So if you
have IPMI running on eth0 and eth1, you need to find both objects and set the
appropriate bits on both. An example of this dbus object is
`/xyz/openbmc_project/control/service/_70hosphor_2dipmi_2dnet_40eth0`

Determining if a service is enabled/disabled follows a similar path, but gets
even more confusing if you run into situations where one interface is enabled
and another is [not][multi example]. There is no useful abstraction provided.

Another problem with the service-config-manager is for it to provide the
interface for disabling or enabling of a service, it must see the corresponding
systemd service(s) run. So if you have a system where you disable IPMI by
[default][disable example], the service-config-manager will never provide you a
mechanism to enable it.

And finally with the introduction of [redundant BMCs][red bmcs], this becomes
even more difficult to manage as there is no easy way to replicate service
config policies to another BMC give the current implementation of utilizing
systemd directly to store the configuration policy.

## Background and References

The service-config-manager could be a useful function within OpenBMC for
abstracting the enabling and disabling of optional services. Similar
repositories like [phosphor-state-manager][psm] and [x86-power-control][x86-pc]
provide full abstraction around systemd service and targets by implementing
D-Bus properties to control the powering on and off of the chassis and host. If
OpenBMC were to some day move away from systemd, that would be abstracted from
the users of PSM and x86-power-control.

## Requirements

1. Provide true abstraction for the enabling and disabling of services
   - A user will have a single property on D-Bus for enabling and disabling a
     service
   - The current per-systemd service concept will be reduced to a single D-Bus
     object per functional service
   - The current properties, Enabled, Masked, and Running will be reduced to a
     single property (Enabled) and users will set that boolean to true/false
2. There will be no requirement that a service has be started once in order to
   be enabled/disabled

## Proposed Design

For each service supported, create a D-Bus object with a boolean Enabled
property.

- Supported services are SSH, HTTPS, IPMI (net,kcs), USB, IPKVM, and
  host-console (over ssh)
  - These properties and their value will be persisted to the BMC filesystem

Support for the [SocketAttributes][socket attr] will be deprecated as there are
no valid use cases for it.

The only user of the service-config-manager is bmcweb and a little within IPMI
so the new D-Bus APIs would be introduced in parallel with the existing ones.
bmcweb would then be switched over to the new APIs and the old ones will be
removed.

Underneath the covers, the implementation will continue to utilize systemd D-Bus
APIs to enable/disable and start/stop the corresponding services.

A good chunk of the existing code will be reused here, it will just be
refactored a bit around this new simplified enable/disable API.

So instead of current code like this to disable IPMI:

```
controlServiceObjs = mapper(get all xyz.openbmc_project.Control.Service.Manager)
for(auto i : controlServiceObjs)
  if i.starts_with("phosphor-ipmi")
    setDbusProperty(..."Running", false)
    setDbusProperty(..."Enabled", false)
)
```

We move to this:

```
setDbusProperty(xyz.openbmc_project.Control.Service.Manager,
                /xyz/openbmc_project/control/service/ipmi, Enabled, false)
```

## Alternatives Considered

Move the existing service-config-manager function into bmcweb. bmcweb has a
precedent of directly interacting with systemd D-Bus APIs. It would be a cleaner
design to have bmcweb directly do this vs. the current design. This would
however put additional code complexity in an already large and complex piece of
software. There is also a requirement of persistency here (remembering if a user
has enabled or disabled a service) that may not always be correct by querying
systemd and bmcweb does not maintain persistent data. There is enough complexity
and code here that abstracting it within a repository like
service-config-manager makes more sense. There is some IPMI use as well which
would require some code replication if we do away with the
service-config-manager.

## Impacts

A new API will be created and the previous ones will be removed. There is
minimal impact of doing this due to bmcweb and IPMI being the only uses sof the
service-config-manager.

Documentation will need to be updated in both the service-config-manager
repository and phosphor-dbus-interfaces.

From a performance perspective, under the covers the implementation will remain
the same, enabling and disabling systemd services, so no changes in this area.

Assuming a firmware update preserves the systemd status, the new code will
detect its settings are default and discover the initial values based on
querying the corresponding systemd service states.

### Organizational

- Does this repository require a new repository? No
- Who will be the initial maintainer(s) of this repository? I will propose to
  add myself (geissonator) as a maintainer once the transition is complete
- Which repositories are expected to be modified to execute this design?
  service-config-manager, bmcweb, phosphor-dbus-interfaces
- Make a list, and add listed repository maintainers to the gerrit review.

## Testing

- Validate moving from the old implementation to the new with a mismatch of
  services enabled and disabled
- Ensure each supported service can be enabled and disabled
- Validate different code update, BMC reboot, and factory reset scenarios

[service-config-manager]: https://github.com/openbmc/service-config-manager
[APIs]:
  https://github.com/openbmc/phosphor-dbus-interfaces/tree/master/yaml/xyz/openbmc_project/Control/Service
[multi example]: https://gerrit.openbmc.org/c/openbmc/bmcweb/+/62229
[disable example]: https://gerrit.openbmc.org/c/openbmc/openbmc/+/54680
[red bmcs]: https://gerrit.openbmc.org/c/openbmc/docs/+/70233
[psm]: https://github.com/openbmc/phosphor-state-manager
[x86-pc]: https://github.com/openbmc/x86-power-control
[socket attr]:
  https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/Control/Service/SocketAttributes.interface.yaml
