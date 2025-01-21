# Redfish cheat sheet

This document is intended to provide a set of example [Redfish][1] client
commands for OpenBMC usage. This document uses cURL. This document assumes
several ids, such as ManagerId, "bmc", and ComputerSystemId, "system". Assuming
an id is not correct and any software written to use the Redfish API should not.
From the Redfish Specification, DSP0266, "Clients shall not make assumptions
about the URIs for the members of a resource collection."

## Query Redfish Service Root

```
export bmc=xx.xx.xx.xx
curl -k https://${bmc}/redfish/v1
```

---

## Establish Redfish connection session

##### Method 1

```
export bmc=xx.xx.xx.xx
curl --insecure -H "Content-Type: application/json" -X POST -D headers.txt https://${bmc}/redfish/v1/SessionService/Sessions -d    '{"UserName":"root", "Password":"0penBmc"}'
```

A file, headers.txt, will be created. Find the `"X-Auth-Token"` in that file.
Save it away in an env variable like so:

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

## View sessions

```
curl -k -H "X-Auth-Token: $token" https://${bmc}/redfish/v1/SessionService/Sessions
```

---

## Host power

Host soft power off:

```
curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X POST https://${bmc}/redfish/v1/Systems/system/Actions/ComputerSystem.Reset -d '{"ResetType": "GracefulShutdown"}'
```

Host hard power off:

```
curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X POST https://${bmc}/redfish/v1/Systems/system/Actions/ComputerSystem.Reset -d '{"ResetType": "ForceOff"}'
```

Host power on:

```
curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X POST https://${bmc}/redfish/v1/Systems/system/Actions/ComputerSystem.Reset -d '{"ResetType": "On"}'
```

Reboot Host:

```
curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X POST https://${bmc}/redfish/v1/Systems/system/Actions/ComputerSystem.Reset -d '{"ResetType": "GracefulRestart"}'
```

---

## BMC reboot

```
curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X POST https://${bmc}/redfish/v1/Managers/bmc/Actions/Manager.Reset -d '{"ResetType": "GracefulRestart"}'
```

---

## BMC factory reset

Proceed with caution:

```
curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X POST https://${bmc}/redfish/v1/Managers/bmc/Actions/Manager.ResetToDefaults -d '{"ResetType": "ResetAll"}'
```

---

## Log entry

Display logging entries:

```
curl -k -H "X-Auth-Token: $token" -X GET https://${bmc}/redfish/v1/Systems/system/LogServices/EventLog/Entries
```

Delete logging entries:

```
curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X POST https://${bmc}/redfish/v1/Systems/system/LogServices/EventLog/Actions/LogService.Reset
```

---

## Firmware update

Firmware update: Note the `<image file path>` must be a tarball.

```
uri=$(curl -k -H "X-Auth-Token: $token" https://${bmc}/redfish/v1/UpdateService | jq -r ' .HttpPushUri')

curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/octet-stream" -X POST -T <image file path> https://${bmc}${uri}
```

Firmware update using multi-part form data: Note the `<apply time>` can be
`OnReset` or `Immediate`.

```
uri=$(curl -k -H "X-Auth-Token: $token" https://${bmc}/redfish/v1/UpdateService | jq -r ' .MultipartHttpPushUri')

curl -k -H "X-Auth-Token: $token" -H "Content-Type:multipart/form-data" -X POST -F UpdateParameters="{\"Targets\":[\"/redfish/v1/Managers/bmc\"],\"@Redfish.OperationApplyTime\":<apply time>};type=application/json" -F "UpdateFile=@<image file path>;type=application/octet-stream" https://${bmc}${uri}
```

TFTP Firmware update using TransferProtocol: Note: The `<image file path>`
contains the address of the TFTP service:
`xx.xx.xx.xx/obmc-phosphor-xxxxx-xxxxxxxxx.static.mtd.tar`

```
curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X POST https://${bmc}/redfish/v1/UpdateService/Actions/UpdateService.SimpleUpdate -d '{"TransferProtocol":"TFTP","ImageURI":"<image file path>"}'
```

TFTP Firmware update with protocol in ImageURI:

