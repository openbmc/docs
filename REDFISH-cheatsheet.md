# OpenBMC Redfish cheat sheet

This document is intended to provide a set of Refish client
commands for OpenBMC usage.

## Using CURL commands
* Query Root
   ```
   $ export bmc=xx.xx.xx.xx
   $ curl -b cjar -k https://${bmc}/redfish/v1
   ```

* Establish Redfish connection session:
    ```
   $ export bmc=xx.xx.xx.xx
   $ curl --insecure -X POST -D headers.txt https://${bmc}/redfish/v1/SessionService/Sessions -d '{"UserName":"root", "Password":"0penBmc"}'    
    ```
    A file, headers.txt will be created. Find the "X-Auth-Token"
    in that file. Save it away in an env variable like so:
    ```
    export bmc_token=<token>
    ```

* View Redfish Objects
    ```
    $ curl -k -H "X-Auth-Token: $bmc_token" -X GET https://${bmc}/redfish/v1/Chassis
    $ curl -k -H "X-Auth-Token: $bmc_token" -X GET https://${bmc}/redfish/v1/Managers
    $ curl -k -H "X-Auth-Token: $bmc_token" -X GET https://${bmc}/redfish/v1/Systems
    ```

* Host soft power off:
    ```
    $ curl -k -H "X-Auth-Token: $bmc_token" -X POST https://${bmc}/redfish/v1/Systems/1/Actions/ComputerSystem.Reset -d '{"ResetType": "GracefulShutdown"}'
    ```

* Host hard power off:
    ```
    $ curl -k -H "X-Auth-Token: $bmc_token" -X POST https://${bmc}/redfish/v1/Systems/1/Actions/ComputerSystem.Reset -d '{"ResetType": "ForceOff"}'
    ```

* Host power on:
    ```
    $ curl -k -H "X-Auth-Token: $bmc_token" -X POST https://${bmc}/redfish/v1/Systems/1/Actions/ComputerSystem.Reset -d '{"ResetType": "On"}'
    ```

* Reboot Host:
    ```
    $ curl -k -H "X-Auth-Token: $bmc_token" -X POST https://${bmc}/redfish/v1/Systems/1/Actions/ComputerSystem.Reset -d '{"ResetType": "GracefulRestart"}'
    ```

* Display logging entries:
    ```
    $ curl -k -H "X-Auth-Token: $bmc_token" -X GET https://${bmc}/redfish/v1/Systems/1/LogServices/SEL/Entries
    ```

* Delete logging entries:
    ```
    $ curl -k -H "X-Auth-Token: $bmc_token" -X POST https://${bmc}/redfish/v1/Systems/1/LogServices/SEL/Actions/LogService.Reset
    ```
