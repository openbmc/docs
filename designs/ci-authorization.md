# Continuous integration and authorization for OpenBMC

Author: Brad Bishop !radsquirrel

Other contributors: None

Created: 2019-01-30

## Problem Description

The OpenBMC project maintains a number of Jenkins CI jobs to ensure incoming
contributions to the project source code meet a level of quality. Incoming
contributions can be made by the general public - anyone with a GitHub account.
However unlikely, it is possible for a bad actor to make code submissions that
attempt to compromise project resources, e.g. build systems, and as such some
amount of authorization of contributors must occur to provide some level of
protection from potential bad actors.

The project already has contributor authorization for CI. This proposal serves
to describe the drawbacks of the current solution and propose an alternative
that addresses those drawbacks.

## Background and References

The current authorization solution checks the user for membership in the
openbmc/general-developers GitHub team. If the contributor is a member of the
team (or a general-developers sub-team), the automated CI processes are
triggered without any human intervention. If the contributor is not a member of
the general-developers team, manual intervention (ok-to-test) is required by a
project maintainer to trigger the automated CI processes.

Additional reading: https://en.wikipedia.org/wiki/Continuous_integration
https://jenkins.io/ https://help.github.com/articles/about-organizations/

## Requirements

The existing method for authorization has a singular problem - the GitHub
organization owner role. In order for contributors to be added to the
openbmc/general-developers GitHub team, the contributor must first be a member
of the openbmc GitHub organization. Only organization owners can invite GitHub
users to become members of an organization. Organization owners have
unrestricted access to all aspects of the project - it would be unwise to bestow
organization ownership for the sole purpose of enabling
openbmc/general-developers group membership administrative capability.

An alternative authorization method for CI should:

- Not require the GitHub organization owner role to administer the list of users
  authorized for CI.
- Enable a hierarchical trust model for user authorization (groups nested within
  groups).

## Proposed Design

The proposal is to simply migrate the current openbmc/general-developers GitHub
team, and all subordinate teams, to Gerrit groups:

group: `openbmc/ci-authorized`

group: `xyzcorp/ci-authorized`

group: `abccorp/ci-authorized`

The openbmc/ci-authorized group can contain users that are not associated with
any specific organization, as well as organizational groups:

group: `openbmc/ci-authorized` contains ->

group `xyzcorp/ci-authorized`

group `abccorp/ci-authorized`

user `nancy`

user `joe`

This proposal also specifies a convention for administration of organizational
groups:

group: `xyzcorp/ci-authorized-owners` administers -> `xyzcorp/ci-authorized`

group: `abccorp/ci-authorized-owners` administers -> `abccorp/ci-authorized`

group: `openbmc/ci-authorized` administers -> `openbmc/ci-authorized`

Finally, any Jenkins CI jobs must be updated to test for membership of the
Gerrit group instead of the GitHub team.

New organizational groups (and associated owner groups) will be created when a
CCLA is signed and accepted by the project.

## Alternatives Considered

Assigning GitHub organization owner roles to organizational group administrators
was considered but is a major violation of the least-privilege-required
principle.

## Impacts

GitHub has vastly superior load balancing and backup capability so there is a
potential for decreased service availability and data loss.

## Testing

Deploy on a live production server 😀
