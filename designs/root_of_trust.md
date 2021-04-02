# Google Specific APIs - A New Service Root for Google

Author:
 Feras Aldahlawi (faldahlawi)

Primary assignee:
 Feras Aldahlawi (faldahlawi)

Other contributors:
  None

Created:
 March 30, 2021

## Problem Description
Redfish API does not have a resource type that is similar to Google's RoT chips. Google needs APIs that are not in the Redfish standard yet. There are working groups dedicated to bring RoT chips support to the Redfish standard already. Hence adding this support under a Google namespace would avoid conflict with those working groups. This document provides the schema of what Google needs for its new service root.

## Background and References
At Google, we rely on communicating with RoT chips using a variety of transport mechanisms. Google wants to extend the support to include REST based APIs.

## Requirements
- Create a new service root of Google specific APIs.
- Create a schema for a RootOfTrust resource.

## Proposed Design
A new service root under `google/v1`. This service root will contain a collection of `RootOfTrust` entities that have the following properties and Actions:
- Chip type string
- Unique Hardware id string
- Firmware version map string to string
- Mailbox API
```
{
  "#RootOfTrust.Mailbox": {
      "target": "/redfish/v1/RootsOfTrust/0/Actions.Mailbox",
      "@Redfish.ActionInfo": "/redfish/v1/RootsOfTrust/0/Actions.Mailbox"
    }
  "Command": 7
  "Version": 6
  "RawRequest": "03947"
}
```

This new service root is very similar to `/ibm/v1`.

## Alternatives Considered
Considered adding the new APIs as an OEM extension to the TPM resource. However, it was an unnatural extension.

## Impacts
N/A.

## Testing
Testing will be done in Google internally. This feature should not impact CI testing.
