# Know your management client

Author: Ratan Gupta <ratagupt@linux.vnet.ibm.com>

Primary assignee: Ratan Gupta

Created: 2019-08-05

## Requirement:

-HMC has a requirement to show the vendor specific connected web clients on the
 GUI.
-At any point of time only two unique HMC can connect.
- Multiple sessions are allowed fom the managing HMC.
into the GUI through Redfish.

### Proposed Design/Flow

Assumption: System would be shipped with vendor specific user
            which would have admin privilege.

- Create a redfish session with the vendor specific user.

We have two choices here

1) Don't create the redfish manager object, Get the info from the session

Following curl command creates a session to the BMC

```curl --insecure -X POST -D headers.txt https://<ip OR hostname>/redfish/v1/SessionService/Sessions -d '{"UserName":"xyz", "Password":"abcdefg", "oem" : {"Vendor" : {"ClientID": <Management Console Identifier>}}}'```

- The Output of the above commnad will have the X-Auth-Token.

*X-Auth-Token: qAgZrZgqUli0ZZ4JzF8s*

- create the session and save it in the persistent location.
    During creation of the session put a check to make sure that
    don't allow more than two unique HMC.However multiple session
    from the same HMC would be allowed.

- client can show the connected HMC by doing the filtering on the
  username and clientIdentifier.

2) Create the redfish manager object

- After creating the session with the above curl request,
- Client sends the request to create the redfish manager object with the
  required parameters(id,name)
- Object would be created if this session belongs to the user "xyz"
  and the session clientID matches  the request param id field.
- The id of the redfish manager object would be the clientID given in the above
  POST request.

``` curl url```

- The type of the manager would be "service".In redfish "service" means
  A software-based service which provides management functions.

- GET request on the /redfish/v1/managers gets all the managers for the system.

- Client can filter the managers depends on the type of the manager.

- Further interaction using this Redfish session should add this X-Auth-Token
  in the command header.

####  How to access the REST interfaces?

- The X-Auth-Token recieved from the above session creation request can be used
  for the REST queries also.

e.g: Fetching REST Logging interface details through REST interface.

``` curl -c cjar -b cjar -k -H "Content-Type: application/json" -H "X-Auth-Token: qAgZrZgqUli0ZZ4JzF8s" -d "{\"data\": []}" -X GET https://< ip OR hostname >/xyz/openbmc_project/logging```

The response for a successful execution of this command would be as below.
{
  "data": {},
  "message": "200 OK",
  "status": "ok"
}

### Alternatives Considered:

#### Client Certificate-based authentication.

##### Session timeout

- The BMC cleans up the session data(X-Auth-Token), if the session is inactive
for 1 hour.

- Any further REST/ Redfish queries to the BMC using that authentication token
  will fail with the “401 – Unauthorized” response code.

- HMC can keep querying any BMC resource object data to keep the session alive.

##### Session/Connection drop

The HMC – BMC connection/session can drop due to various reasons viz
1) Network glitch
2) BMC unexpected reboot/crash
3) MC unexpected reboot/crash

- BMC will wait until the session timeout before declaring that the HMC is
  missing and performing the cleanup actions.

- MC will receive the “401 – Unauthorized” response codes, indicating a
  session/connection drop.
- The HMC will need to re-try the session establishment to re-manage the BMC.

##### Session termination

- The Session will be deleted when MC unmanages the BMC by logging out of the
  session.
- This can be achieved by sending a DELETE request to the session resource
  created for this MC.

curl -k -H "X-Auth-Token: <token>" -X DELETE -D headers.txt https://<ip>/redfish/v1/ SessionService/Sessions/<id>

