# Redfish cheat sheet
This chapter is intended to provide a set of Redfish client commands for OpenBMC usage.
(Using CURL commands)

## 1 Query Root
```
$ export bmc=xx.xx.xx.xx
$ curl -b cjar -k https://${bmc}/redfish/v1
```
## 2 Establish Redfish connection session
+ Method 1
   ```
$ export bmc=xx.xx.xx.xx
$ curl --insecure -X POST -D headers.txt https://${bmc}/redfish/v1/SessionService/Sessions -d '{"UserName":"root", "Password":"0penBmc"}'
   ```
   A file, headers.txt, will be created. Find the `"X-Auth-Token"`
   in that file. Save it away in an env variable like so:
   
   ```
   export bmc_token=<token>
   ```
- Method 2
   ```
$ export bmc=xx.xx.xx.xx
$ export token=`curl -k -H "Content-Type: application/json" -X POST https://${bmc}/login -d '{"username" :  "root", "password" :  "0penBmc"}' | grep token | awk '{print $2;}' | tr -d '"'`
    % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                                                    Dload  Upload   Total   Spent    Left  Speed
  100     85     100    37  100    48  109  141  --:--:-- --:--:-- --:--:--     250
  $ curl -k -H "X-Auth-Token: $token" https://${bmc}/redfish/v1/...
  ```
   Note: Method 2 is used in this document.
## 3 View Redfish Objects
```
$ curl -k -H "X-Auth-Token: $token" -X GET https://${bmc}/redfish/v1/Chassis
$ curl -k -H "X-Auth-Token: $token" -X GET https://${bmc}/redfish/v1/Managers
$ curl -k -H "X-Auth-Token: $token" -X GET https://${bmc}/redfish/v1/Systems
```
## 4 Host power
- Host soft power off:
   ```
$ curl -k -H "X-Auth-Token: $token" -X POST https://${bmc}/redfish/v1/Systems/system/Actions/ComputerSystem.Reset -d '{"ResetType": "GracefulShutdown"}'
   ```
- Host hard power off:
   ```
$ curl -k -H "X-Auth-Token: $token" -X POST https://${bmc}/redfish/v1/Systems/system/Actions/ComputerSystem.Reset -d '{"ResetType": "ForceOff"}'
   ```
- Host power on:
   ```
$ curl -k -H "X-Auth-Token: $token" -X POST https://${bmc}/redfish/v1/Systems/system/Actions/ComputerSystem.Reset -d '{"ResetType": "On"}'
   ```
- Reboot Host:
   ```
$ curl -k -H "X-Auth-Token: $token" -X POST https://${bmc}/redfish/v1/Systems/system/Actions/ComputerSystem.Reset -d '{"ResetType": "GracefulRestart"}'
   ```
## 5 Log entry
- Display logging entries:
   ```
  $ curl -k -H "X-Auth-Token: $token" -X GET https://${bmc}/redfish/v1/Systems/system/LogServices/EventLog/Entries
  {
    "@odata.context": "/redfish/v1/$metadata#LogEntryCollection.LogEntryCollection",
    "@odata.id": "/redfish/v1/Systems/system/LogServices/EventLog/Entries",
    "@odata.type": "#LogEntryCollection.LogEntryCollection",
    "Description": "Collection of System Event Log Entries",
    "Members": [
      {
        "@odata.context": "/redfish/v1/$metadata#LogEntry.LogEntry",
        "@odata.id": "/redfish/v1/Systems/system/LogServices/EventLog/Entries/1",
        "@odata.type": "#LogEntry.v1_4_0.LogEntry",
        "Created": "2019-07-15T11:48:51+00:00",
        "EntryType": "Event",
        "Id": "1",
        "Message": "xyz.openbmc_project.Common.Error.InternalFailure",
        "Name": "System Event Log Entry",
        "Severity": "Critical"
      }
    ],
    "Members@odata.count": 1,
    "Name": "System Event Log Entries"
  }
  ```
- Delete logging entries:
  ```
$ curl -k -H "X-Auth-Token: $token" -X POST https://${bmc}/redfish/v1/Systems/system/LogServices/EventLog/Actions/LogService.Reset
  ```
## 6 Firmware ApplyTime:
   ```
$ curl -k -H "X-Auth-Token: $token" -X PATCH -d '{ "ApplyTime":"Immediate"}' https://${bmc}/redfish/v1/UpdateService
{
  "@Message.ExtendedInfo": [
    {
      "@odata.type": "/redfish/v1/$metadata#Message.v1_0_0.Message",
      "Message": "Successfully Completed Request",
      "MessageArgs": [],
      "MessageId": "Base.1.4.0.Success",
      "Resolution": "None",
      "Severity": "OK"
    }
  ]
}
   ```

```
$ curl -k -H "X-Auth-Token: $token" -X PATCH -d '{ "ApplyTime":"OnReset"}' https://${bmc}/redfish/v1/UpdateService
{
  "@Message.ExtendedInfo": [
    {
      "@odata.type": "/redfish/v1/$metadata#Message.v1_0_0.Message",
      "Message": "Successfully Completed Request",
      "MessageArgs": [],
      "MessageId": "Base.1.4.0.Success",
      "Resolution": "None",
      "Severity": "OK"
    }
  ]
}
```
## 7 Firmware update
- Firmware update:
  Note the `<image file path>` shall be a tarball.
   ```
$ curl -k -H "X-Auth-Token: $token" -H "Content-Type: application/octet-stream" -X POST -T <image file path> https://${bmc}/redfish/v1/UpdateService
  {
    "@Message.ExtendedInfo": [
      {
        "@odata.type": "/redfish/v1/$metadata#Message.v1_0_0.Message",
        "Message": "Successfully Completed Request",
        "MessageArgs": [],
        "MessageId": "Base.1.4.0.Success",
        "Resolution": "None",
        "Severity": "OK"
      }
    ]
  }
   ```
- TFTP Firmware update using TransferProtocol: 
  Note: The `<image file path>` contains the address of the TFTP service: `xx.xx.xx.xx/obmc-phosphor-xxxxx-xxxxxxxxx.static.mtd.tar`
  
   ```
  $ curl -k -H "X-Auth-Token: $token" -X POST https://${bmc}/redfish/v1/UpdateService/Actions/UpdateService.SimpleUpdate -d '{"TransferProtocol":"TFTP","ImageURI":"<image file path>"}'
    {
    "@Message.ExtendedInfo": [
      {
        "@odata.type": "/redfish/v1/$metadata#Message.v1_0_0.Message",
        "Message": "Successfully Completed Request",
        "MessageArgs": [],
        "MessageId": "Base.1.4.0.Success",
        "Resolution": "None",
        "Severity": "OK"
  }
  ]
  }
   ```
- TFTP Firmware update with protocol in ImageURI: 
  ```
$ curl -k -H "X-Auth-Token: $token" -X POST https://${bmc}/redfish/v1/UpdateService/Actions/UpdateService.SimpleUpdate -d '{"ImageURI":"tftp://<image file path>"}'
{
  "@Message.ExtendedInfo": [
    {
      "@odata.type": "/redfish/v1/$metadata#Message.v1_0_0.Message",
      "Message": "Successfully Completed Request",
      "MessageArgs": [],
      "MessageId": "Base.1.4.0.Success",
      "Resolution": "None",
      "Severity": "OK"
    }
  ]
}
  ```
## 8 Update "root" password
Change password to "0penBmc1":
  ```
$ curl -k -H "X-Auth-Token: $token" -X PATCH -d '{"Password": "0penBmc1"}' https://${bmc}/redfish/v1/AccountService/Accounts/root
  ```