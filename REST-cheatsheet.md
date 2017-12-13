# OpenBMC REST cheat sheet

This document is intended to provide a set of REST client commands for OpenBMC usage.

## Using CURL commands
* Establish REST connection session:
    ```
   $ export BMC_IP=xx.xx.xx.xx
   $ curl -c cjar -b cjar -k -H "Content-Type: application/json" -X POST https://$BMC_IP/login -d "{\"data\": [ \"root\", \"0penBmc\" ] }"
    ```

* List and enumerate:
    ```
    $ curl -c cjar -b cjar -k -H "Content-Type: application/json" https://$BMC_IP/xyz/openbmc_project/list
    $ curl -c cjar -b cjar -k -H "Content-Type: application/json" https://$BMC_IP/xyz/openbmc_project/enumerate
    ```

* Path walk through:
    ```
    $ curl -c cjar -b cjar -k -H "Content-Type: application/json" https://$BMC_IP/xyz/openbmc_project/
    $ curl -c cjar -b cjar -k -H "Content-Type: application/json" https://$BMC_IP/xyz/openbmc_project/state/
    ```

* Host soft power off:
    ```
    $ curl -c cjar -b cjar -k -H "Content-Type: application/json" -d '{"data": "xyz.openbmc_project.State.Host.Transition.Off"}' -X PUT https://$BMC_IP/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
    ```

* Host hard power off:
    ```
    $ curl -c cjar -b cjar -k -H "Content-Type: application/json" -X PUT -d '{"data":"xyz.openbmc_project.State.Chassis.Transition.Off"}' https://$BMC_IP//xyz/openbmc_project/state/chassis0/attr/RequestedPowerTransition
    ```
* Host power on:
    ```
    $ curl -c cjar -b cjar -k -H "Content-Type: application/json" -d '{"data": "xyz.openbmc_project.State.Host.Transition.On"}' -X PUT https://$BMC_IP/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
    ```

* Reboot BMC:
    ```
    $ curl -c cjar -b cjar -k -H "Content-Type: application/json" -X PUT -d '{"data":"xyz.openbmc_project.State.BMC.Transition.Reboot"}' https://$BMC_IP//xyz/openbmc_project/state/bmc0/attr/RequestedBMCTransition
    ```
* Delete logging(s):
    ```
    $ curl -c cjar -b cjar -k -H "Content-Type: application/json" -X DELETE https://$BMC_IP/xyz/openbmc_project/logging/entry/<entry_id>
    $ curl -b cjar -k -H 'Content-Type: application/json' -X POST -d '{"data":[]}' https://$BMC_IP/xyz/openbmc_project/logging/action/DeleteAll
    ```
* Boot mode setting:
    ```
    $ curl -c cjar -b cjar -k -H "Content-Type: application/json" -X PUT https://$BMC_IP/xyz/openbmc_project/control/host0/boot/one_time/attr/BootMode -d '{"data": "xy.openbmc_project.Control.Boot.Mode.Modes.Regular"}'
    ```
* Boot source setting:
    ```
    $ curl -c cjar -b cjar -k -H "Content-Type: application/json" -X PUT https://$BMC_IP/xyz/openbmc_project/control/host0/boot/one_time/attr/BootSource -d '{"data": "xyz.openbmc_project.Control.Boot.Source.Sources.Default"}
    ```
