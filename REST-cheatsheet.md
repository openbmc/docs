# OpenBMC REST cheat sheet

This document is intended to provide a set of REST client commands for OpenBMC usage.

## Using CURL commands

### Notes on authentication:
The original REST server, from the phosphor-rest-server repository, uses
authentication handled by the curl cookie jar files. The bmcweb REST server
can use the same cookie jar files for read-only REST methods like GET, but
requires either an authentication token or the username and password passed in
as part of the URL for non-read-only methods.

The phosphor-rest server will no longer be the default REST server after the
2.6 OpenBMC release.

### Establish REST connection session
* Using just the cookie jar files for the phosphor-rest server:
    ```
   $ export bmc=xx.xx.xx.xx
   $ curl -c cjar -b cjar -k -H "Content-Type: application/json" -X POST https://${bmc}/login -d "{\"data\": [ \"root\", \"0penBmc\" ] }"
    ```
* If passing in the username/password as part of the URL, no unique login call
  is required.  The URL format is:
  ```
    <username>:<password>@<hostname>/<path>...
  ```
  For example:
  ```
  $ export bmc=xx.xx.xx.xx
  curl -k -X GET https://root:0penBmc@${bmc}/xyz/openbmc_project/list
  ```
* Token based authentication.  There are two slightly different formats,
  which return the token differently.  In either case, the token is then
  passed in to future REST calls via the 'X-Auth-Token' or 'Authorization'
  headers.

  1. Get the authentication token from the header that comes back from the
     login when using the `data` username/password JSON dictionary and a
     `-i` argument to show headers:
     ```
     $ export bmc=xx.xx.xx.xx
     $ curl -i -k -H "Content-Type: application/json" -X POST https://${bmc}/login -d "{\"data\": [ \"root\", \"0penBmc\" ] }"
       HTTP/1.1 200 OK
       Set-Cookie: XSRF-TOKEN=4bmZdLP7OXgreUbazXN3; Secure
       Set-Cookie: SESSION=vnTdgYnvhTnnbirQizrr; Secure; HttpOnly
       ...

       $ curl -H "X-Auth-Token: vnTdgYnvhTnnbirQizrr" -X POST ...
       $ curl -H "Authorization: Token vnTdgYnvhTnnbirQizrr" -X POST ...
     ```
  2. Get the authentication token as part of the response when using a JSON
     dictionary with 'username' and 'password' keys:

     ```
     $ export bmc=xx.xx.xx.xx
     $ curl -k -H "Content-Type: application/json" -X POST https://${bmc}/login -d '{"username" :  "root", "password" :  "0penBmc"}'
      {
        "token": "g47HgLLlZWKLwiyuUwJM"
      }

      $ curl -H "X-Auth-Token: g47HgLLlZWKLwiyuUwJM -X POST ...
      $ curl -H "Authorization: Token g47HgLLlZWKLwiyuUwJM -X POST ...
     ```

### Commands
Note: To keep the syntax below common between the phosphor-rest and bmcweb
      implementations as described above, this assumes that if bmcweb
      is used it is using the `<username>:<password>@<host>` login method
      as desribed above:
   ```
      export bmc=<username>:<password>@xx.xx.xx.xx
   ```

* List and enumerate:
    ```
    $ curl -b cjar -k https://${bmc}/xyz/openbmc_project/list
    $ curl -b cjar -k https://${bmc}/xyz/openbmc_project/enumerate
    ```

* List sub-objects:
    ```
    $ curl -b cjar -k https://${bmc}/xyz/openbmc_project/
    $ curl -b cjar -k https://${bmc}/xyz/openbmc_project/state/
    ```

* Host soft power off:
    ```
    $ curl -b cjar -k -H "Content-Type: application/json" -d '{"data": "xyz.openbmc_project.State.Host.Transition.Off"}' -X PUT https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
    ```

* Host hard power off:
    ```
    $ curl -b cjar -k -H "Content-Type: application/json" -X PUT -d '{"data":"xyz.openbmc_project.State.Chassis.Transition.Off"}' https://${bmc}//xyz/openbmc_project/state/chassis0/attr/RequestedPowerTransition
    ```

