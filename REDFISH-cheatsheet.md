# Redfish cheat sheet
This document is intended to provide a set of [Redfish][1] client commands for OpenBMC usage.
(Using CURL commands)

## Query Root
```
export bmc=xx.xx.xx.xx
curl -b cjar -k https://${bmc}/redfish/v1
```
---
## Establish Redfish connection session
##### Method 1
```
export bmc=xx.xx.xx.xx
curl --insecure -X POST -D headers.txt https://${bmc}/redfish/v1/SessionService/Sessions -d    '{"UserName":"root", "Password":"0penBmc"}'
```
A file, headers.txt, will be created. Find the `"X-Auth-Token"`
in that file. Save it away in an env variable like so:

```
export bmc_token=<token>
```

##### Method 2

```
export bmc=xx.xx.xx.xx
export token=`curl -k -H "Content-Type: application/json" -X POST https://${bmc}/login -d '{"username" :  "root", "password" :  "0penBmc"}' | grep token | awk '{print $2;}' | tr -d '"'`
curl -k -H "X-Auth-Token: $token" https://${bmc}/redfish/v1/...
```
Note: Method 2 is used in this document.

---

## View Redfish Objects
```
curl -k -H "X-Auth-Token: $token" -X GET https://${bmc}/redfish/v1/Chassis
curl -k -H "X-Auth-Token: $token" -X GET https://${bmc}/redfish/v1/Managers
curl -k -H "X-Auth-Token: $token" -X GET https://${bmc}/redfish/v1/Systems
```
---

## Host power
Host soft power off:
```
curl -k -H "X-Auth-Token: $token" -X POST https://${bmc}/redfish/v1/Systems/system/Actions/ComputerSystem.Reset -d '{"ResetType": "GracefulShutdown"}'
```

Host hard power off:
```
curl -k -H "X-Auth-Token: $token" -X POST https://${bmc}/redfish/v1/Systems/system/Actions/ComputerSystem.Reset -d '{"ResetType": "ForceOff"}'
```

Host power on:
```
curl -k -H "X-Auth-Token: $token" -X POST https://${bmc}/redfish/v1/Systems/system/Actions/ComputerSystem.Reset -d '{"ResetType": "On"}'
```

Reboot Host:
```
curl -k -H "X-Auth-Token: $token" -X POST https://${bmc}/redfish/v1/Systems/system/Actions/ComputerSystem.Reset -d '{"ResetType": "GracefulRestart"}'
```

BMC reboot:
```
curl -k -X POST https://${bmc}/redfish/v1/Managers/bmc/Actions/Manager.Reset -d '{"ResetType": "GracefulRestart"}'
```

Set autoreboot:
```
curl -k -v -X PATCH https://${bmc}/redfish/v1/Systems/system -d '{"Boot":{"AutomaticRetryConfig": "RetryAttempts"}}'
```

Set Power Restore policy
```
curl -k -H "X-Auth-Token: $token" -X PATCH -d '{"PowerRestorePolicy":"AlwaysOn"}' https://${bmc}/redfish/v1/Systems/system
```
---

## Log entry

Get logging entries:
```
curl -k -X POST https://${bmc}/redfish/v1/Systems/system/LogServices/EventLog/Entries
```

Clear all logging entries:
```
curl -k -X POST https://${bmc}/redfish/v1/Systems/system/LogServices/EventLog/Actions/LogService.ClearLog
```

Display logging entries:
```
curl -k -H "X-Auth-Token: $token" -X GET https://${bmc}/redfish/v1/Systems/system/LogServices/EventLog/Entries
```

Delete logging entries:
```
curl -k -H "X-Auth-Token: $token" -X POST https://${bmc}/redfish/v1/Systems/system/LogServices/EventLog/Actions/LogService.Reset
```
---

## Sessions:
View sessions:
```
curl -k https://$bmc/redfish/v1/SessionService/Sessions/
```
---

## Network Protocols:
Disable IPMI:
```
curl -k -v -X PATCH https://${bmc}/redfish/v1/Managers/bmc/NetworkProtocol -d '{"IPMI":{"ProtocolEnabled": false}}'
```

Enabled NTP
```
curl -k -v -X PATCH https://${bmc}/redfish/v1/Managers/bmc/NetworkProtocol -d '{"NTP":{"ProtocolEnabled": true}}'
curl -k -v -X PATCH https://${bmc}/redfish/v1/Managers/bmc/NetworkProtocol -d '{"NTP":{"NTPServers":["1.1.1.1"]}}'
```
---

## Firmware ApplyTime:
```
curl -k -H "X-Auth-Token: $token" -X PATCH -d '{ "ApplyTime":"Immediate"}' https://${bmc}/redfish/v1/UpdateService
```

or

```
curl -k -H "X-Auth-Token: $token" -X PATCH -d '{ "ApplyTime":"OnReset"}' https://${bmc}/redfish/v1/UpdateService
```
---

## Firmware update
Firmware update:
Note the `<image file path>` must be a tarball.

```
curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/octet-stream" -X POST -T <image file path> https://${bmc}/redfish/v1/UpdateService
```
TFTP Firmware update using TransferProtocol:
  Note: The `<image file path>` contains the address of the TFTP service: `xx.xx.xx.xx/obmc-phosphor-xxxxx-xxxxxxxxx.static.mtd.tar`

```
curl -k -H "X-Auth-Token: $token" -X POST https://${bmc}/redfish/v1/UpdateService/Actions/UpdateService.SimpleUpdate -d '{"TransferProtocol":"TFTP","ImageURI":"<image file path>"}'
```
TFTP Firmware update with protocol in ImageURI:
```
curl -k -H "X-Auth-Token: $token" -X POST https://${bmc}/redfish/v1/UpdateService/Actions/UpdateService.SimpleUpdate -d '{"ImageURI":"tftp://<image file path>"}'
```
---

## Boot Source:
Boot source:
```
curl -k -H "X-Auth-Token: $bmc_token" -X PATCH https://${BMC_IP}/redfish/v1/Systems/system -d '{"Boot": {"BootSourceOverrideEnabled": "Continuous", "BootSourceOverrideTarget": "Hdd"}}'
```
---

## Update "root" password
Change password to "0penBmc1":
```
$ curl -k -H "X-Auth-Token: $token" -X PATCH -d '{"Password": "0penBmc1"}' https://${bmc}/redfish/v1/AccountService/Accounts/root
```
---

## Factory Reset
Factory reset:
```
curl -k -X POST https://${bmc}/redfish/v1/Managers/bmc/Actions/Manager.ResetToDefaults -d '{"ResetToDefaultsType": "ResetAll"}'
```
---

[1]: https://www.dmtf.org/standards/redfish

