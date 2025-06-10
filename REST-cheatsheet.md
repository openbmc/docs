# OpenBMC REST cheat sheet

This document is intended to provide a set of REST client commands for OpenBMC
usage. OpenBMC REST is disabled by default in bmcweb. For more information, see
<https://github.com/openbmc/bmcweb/commit/47c9e106e0057dd70133d50e928e48cbc68e709a>.
Most of the functionality previously provided by OpenBMC REST is available in
Redfish.

## Using CURL commands

### Notes on authentication

The original REST server, from the phosphor-rest-server repository, uses
authentication handled by the curl cookie jar files. The bmcweb REST server can
use the same cookie jar files for read-only REST methods like GET, but requires
either an authentication token or the username and password passed in as part of
the URL for non-read-only methods.

Starting with the 2.7 OpenBMC release (August 2019), bmcweb is the default REST
server. The phosphor-rest-server repository was archived in October 2022.

### Establish REST connection session

- Using just the cookie jar files for the phosphor-rest server:

```bash
 export bmc=xx.xx.xx.xx
 curl -c cjar -b cjar -k -H "Content-Type: application/json" -X POST https://${bmc}/login -d "{\"data\": [ \"root\", \"0penBmc\" ] }"
```

- If passing in the username/password as part of the URL, no unique login call
  is required. The URL format is:

```bash
  <username>:<password>@<hostname>/<path>...
```

For example:

```bash
 export bmc=xx.xx.xx.xx
 curl -k -X GET https://root:0penBmc@${bmc}/xyz/openbmc_project/list
```

- Token based authentication.

```bash
 export bmc=xx.xx.xx.xx
 export token=`curl -k -H "Content-Type: application/json" -X POST https://${bmc}/login -d '{"username" :  "root", "password" :  "0penBmc"}' | grep token | awk '{print $2;}' | tr -d '"'`
 curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/...
```

The third method is recommended.

### Commands

Note: To keep the syntax below common between the phosphor-rest and bmcweb
implementations as described above, this assumes that if bmcweb is used it is
using the 'Token based' login method as described above:

```bash
export bmc=xx.xx.xx.xx
export token=`curl -k -H "Content-Type: application/json" -X POST https://${bmc}/login -d '{"username" :  "root", "password" :  "0penBmc"}' | grep token | awk '{print $2;}' | tr -d '"'`
curl -k -H "X-Auth-Token: $token" https://$bmc/xyz/openbmc_project/...
```

- List and enumerate:

```bash
 curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/list
 curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/enumerate
```

- List sub-objects:

```bash
 curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/
 curl -k -H "X-Auth-Token: $token" https://${bmc}/xyz/openbmc_project/state/
```

- Host soft power off:

```bash
 curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -d '{"data": "xyz.openbmc_project.State.Host.Transition.Off"}' -X PUT https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
```

- Host hard power off:

```bash
 curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT -d '{"data":"xyz.openbmc_project.State.Chassis.Transition.Off"}' https://${bmc}//xyz/openbmc_project/state/chassis0/attr/RequestedPowerTransition
```

- Host power on:

```bash
 curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -d '{"data": "xyz.openbmc_project.State.Host.Transition.On"}' -X PUT https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
```

- Reboot Host:

```bash
 curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT -d '{"data":"xyz.openbmc_project.State.Host.Transition.Reboot"}' https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
```

- Reboot BMC:

```bash
 curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT -d '{"data":"xyz.openbmc_project.State.BMC.Transition.Reboot"}' https://${bmc}/xyz/openbmc_project/state/bmc0/attr/RequestedBMCTransition
```

- Display logging entries:

```bash
 curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X GET https://${bmc}/xyz/openbmc_project/logging/entry/enumerate
```

- Delete logging entries:

```bash
 curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X DELETE https://${bmc}/xyz/openbmc_project/logging/entry/<entry_id>
 curl -k -H "X-Auth-Token: $token" -H 'Content-Type: application/json' -X POST -d '{"data":[]}' https://${bmc}/xyz/openbmc_project/logging/action/DeleteAll
```

- Delete dump entries:

```bash
 curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X DELETE https://${bmc}/xyz/openbmc_project/dump/entry/<entry_id>
 curl -k -H "X-Auth-Token: $token" -H 'Content-Type: application/json' -X POST -d '{"data":[]}' https://${bmc}/xyz/openbmc_project/dump/action/DeleteAll
```

- Delete images from system:

  - Delete image:

```bash
 curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X POST -d '{"data": []}' https://${bmc}/xyz/openbmc_project/software/<image id>/action/Delete
```

- Delete all non-running images:

```bash
 curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X POST -d '{"data": []}' https://${bmc}/xyz/openbmc_project/software/action/DeleteAll
```

- Clear gard records:

```bash
 curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X POST -d '{"data":[]}' https://${bmc}/org/open_power/control/gard/action/Reset
```

- Control boot source override:

  - Read current boot source override settings:

```bash
 curl -k -H "X-Auth-Token: $token"  https://${bmc}/xyz/openbmc_project/control/host0/boot/enumerate