* Host power on:
    ```
    $ curl -b cjar -k -H "Content-Type: application/json" -d '{"data": "xyz.openbmc_project.State.Host.Transition.On"}' -X PUT https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
    ```

* Reboot Host:
    ```
    $ curl -b cjar -k -H "Content-Type: application/json" -X PUT -d '{"data":"xyz.openbmc_project.State.Host.Transition.Reboot"}' https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
    ```

* Reboot BMC:
    ```
    $ curl -b cjar -k -H "Content-Type: application/json" -X PUT -d '{"data":"xyz.openbmc_project.State.BMC.Transition.Reboot"}' https://${bmc}//xyz/openbmc_project/state/bmc0/attr/RequestedBMCTransition
    ```

* Display logging entries:
    ```
    $ curl -b cjar -k -H "Content-Type: application/json" -X GET https://${bmc}/xyz/openbmc_project/logging/entry/enumerate
    ```

* Delete logging entries:
    ```
    $ curl -b cjar -k -H "Content-Type: application/json" -X DELETE https://${bmc}/xyz/openbmc_project/logging/entry/<entry_id>
    $ curl -b cjar -k -H 'Content-Type: application/json' -X POST -d '{"data":[]}' https://${bmc}/xyz/openbmc_project/logging/action/DeleteAll
    ```

* Delete dump entries:
    ```
    $ curl -b cjar -k -H "Content-Type: application/json" -X DELETE https://${bmc}/xyz/openbmc_project/dump/entry/<entry_id>
    $ curl -b cjar -k -H 'Content-Type: application/json' -X POST -d '{"data":[]}' https://${bmc}/xyz/openbmc_project/dump/action/DeleteAll
    ```

* Delete images from system:

    - Delete image:
    ```
    $ curl -b cjar -k -H "Content-Type: application/json" -X POST -d '{"data": []}' https://${bmc}/xyz/openbmc_project/software/<image id>/action/Delete
    ```

    - Delete all non-running images:
    ```
    $ curl -b cjar -k -H "Content-Type: application/json" -X POST -d '{"data": []}' https://${bmc}/xyz/openbmc_project/software/action/DeleteAll
    ```

* Clear gard records:
    ```
    $ curl -b cjar -k -H "Content-Type: application/json" -X POST -d '{"data":[]}' https://${bmc}/org/open_power/control/gard/action/Reset
    ```

* Set boot mode:
    ```
    $ curl -b cjar -k -H "Content-Type: application/json" -X PUT https://${bmc}/xyz/openbmc_project/control/host0/boot/one_time/attr/BootMode -d '{"data": "xy.openbmc_project.Control.Boot.Mode.Modes.Regular"}'
    ```

* Set boot source:
    ```
    $ curl -b cjar -k -H "Content-Type: application/json" -X PUT https://${bmc}/xyz/openbmc_project/control/host0/boot/one_time/attr/BootSource -d '{"data": "xyz.openbmc_project.Control.Boot.Source.Sources.Default"}
    ```

* Set NTP and Nameserver:

    Examples using public server.
    - NTP Server:
    ```
    $ curl -b cjar -k -H "Content-Type: application/json" -X PUT -d '{"data": ["pool.ntp.org"] }' https://${bmc}/xyz/openbmc_project/network/eth0/attr/NTPServers
    ```

    - Name Server:
    ```
    $ curl -b cjar -k -H "Content-Type: application/json" -X PUT -d '{"data": ["time.google.com"] }' https://${bmc}/xyz/openbmc_project/network/eth0/attr/Nameservers
    ```

