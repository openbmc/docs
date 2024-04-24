# Redundant BMCs Data Synchronization

Author: Ramesh Iyyar (rameshi)

Other contributors: Matt Spinler (mspinler)

Created: April 26, 2024

## Problem Description

IBM intends to introduce a multi-chassis system featuring two chassis, each
equipped with its own BMC chip running the OpenBMC firmware stack to ensure BMC
redundancy. One BMC will operate as the Active BMC, providing accessto the Host
chip at any given moment, while the other will function as the Passive BMC,
ready to assume control if the Active BMC encounters issues.

In order to activate the redundant BMC feature, the Passive BMC requires access
to all necessary data collected by the Active BMC across various scenarios. This
proposal aims to offer a solution for transferring Active BMC application data
to the Passive BMC, and vice versa.

## Background and References

An overview of two chassis, each housing a BMC chip, within a multi-chassis
system.

```
Chassis-0                       Chassis-1
+--------------------+          +--------------------+
|   +------------+   | Network  |   +------------+   |
|   |            |   | Cable    |   |            |   |
|   |            +------------------+            |   |
|   |    BMC     |   |          |   |     BMC    |   |
|   |            |   |          |   |            |   |
|   |            |   |          |   |            |   |
|   +-+-------+--+   |          |   +--+-------+-+   |
|     | FSI   |      |          |      | FSI   |     |
|     | Cable +---------+    +---------+ Cable |     |
|     |              |  |    |  |              |     |
|   +-+----------+   |  |    |  |   +----------+-+   |
|   |            |   |  |    |  |   |            |   |
|   |            |   |  +-----------+            |   |
|   |     Host   |   |       |  |   |     Host   |   |
|   |            +-----------+  |   |            |   |
|   |            |   |          |   |            |   |
|   +------------+   |          |   +------------+   |
+--------------------+          +--------------------+
```

There are two cables:

1.  The network cable, utilized for data transfer.
2.  The FSI cable, where FSI stands for "FRU Service Interface," with FRU
    representing "Field Replaceable Unit." The BMC employs the FSI cable to
    communicate with IBM's POWER processors.

The primary aim of this design is to enable data synchronization between BMCs.
Additional information regarding redundant BMC concerns, requirements, and
design is extensively provided [here][1].

The initial proposal for this direction was introduced on the mailing list, as
documented [here][2].

## Requirements

The overarching requirements that this design must fulfill are:

- The data gathered by Active BMC applications should consistently remain
  available within the Passive BMC, updating whenever changes occur. This
  guarantees that BMC applications can utilize this data in the event of a role
  switch due to an error in the Active BMC.

- A mechanism is essential to trigger a full synchronization when necessary,
  ensuring that all application data is synced as needed.

- A mechanism is necessary to toggle the sync feature on/off. This ensures that
  it can be disabled when redundancy is not applicable (for instance, when only
  one BMC is in a functional state).

- A mechanism is needed to pause and resume synchronization on one BMC if the
  other BMC undergoes a reboot.

- A configuration file is needed to enumerate the file(s)/directory(s) for
  synchronization.

  - There needs to be a method to specify when data synchronization is needed
    based on the data, such as requiring an immediate sync or a periodic sync,
    among other options.

  - A method must be established to indicate the required direction of data
    synchronization based on the data, including options such as A2P (Active to
    Passive), P2A (Passive to Active), and Bidirectional (both A2P and P2A).

  - A method to designate retry attempts in case the synchronization operation
    fails for any reason is necessary. This enables us to retry the operation
    until attempts are exhausted, after which redundancy should be disabled.

- Mutual authentication is necessary to ensure that data is synchronized with
  the trusted BMC.

- It's essential to maintain the sync status to facilitate decisions on whether
  redundancy needs to be disabled. This is crucial because BMC application data
  is required to attempt a failover in case of any errors in the Active BMC.

## Proposed Design

### Application Design

This design introduces a new application called `phosphor-rbmc-data-sync-mgr`
which runs on each BMC to synchronize data with other BMCs and offers a set of
DBus APIs. This application begins its operation once the BMC's role is
determined. For more details on role determination, please refer [here][1].