```
curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X POST https://${bmc}/redfish/v1/UpdateService/Actions/UpdateService.SimpleUpdate -d '{"ImageURI":"tftp://<image file path>"}'
```

---

## Update "root" password

Change password to "0penBmc1":

```
curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PATCH -d '{"Password": "0penBmc1"}' https://${bmc}/redfish/v1/AccountService/Accounts/root
```

---

## BIOS firmware boot control

Enter into BIOS setup on boot

```
curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PATCH https://${bmc}/redfish/v1/Systems/system -d '{"Boot":{"BootSourceOverrideEnabled": "Continuous","BootSourceOverrideTarget": "BiosSetup"}}'
```

Fully boot

```
curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PATCH https://${bmc}/redfish/v1/Systems/system -d '{"Boot":{"BootSourceOverrideEnabled": "Disabled","BootSourceOverrideTarget": "None"}}'
```

Change Legacy/EFI selector (valid only if host is based on the x86 CPU)

```
curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PATCH https://${bmc}/redfish/v1/Systems/system -d '{"Boot":{"BootSourceOverrideEnabled": "Once","BootSourceOverrideTarget": "None","BootSourceOverrideMode": "UEFI"}}'
```

---

## Enable NTP

Add a NTP Server

```
curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PATCH https://${bmc}/redfish/v1/Managers/bmc/NetworkProtocol -d '{"NTP":{"NTPServers":["time.nist.gov"]}}'
```

Now enable NTP

```
curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PATCH https://${bmc}/redfish/v1/Managers/bmc/NetworkProtocol -d '{"NTP":{"ProtocolEnabled": true}}'
```

---

## Disable IPMI

```
curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X PATCH https://${bmc}/redfish/v1/Managers/bmc/NetworkProtocol -d '{"IPMI":{"ProtocolEnabled": false}}'
```

## Manage subscriptions

List configured subscriptions:

```
curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" https://${bmc}/redfish/v1/EventService/Subscriptions/
```

Setup subscription

```
curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" -X POST https://${bmc}/redfish/v1/EventService/Subscriptions -d '{"Context": "My context", "DeliveryRetryPolicy": "RetryForever", "Destination": "http://yy.yy.yy.yy:8888/events", "EventFormatType": "Event", "Protocol": "Redfish", "SubscriptionType": "RedfishEvent"}"
```

Store the ID of the first subscription in an environment variable:

```
export subscription_id=$(curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" https://${bmc}/redfish/v1/EventService/Subscriptions/ |jq '.Members[0]."@odata.id"' | cut -d '/' -f 6| cut -d '"' -f 1)
```

Display information for the subscription associated with the stored ID:

```
curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" https://${bmc}/redfish/v1/EventService/Subscriptions/${subscription_id}
```

Update the subscription corresponding to the stored ID:

```
curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" https://${bmc}/redfish/v1/EventService/Subscriptions/${subscription_id} -X PATCH -d '{"VerifyCertificate": false}'
```

Remove the subscription identified by the stored ID:

```
curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" https://${bmc}/redfish/v1/EventService/Subscriptions/${subscription_id} -X DELETE
```

### Test subscriptions

Send a test event using embedded into Redfish Event Service facility:

```
curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/json" https://${bmc}/redfish/v1/EventService/Actions/EventService.SubmitTestEvent -X POST -d '{"EventTimestamp":"2023-02-13T14:49:20Z","Severity":"Warning","Message":"This is a test event message 2","MessageId":"iLOResourceEvents.1.3.DrvArrLogDrvErasing","MessageArgs":["1","slot 3"],"OriginOfCondition":"/redfish/v1/Systems/1/Storage"}'
```

Send a test DBus event, it should be picked up by the bmcweb and send event to
the subscriber (the command should be executed on the BMC):

```
busctl call xyz.openbmc_project.Logging \
/xyz/openbmc_project/logging \
xyz.openbmc_project.Logging.Create \
Create 'ssa{ss}' \
OpenBMC.0.1.PowerButtonPressed \
xyz.openbmc_project.Logging.Entry.Level.Error 0
```

[1]: https://www.dmtf.org/standards/redfish
