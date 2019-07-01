# OpenBMC Redfish cheat sheet

This document is intended to provide a set of [Redfish][1] client
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
    A file, headers.txt, will be created. Find the "X-Auth-Token"
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
    $ curl -k -H "X-Auth-Token: $bmc_token" -X POST https://${bmc}/redfish/v1/Systems/system/Actions/ComputerSystem.Reset -d '{"ResetType": "GracefulShutdown"}'
    ```

* Host hard power off:
    ```
    $ curl -k -H "X-Auth-Token: $bmc_token" -X POST https://${bmc}/redfish/v1/Systems/system/Actions/ComputerSystem.Reset -d '{"ResetType": "ForceOff"}'
    ```

* Host power on:
    ```
    $ curl -k -H "X-Auth-Token: $bmc_token" -X POST https://${bmc}/redfish/v1/Systems/system/Actions/ComputerSystem.Reset -d '{"ResetType": "On"}'
    ```

* Reboot Host:
    ```
    $ curl -k -H "X-Auth-Token: $bmc_token" -X POST https://${bmc}/redfish/v1/Systems/system/Actions/ComputerSystem.Reset -d '{"ResetType": "GracefulRestart"}'
    ```

* Display logging entries:
    ```
    $ curl -k -H "X-Auth-Token: $bmc_token" -X GET https://${bmc}/redfish/v1/Systems/system/LogServices/EventLog/Entries
    ```

* Delete logging entries:
    ```
    $ curl -k -H "X-Auth-Token: $bmc_token" -X POST https://${bmc}/redfish/v1/Systems/system/LogServices/EventLog/Actions/LogService.Reset
    ```

* Firmware ApplyTime:
    ```
    $ curl -k -H "X-Auth-Token: $bmc_token" -X PATCH -d '{ "ApplyTime":"Immediate"}' https://${bmc}/redfish/v1/UpdateService
    ```

    ```
    $ curl -k -H "X-Auth-Token: $bmc_token" -X PATCH -d '{ "ApplyTime":"OnReset"}' https://${bmc}/redfish/v1/UpdateService
    ```

* Firmware update:
    ```
    $ curl -k -H "X-Auth-Token: $bmc_token" -H "Content-Type: application/octet-stream" -X POST -T <image file path> https://${bmc}/redfish/v1/UpdateService
    ```

* TFTP Firmware update using TransferProtocol:
    ```
    curl -k -H "X-Auth-Token: $bmc_token" -X POST https://${bmc}/redfish/v1/UpdateService/Actions/UpdateService.SimpleUpdate -d '{"TransferProtocol":"TFTP","ImageURI":"<image file path>"}'
    ```

* TFTP Firmware update with protocol in ImageURI:
    ```
    curl -k -H "X-Auth-Token: $bmc_token" -X POST https://${bmc}/redfish/v1/UpdateService/Actions/UpdateService.SimpleUpdate -d '{"ImageURI":"tftp://<image file path>"}'
    ```

[1]: https://www.dmtf.org/standards/redfish
