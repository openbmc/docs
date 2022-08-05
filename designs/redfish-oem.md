# Redfish OEM

Author:
  Jason Bills: jmbills

Other contributors:
  None

Created:
  August 5, 2022

## Problem Description
Supporting OEM and company-specific Redfish resources today requires carrying
patches for bmcweb.  Patches are cumbersome to manage and are difficult to
share with others in the community.

## Background and References
Redfish is designed to be extensible with OEM resources.  However, in OpenBMC
and bmcweb, it is impractical to support all possible OEM resources from the
community. So, they are generally required to be supported in downstream
patches.

## Requirements
The primary requirement is an interface for any OpenBMC application to
provide a Redfish resource that bmcweb can make available in the standard
Redfish tree.

Ideally, this interface will allow an application to support an OEM Redfish
resource that a user can act upon through bmcweb just like any standard
resource: using GET, POST, and PATCH, as well as the ability to trigger
Redfish Actions.

## Proposed Design
TBD

## Alternatives Considered
None

## Impacts
TBD

### Organizational
- Does this repository require a new repository?  (Yes, No)
- Who will be the initial maintainer(s) of this repository?
- Which repositories are expected to be modified to execute this design?
- Make a list, and add listed repository maintainers to the gerrit review.

## Testing
TBD
