# OpenBMC REST cheat sheet

This document is intended to provide a set of REST client commands for OpenBMC usage.

## Using CURL commands
* Establish REST connection session:
    ```
   $ export bmc=xx.xx.xx.xx
   $ curl -c cjar -b cjar -k -H "Content-Type: application/json" -X POST https://${bmc}/login -d "{\"data\": [ \"root\", \"0penBmc\" ] }"
    ```

* List and enumerate:
    ```
    $ curl -c cjar -b cjar -k -H "Content-Type: application/json" https://${bmc}/xyz/openbmc_project/list
    $ curl -c cjar -b cjar -k -H "Content-Type: application/json" https://${bmc}/xyz/openbmc_project/enumerate
    ```

* List sub-objects:
    ```
    $ curl -c cjar -b cjar -k -H "Content-Type: application/json" https://${bmc}/xyz/openbmc_project/
    $ curl -c cjar -b cjar -k -H "Content-Type: application/json" https://${bmc}/xyz/openbmc_project/state/
    ```

* Host soft power off:
    ```
    $ curl -c cjar -b cjar -k -H "Content-Type: application/json" -d '{"data": "xyz.openbmc_project.State.Host.Transition.Off"}' -X PUT https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
    ```

* Host hard power off:
    ```
    $ curl -c cjar -b cjar -k -H "Content-Type: application/json" -X PUT -d '{"data":"xyz.openbmc_project.State.Chassis.Transition.Off"}' https://${bmc}//xyz/openbmc_project/state/chassis0/attr/RequestedPowerTransition
    ```
* Host power on:
    ```
    $ curl -c cjar -b cjar -k -H "Content-Type: application/json" -d '{"data": "xyz.openbmc_project.State.Host.Transition.On"}' -X PUT https://${bmc}/xyz/openbmc_project/state/host0/attr/RequestedHostTransition
    ```

* Reboot BMC:
    ```
    $ curl -c cjar -b cjar -k -H "Content-Type: application/json" -X PUT -d '{"data":"xyz.openbmc_project.State.BMC.Transition.Reboot"}' https://${bmc}//xyz/openbmc_project/state/bmc0/attr/RequestedBMCTransition
    ```
* Delete logging entries:
    ```
    $ curl -c cjar -b cjar -k -H "Content-Type: application/json" -X DELETE https://${bmc}/xyz/openbmc_project/logging/entry/<entry_id>
    $ curl -b cjar -k -H 'Content-Type: application/json' -X POST -d '{"data":[]}' https://${bmc}/xyz/openbmc_project/logging/action/DeleteAll
    ```
* Set boot mode:
    ```
    $ curl -c cjar -b cjar -k -H "Content-Type: application/json" -X PUT https://${bmc}/xyz/openbmc_project/control/host0/boot/one_time/attr/BootMode -d '{"data": "xy.openbmc_project.Control.Boot.Mode.Modes.Regular"}'
    ```
* Set boot source:
    ```
    $ curl -c cjar -b cjar -k -H "Content-Type: application/json" -X PUT https://${bmc}/xyz/openbmc_project/control/host0/boot/one_time/attr/BootSource -d '{"data": "xyz.openbmc_project.Control.Boot.Source.Sources.Default"}
    ```
