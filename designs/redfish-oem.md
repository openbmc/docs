# Redfish OEM

Author:
  Jason Bills: jmbills

Other contributors:
  None

Created:
  August 5, 2022

## Problem Description
Supporting OEM and company-specific Redfish resources today requires
modifications directly to bmcweb. Maintaining all of these modifications
falls to the maintainers of bmcweb and will become more cumbersome as the
number of customizations grows.

## Background and References
Redfish, like IPMI, is designed to be extensible with OEM resources. For
IPMI, OpenBMC provides the capability to extend IPMI support through OEM
libraries. For example, https://github.com/openbmc/intel-ipmi-oem.

The goal of this design is to provide similar capability for Redfish.

## Requirements
The primary requirement is an interface for any OpenBMC application to
provide a Redfish resource that bmcweb can make available in the standard
Redfish tree.

This interface will allow an application to support a Redfish resource that a
user can act upon through bmcweb just like any standard resource: using GET,
POST, and PATCH.

This interface will support adding resources and collections of resources
anywhere in the Redfish tree, including previously unknown namespaces. The
added resources can contain both properties and actions.

The application that provides the Redfish resource must also provide
- the corresponding Redfish schemas
- the PrivilegeRegistry entries for the added resources

All added resources and schemas must meet the requirements in the Redfish
specification and pass the Redfish Service Validator. The owner of each
Redfish library will be responsible for keeping it up-to-date with any
Redfish changes to continue passing the Validator.

These features are not required but may be desireable for future enhancments:
- Ability to add MessageRegistries
- Ability to change Eventing

Existing OpenBMC OEM resources can be ported out of bmcweb into a generic
OpenBMC Redfish library.

Existing company-specific resources will be ported out of bmcweb into
company-specific Redfish libraries.

## Proposed Design
TBD

## Alternatives Considered
Single repo - If OpenBMC adopts the proposed single-repo approach, that would
alleviate the need to fork bmcweb individually.

## Impacts
TBD

### Organizational
- Does this repository require a new repository?  (Yes, No)
- Who will be the initial maintainer(s) of this repository?
- Which repositories are expected to be modified to execute this design?
- Make a list, and add listed repository maintainers to the gerrit review.

## Testing
TBD