```

- Set boot source:

```bash
 curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT https://${bmc}/xyz/openbmc_project/control/host0/boot/attr/BootSource -d '{"data": "xyz.openbmc_project.Control.Boot.Source.Sources.Default"}'
```

- Set boot mode:

```bash
 curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT https://${bmc}/xyz/openbmc_project/control/host0/boot/attr/BootMode -d '{"data": "xyz.openbmc_project.Control.Boot.Mode.Modes.Regular"}'
```

- Set boot type (valid only if host is based on the x86 CPU):

```bash
 curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT https://${bmc}/xyz/openbmc_project/control/host0/boot/attr/BootType -d '{"data": "xyz.openbmc_project.Control.Boot.Type.Types.EFI"}'
```

- Set boot source override persistent:

```bash
 curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT https://${bmc}/xyz/openbmc_project/control/host0/boot/one_time/attr/Enabled -d '{"data": "false"}'
```

- Enable boot source override:

```bash
 curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT https://${bmc}/xyz/openbmc_project/control/host0/boot/attr/Enabled -d '{"data": "true"}'
```

- Set NTP and Nameserver:

  Examples using public server.

  - NTP Server:

```bash
 curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT -d '{"data": ["pool.ntp.org"] }' https://${bmc}/xyz/openbmc_project/network/eth0/attr/NTPServers
```

- Name Server:

```bash
 curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT -d '{"data": ["time.google.com"] }' https://${bmc}/xyz/openbmc_project/network/eth0/attr/Nameservers
```

- Configure time ownership and time sync method:

  The introduction about time setting is here:
  <https://github.com/openbmc/phosphor-time-manager>

  Note: Starting from OpenBMC 2.6 (with systemd v239), systemd's timedated
  introduces a new beahvior that it checks the NTP services' status during
  setting time, instead of checking the NTP setting:

  -When NTP server is set to disabled, and the NTP service is stopping but not
  stopped, setting time will get an error.

  Before OpenBMC 2.4 (with systemd v236), the above will always succeed. This
  results in
  [openbmc/openbmc#3459](https://github.com/openbmc/openbmc/issues/3459), and
  the related test cases are updated to cooperate with this behavior change.

  - Read:

```bash
 curl -k -H "X-Auth-Token: $token" -X GET https://${bmc}/xyz/openbmc_project/time/owner/attr/TimeOwner
 curl -k -H "X-Auth-Token: $token" -X GET https://${bmc}/xyz/openbmc_project/time/sync_method/attr/TimeSyncMethod
```

- Write:

Time owner:

```bash
 curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X  PUT -d '{"data": "xyz.openbmc_project.Time.Owner.Owners.BMC" }' https://${bmc}/xyz/openbmc_project/time/owner/attr/TimeOwner
 curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X  PUT -d '{"data": "xyz.openbmc_project.Time.Owner.Owners.Host" }' https://${bmc}/xyz/openbmc_project/time/owner/attr/TimeOwner
 curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X  PUT -d '{"data": "xyz.openbmc_project.Time.Owner.Owners.Split" }' https://${bmc}/xyz/openbmc_project/time/owner/attr/TimeOwner
 curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X  PUT -d '{"data": "xyz.openbmc_project.Time.Owner.Owners.Both" }' https://${bmc}/xyz/openbmc_project/time/owner/attr/TimeOwner
```

Time sync method:

```bash
 curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X  PUT -d '{"data": "xyz.openbmc_project.Time.Synchronization.Method.NTP" }' https://${bmc}/xyz/openbmc_project/time/sync_method/attr/TimeSyncMethod
 curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X  PUT -d '{"data": "xyz.openbmc_project.Time.Synchronization.Method.Manual" }' https://${bmc}/xyz/openbmc_project/time/sync_method/attr/TimeSyncMethod
```

- Power Supply Redundancy:

  - Read:

```bash
 curl -k -H "X-Auth-Token: $token" -X GET https://${bmc}/xyz/openbmc_project/control/power_supply_redundancy
```

- Write (Enable/Disable):

```bash
 curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT https://${bmc}/xyz/openbmc_project/control/power_supply_redundancy/attr/PowerSupplyRedundancyEnabled -d '{"data": 1}'
 curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PUT https://${bmc}/xyz/openbmc_project/control/power_supply_redundancy/attr/PowerSupplyRedundancyEnabled -d '{"data": 0}'
```

- Factory Reset:

  - Factory reset host and BMC software:

```bash
 curl -k -H "X-Auth-Token: $token" -H 'Content-Type: application/json' -X POST -d '{"data":[]}' https://${bmc}/xyz/openbmc_project/software/action/Reset
```

- Factory reset network setting:

```bash
 curl -k -H "X-Auth-Token: $token" -H 'Content-Type: application/json' -X POST -d '{"data":[]}' https://${bmc}/xyz/openbmc_project/network/action/Reset
```

- Enable field mode:

```bash
 curl -k -H "X-Auth-Token: $token" -H 'Content-Type: application/json' -X PUT -d '{"data":1}' https://${bmc}/xyz/openbmc_project/software/attr/FieldModeEnabled
```

and then reboot BMC.