The proposed application will employ `inotify` and `rsync` for synchronization.
Upon startup, it will initiate a full sync via `rsync` and monitor configured
files and directories using the `inotify` API. Upon receiving an inotify event,
it will trigger synchronization using `rsync`, utilizing the other BMC's IP
address hosted by `phosphor-networkd`.

Data synchronization will be disabled if BMC redundancy is disabled for any
reason.

BMC redundancy will be disabled if data synchronization fails after retry
attempts, as the redundancy cannot be supported. This ensures that failover
attempts are not made.

#### Failover

Upon triggering failover, a full sync will be initiated from the new Active BMC
to the new Passive BMC once it's ready to receive data, and data monitoring will
commence based on the sync direction, utilizing the new role.

#### Data Configuration

A singular JSON configuration file will encompass details of all data files and
directories, alongside additional synchronization configurations.

**Example Configuration File:**

```
{
	Files: [
		{
			FileName: "/file1/path/to/sync",
			Description: "Add details about the data and purpose of
the synchronization",
			SyncDirection: "A2P",
			SyncType: "Immediate",
			RetryAttempts: 1,
		},
		{
			FileName: "/file2/path/to/sync",
			Description: "Add details about the data and purpose of
the synchronization",
			SyncDirection: "Bidirectional",
			SyncType: "Periodic",
			PeriodicTime: 1h,
			RetryAttempts: 2,
		},
		...
	],
	Directories: [
		{
			DirectoryName: "/directory1/path/to/sync",
			Description: "Add details about the data and purpose of
the synchronization",
			SyncDirection: "P2A",
			SyncType: "Periodic",
			PeriodicTime: 5m,
			RetryAttempts: 1,
			ExcludeFilesList: ["/files/to/ignore/for/sync",],
			IncludeFilesList: ["/files/to/consider/for/sync",],
		},
		...
	],
}
```

A JSON schema will be defined to validate the data synchronization configuration
file.

#### DBus Representation

The intended application will host the following DBus methods and properties on
the proposed object path `/xyz/openbmc_project/rbmc/data_sync/`.

| Name        | Methods or Properties | Type    | Interface                    |
| ----------- | --------------------- | ------- | ---------------------------- |
| FullSync    | Method                | -       | xyz.openbmc_project.DataSync |
| DisableSync | Property              | Boolean | xyz.openbmc_project.DataSync |
| PauseSync   | Property              | Boolean | xyz.openbmc_project.DataSync |
| SyncStatus  | Property              | Enum    | xyz.openbmc_project.DataSync |

## Alternatives Considered

### DRBD (Distributed Replicated Block Device)

The investigation focused on DRBD to replicate Active BMC data into the Passive
BMC, but a few challenges were observed when integrating with OpenBMC.

- DRBD will replicate data from the entire block device, but there's a
  possibility of needing to exclude certain files/directories from
  synchronization or include specific files/directories for synchronization.
  This could require reorganizing the filesystem to segregate syncable and
  non-syncable data into separate volumes, leading to significant changes in all
  BMC applications.

- The BMC must utilize DRBD in "Primary-Primary" mode because the passive BMC
  needs to read synchronized data concurrently. This mode requires the use of a
  clustered filesystem (such as GFS or OCFS).

## Impacts

There are no impacts on non-redundant systems, as this feature is only enabled
in systems capable of supporting BMC redundancy.

### Organizational

This design introduces a new daemon called `phosphor-rbmc-data-sync-mgr`, which
could reside in a new repository named `phosphor-data-sync` or a relevant
existing repository (a close match being `phosphor-bmc-code-mgmt`, which hosts
an application called `phosphor-sync-software-manager`).

## Testing

Unit test cases will be authored to evaluate all functional logic, utilizing
mocks to simulate other dependent APIs.

[1]: https://gerrit.openbmc.org/c/openbmc/docs/+/70233
[2]:
  https://lore.kernel.org/openbmc/E5183DEB-8B54-45AF-BE0F-6D470937B73D@gmail.com/
