# Optionality

OpenBMC does its best to be widely applicable to all BMC deployments in the
world, including many varieties of hardware. To do this requires care in the way
new features are developed, to ensure stability of systems over time. With that
as a goal, any given feature should fit into one of the following categories.

## Core

Required for the deployment of _any_ OpenBMC build. Examples of this include the
Linux kernel, systemd, sdbusplus, and phosphor object mapper. These features are
expected to remain widely applicable to all systems.

## Subsystem

These are subsystems that, while widely applicable, a user might choose to
remove entire portions of the codebase that might not be needed in that
deployment, or not used. This might be done to save binary size in the flash
image, or might be done to reduce the security attack surface. These kids of
features should be architected to ensure that, when removed, they do not cause
functional impacts to other subsystems. Examples of these include the webui,
Redfish, IPMI, and others.

## Feature Configuration Types

Features within an individual subsystem must be built such that they fall into
one of these categories. For non-trivial feature additions, the commit message
of the feature, as well as the design doc should explicitly state one of the
following classes of feature.

### Non configurable features

Theses are features that are broadly applicable to all all deployments of a
subsystem either by general usage, or by requirements in a specification, and
therefore don't need to be able to be configured. Examples of these would
include, Http keep alive, Security features like timeouts and payload size
limits, and required commands. These types of features generally show no
user-facing impact to function, although might do things like improve
performance.

Requires: Standards conformance, applicability to all flash-size systems, as
well as all processor classes.

### User opt-in features

User opt-in features are features for which an external user must explicitly
change their behavior to "opt in" to using a feature. Features like this, while
they may be configurable at compile time, are not required to be configurable at
compile time. Examples of this include Redfish query parameters, or IPMI OEM
commands.

Requires: Explicit, non-default user opt-in to execute the various features.

### Developer opt-in features

Many times a system or specific team might want a feature that is intended
_only_ for a subset of the OpenBMC audience, and might add OEM parameters to
non-standard interfaces, new command sets, or new APIs that might be applicable.
Features of this nature MUST be configurable at either compile time or runtime,
and generally will default to disabled. Features of this nature, when disabled,
must take care to make no changes to the behavior of systems for which the
feature is disabled. Company or feature specific functions, as well as the
associated metadata must not be visible on a system for which the feature is
disabled. Note, there are cases where _removing_ a feature might cause user
impact, and therefore requires an option.

Specific mechanisms to enable or disable these features are commonly:

1. Meson options
2. Entity-manager configuration
3. Yocto recipe changes
4. Phosphor ObjectMapper searches
