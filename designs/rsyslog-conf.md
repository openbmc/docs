# Modular configuration for rsyslog

Author:
  Benjamin Fair (benjaminfair)

Primary assignee:
  Benjamin Fair (benjaminfair)

Other contributors:
  None

Created:
  2021-06-29

## Problem Description
The goal of this design is to create a modular, maintainable method for Bitbake
recipes to provide custom configuration rules to `rsyslog`.

## Background and References
There are three types of events that are currently stored in the `systemd`
journal which some users may want to persist to flash and be able to recover
after a BMC reboot: IPMI SEL entries, Redfish logs, and application crashes.

Currently,
[Facebook](https://github.com/openbmc/openbmc/blob/2a35c66601ea646d03f53f8ee1e0bea3acf179c1/meta-facebook/meta-tiogapass/recipes-extended/rsyslog/rsyslog/rsyslog.conf),
[Ampere](https://github.com/openbmc/openbmc/blob/2a35c66601ea646d03f53f8ee1e0bea3acf179c1/meta-ampere/meta-common/recipes-extended/rsyslog/rsyslog/rsyslog.conf),
[Intel](https://github.com/openbmc/openbmc/blob/2a35c66601ea646d03f53f8ee1e0bea3acf179c1/meta-intel-openbmc/meta-common/recipes-extended/rsyslog/rsyslog/rsyslog.conf),
and
[Fii](https://github.com/openbmc/openbmc/blob/2a35c66601ea646d03f53f8ee1e0bea3acf179c1/meta-fii/meta-kudo/recipes-extended/rsyslog/rsyslog/rsyslog.conf)
all ship very similar configuration files for `rsyslog` that persist different
combinations of these event types. Because of this duplication, changes or
improvements to this configuration are unlikely to be incorporated into the
other users' versions.

For other configuration files that relate to specific daemons in OpenBMC, such as
service files and D-Bus ACLs, the best practice is to store and maintain these
files within the repositories that they apply to rather than in machine
metadata layers.

## Requirements
 * Eliminate the duplicated portions of the existing `rsyslog.conf` files
 * Enable future users of `rsyslog` to easily persist new types of events
 * Avoid installing or enabling features of `rsyslog` on platforms that do not
   require those features

## Proposed Design
The portion of `rsyslog.conf` that pertains to IPMI SEL will be moved to a
configurtion file in the
[phosphor-sel-logger](https://github.com/openbmc/phosphor-sel-logger) repo.
Similarly, the Redfish event and application crash configurations will move to
files in the [bmcweb](https://github.com/openbmc/bmcweb) repo. By default these
will not be installed into the final image, but a compile-time configuration
option will cause them to be installed into `/etc/rsyslog.d/`.

We will use the `PACKAGECONFIG` system in Bitbake to add `rsyslog` support as an
option to the recipes for these daemons. This will install the relevant
configuration files and add a dependency to the `rsyslog-imjournal` package,
which is split off from the main `rsyslog` package. `rsyslog-imjournal` contains
the plugin file for `imjournal` (needed for full support of the `systemd`
journal) and a configuration file that loads and configures this plugin.

Finally, the default Phosphor-provided `rsyslog.conf` will load all of the
configuration files on the system in order if present, starting with
`imjournal`, then the event types mentioned above, and finally the custom
logging configuration which `phosphor-logging` may generate at runtime.

## Alternatives Considered
The main alternative is to continue with the existing method of copying and
pasting `rsyslog.conf` into the metadata layer for each user of `rsyslog`. This
is currently four, but is likely to continue increasing over time. As mentioned
in the Background section, this solution scales poorly and is difficult to
maintain. Further, it does not follow the best practices of the rest of OpenBMC.

A second alternative is to keep all of the `rsyslog` configuration modules in
`meta-phosphor` and provide options to enable them as desired. However, this
approach is contrary to the general OpenBMC pattern of keeping and managing
application-specific configuration files in the repositiories for those
applications, as seen with `systemd` service files and `d-bus` policies.

## Impacts
There should be no impact to current machines since each user will still have
the same configuration, although split into multiple files rather than a
monolithic configuration.

## Testing
The primary testing that I will perform is simply building and examining the
resulting images. Functional testing will require assistance from people with
access to the relevant machines from Intel, Ampere, Facebook, and Fii.