* Configure time ownership and time sync method:

    - Read:
    ```
    $ curl -b cjar -k -X GET https://${bmc}/xyz/openbmc_project/time/owner/attr/TimeOwner
    $ curl -b cjar -k -X GET https://${bmc}/xyz/openbmc_project/time/sync_method/attr/TimeSyncMethod
    ```
    - Write:
    ```
    $ curl -b cjar -k -H "Content-Type: application/json" -X  PUT -d '{"data": "xyz.openbmc_project.Time.Synchronization.Method.NTP" }' https://${bmc}/xyz/openbmc_project/time/sync_method/attr/TimeSyncMethod
    $ curl -b cjar -k -H "Content-Type: application/json" -X  PUT -d '{"data": "xyz.openbmc_project.Time.Synchronization.Method.Manual" }' https://${bmc}/xyz/openbmc_project/time/sync_method/attr/TimeSyncMethod

    $ curl -b cjar -k -H "Content-Type: application/json" -X  PUT -d '{"data": "xyz.openbmc_project.Time.Owner.Owners.BMC" }' https://${bmc}/xyz/openbmc_project/time/owner/attr/TimeOwner
    $ curl -b cjar -k -H "Content-Type: application/json" -X  PUT -d '{"data": "xyz.openbmc_project.Time.Owner.Owners.Host” }' https://${bmc}/xyz/openbmc_project/time/owner/attr/TimeOwner
    $ curl -b cjar -k -H "Content-Type: application/json" -X  PUT -d '{"data": "xyz.openbmc_project.Time.Owner.Owners.Split" }' https://${bmc}/xyz/openbmc_project/time/owner/attr/TimeOwner
    $ curl -b cjar -k -H "Content-Type: application/json" -X  PUT -d '{"data": "xyz.openbmc_project.Time.Owner.Owners.Both” }' https://${bmc}/xyz/openbmc_project/time/owner/attr/TimeOwner
    ```

* Power Supply Redundancy:

    - Read:
    ```
    $ curl -b cjar -k -H "Content-Type: application/json" -X POST https://${bmc}/xyz/openbmc_project/sensors/chassis/PowerSupplyRedundancy/action/getValue -d '{"data": []}'
    ```

    or

    ```
    $ curl -b cjar -k -X GET https://${bmc}/xyz/openbmc_project/control/power_supply_redundancy
    ```

    - Write (Enable/Disable):
    ```
    $ curl -b cjar -k -H "Content-Type: application/json" -X POST https://${bmc}/xyz/openbmc_project/sensors/chassis/PowerSupplyRedundancy/action/setValue -d '{"data": ["Enabled"]}'

    $ curl -b cjar -k -H "Content-Type: application/json" -X POST https://${bmc}/xyz/openbmc_project/sensors/chassis/PowerSupplyRedundancy/action/setValue -d '{"data": ["Disabled"]}'
    ```
    or

    ```
    $ curl -b cjar -k -H "Content-Type: application/json" -X PUT https://${bmc}/xyz/openbmc_project/control/power_supply_redundancy/attr/PowerSupplyRedundancyEnabled -d '{"data": 1}'
    $ curl -b cjar -k -H "Content-Type: application/json" -X PUT https://${bmc}/xyz/openbmc_project/control/power_supply_redundancy/attr/PowerSupplyRedundancyEnabled -d '{"data": 0}'
    ```

* Update "root" password:

    - Change password from "OpenBmc" to "abc123":
    ```
    $ curl -c cjar -b cjar -k -H "Content-Type: application/json" -X POST https://${bmc}/login -d "{\"data\": [ \"root\", \"0penBmc\" ] }"
    $ curl -b cjar -k -H "Content-Type: application/json" -d "{\"data\": [\"abc123\"] }" -X POST  https://${bmc}/xyz/openbmc_project/user/root/action/SetPassword
    ```

* Factory Reset:

    - Factory reset host and BMC software:
    ```
    $ curl -b cjar -k -H 'Content-Type: application/json' -X POST -d '{"data":[]}' https://${bmc}/xyz/openbmc_project/software/action/Reset
    ```

    - Factory reset network setting:
    ```
    $ curl -b cjar -k -H 'Content-Type: application/json' -X POST -d '{"data":[]}' https://${bmc}/xyz/openbmc_project/network/action/Reset
    ```

    - Enable field mode:
    ```
    $ curl -b cjar -k -H 'Content-Type: application/json' -X PUT -d '{"data":1}' https://${bmc}/xyz/openbmc_project/software/attr/FieldModeEnabled
    ```

    and then reboot BMC.
