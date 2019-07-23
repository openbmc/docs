# Management console BMC requirement specification
Author: Ratan Gupta/Sunitha Harish

Primary assignee: Ratan Gupta

Created: July 23, 2019

# Discovery
* BMC should support multicast discovery.
* Discovery response should have the following properties.
  - IP address of BMC/Hostname
  - published services name/type/additional data

# Session Management

### Max connection/session
* The BMC has no limit on the number of client connections/sessions.
* BMC shall use the unique ID of the HMC to maintain the number of HMCs managing it.
* Due to the overhead in managing locks and save area files, HMC proposes in restricting the maximum
connections as 2. BMC should disallow the session establishment if there are already 2 HMCs being
connected to the BMC.

### Handling Connection drop scenarios
* HMC should remember the state of an ongoing operation to optimally handle the connection drop
scenarios. For eg: code update operation takes totally three https requests from client (Upload of the
image => Activation of image => BMC reboot). In this case, HMC should keep track of the completed
steps and resume from the failed one.
* Scenarios to handle connection drops: Code update, Dump (large data transfer)
  #### Connection Timeout
* HMC-BMC redfish session can be kept alive by polling the redfish service root uri
Inactive sessions on timer expiry will get automatically logged-off and the related cleanup will be carried after a timeout of TBD secs.
* Failure at HMC to receive the response to the polling request, shall be interpreted as “BMC
Timeout” and the HMC will need to reestablish the connection.
* Failure at BMC to receive the polling request, shall be interpreted as “HMC Timeout” and BMC
will perform the session cleanup.

# Know Your Client
* BMC should uniquely identify the HMC session.
* This is required in the following use cases.
  - HMC requires that BMC should notify other HMC which is connected if there is any change in
save area files (CRUD operations).
  - HMC requires that only HMC as a management client should be allowed to configure the
certificates required by VMI partition.
  - HMC requires to send notifications to other HMC connected to the BMC. For these events, the
BMC will act as a pass through/proxy.

# BMC States
* The BMC shall provide the state of the  resource when its queried by the HMC using HTTP GET
  - CEC State: State of the System
  - BMC State: State of the Manager
* The state can be either Standby or Runtime, will update it later once we find other states for the BMC, work is going on around this area.
* Change in the state will generate a State Change Event.
* Upon receiving this event, HMC can query the corresponding resource to get the new state value and perform any related operations.

# Save Area Management
* Each partition data will be saved as save area file in the BMC file system.  BMC shall provide 15MB
reserved space for save area files.
* In a fully configured system, there can be 1000 partitions which will require an equal number of save
area files.
* BMC should provide the interface for CRUD operations for save area files
* BMC should notify other HMC which is connected if there is any change
  in the save area files. This shall be done via asynchronous event handling mechanisms.

### Use Cases:
- When the virtualization firmware is down, the management console needs to create the logical partitions on a destination system using the configuration details stored in the save area files.
- Restore configuration using the save area files when the system is in an error state. The system may have up to 1,000 LPARs that need to be restored.

# Lock Management
The Hardware Management Console (HMC) is a hardware appliance that can be used to configure and control one or more managed systems. As single HMC(multiple sessions)/Multiple HMC's can possibly control same resource, we need some sort of locking scheme across all the HMC's so that we can avoid collision between multiple requests.

In FSP world locks come in two flavors, persistent and non persistent. They are differentiated by their life time with respect to the HMC-BMC connection as follows:
    - Non-Persistent Locks: Unlocked any time when an HMC transitions to temporary or permanent
disconnect, whether purposefully or accidentally (i.e. TCP/IP connection dropped)
    - Persistent Locks: Unlocked when an HMC transitions to permanent disconnect intentionally.
In BMC currently we don't have requirement to implement the persistent lock.

### Use Cases
- One HMC issuing a code update and the other HMC trying to create a partition at the
same time need to be made mutually exclusive even though both operations are distinct
- HMC acquires NON PERSISTENT locks for different resources and operations for all
virtualization use cases. NON PERSISTENT READ locks for code update, Save Area and
Capacity On Demand are acquired to make sure that none/subset of these operations are
not in progress while performing virtualization operations.
- Below are some of the virtualization use cases along with additional resource specific
lock details.
    Create Partition requires CEC level NON_PERSISTENT_WRITE lock
    Modify partition resources dynamically (DLPAR) requires resource like Memory,CPU,IO level lock
    Remote restart partition and Disaster Recovery- requires CEC level NON_PERSISTENT_WRITE lock.
    Live Partition Mobility (LPM) requires CEC level NON_PERSISTENT_WRITE lock.

As BMC is the central entity across multiple HMC's, the lock management feature should be implemented with in BMC itself.This document focuses on various design considerations that were thought off and also the dbus interface for lock management.

## Requirements on Lock
- Every lock request asked by HMC should be uniquely assigned with a ID(used by the management console to debug) , called as RequestID .
- There should be a provision to obtain either a write lock (which is acquired if a resource is to be modified) or a read lock (which is acquired if the resource is to be read).
- There should a provision to obtain multiple read locks on same resource.
- Multiple write locks on same resource should not be valid.
- Read lock on reasource which is write locked should not be valid.
- Write lock on resource which is read locked should not be valid.
- There should be a provision to obtain multi-level locks (parent and child locking mechanism), so that we should be able aquire lock for all/some resources which are under another resource.

## Requirements on Multi Level Locking.
- The locking mechanism should work for a minimum of 2 levels and a Maximum of 6 levels
- Do not lock all resources under current resource level
- Lock all resources under current resource level
- Lock all resources under current resource level with same segment size.

# Dump Management
In BMC enabled system, there can be following type of dumps.

### Platform System Dump:
* Platform Dump can be extracted only when the CEC is powered on.
* Following are the the type of platform system dumps
- CheckStop dump:
    it may happen at RunTime or during IPL
    Dump size could be in GB
    This dump gets collected while system checkstops.
    This dump may have H/w data as well as memory Data.
    Collected by SBE.
- MPIPL:
    Memory Preserving IPL dump.
    Dump size could be in GB.
    This dump gets collected while Phyp/OPAL crashes.
    This dump is preserved at Phyp memory.
- HostBoot Dump:
    This Dump gets generated during IPL time
    When hostboot crashes,SBE Collects this dump and offload to BMC.
    Dump size would be in MB.
- SBE Dump:
    When SBE crashes then BMC would collect the dump of SBE.
    Dump Size would be smaller(KB).
- This Dump may triggerd by the External Client.

### Platform Resource Dump:
- A special non-invasive type of dump designed to capture Phyp flight recorder entries, object structures, and other useful information on specific platform resources in order to improve debug of lab and field problems without disrupting the system.
- This Dump may triggerd by the External Client.
- Dump size would be in MB.
- It should be non disruptive.

### BMC Dump:
BMC dump will be generated automatically on a BMC service failure/critical error
Currently BMC has support for only the BMC dump. It contains the following data:
- BMC network details
- System Inventory data
- BMC debug data (error Log/Core)
- boot env variables
- BMC State, Chassis state
- CPU info, disk usage
- Journal data etc.

The BMC shall notify the clients about the dump availability via Asynchronous event. Upon receiving the notification, HMC can offload the dump using the dump offload interface provided by BMC.

BMC shall provide an interface to generate /& offload the BMC/Platform System/Platform Resource dumps. This would be a User Initiated dump.
BMC shall provide an interface to list the available dumps in the BMC
BMC shall provide an interface to delete the dumps
HMC needs Create/Read/Update/Delete operation for dumps with the authorization level of admin.

# TBD: BMC shall provide the interface to configure the dump policy

- Platform System Dump Collection Policy : This policy controls whether Platform System Dump collection is enabled or disabled
    Do not collect Platform System Dumps. Since this is a lab option.
    Collect Platform System Dumps as necessary (for example,after a system crash/checkstop, user initiated dump request).
    This is the default Policy.
- Platform System Dump Hardware Content Policy: Specifies how much hardware content to collect when a - Platform System Dump occurs.
    Automatic: Platform System Dump event itself defines the hardware content captured. This is the default policy.
    Maximum: Capture as much hardware data as possible.
- Platform System Dump Firmware Content Policy
    Automatic: Capture the data in memory owned by Phyp except any TCEs. This is the default policy.
    Maximum: Capture all data included in the “Automatic” policy plus all TCEs of any type.
    Physical I/O: Capture all data included in the “Automatic” policy plus all PCI TCEs.
    Virtual I/O: Capture all data included in the “Automatic” policy plus all virtual TCEs.
    HPS Cluster: Capture all data included in the “Automatic” policy plus all SMA TCEs.
    Infiniband I/O: Capture all data included in the “Automatic” policy plus all Infiniband TCEs.

# Firmware Update
All authenticated & authorized HMC sessions can perform a firmware update on the BMC
The BMC will have a single flash, which can contain more than one driver levels. HMC can select the
driver level to boot the BMC
TODO: Decide on the maximum number of driver levels
BMC shall provide an interface to upload an image
BMC shall provide an interface to list all the firmware images available in the BMC flash
BMC shall provide an interface to select one of the images for booting
BMC shall trigger an Event to notify the clients about the firmware update completion (success/failure)

# Asynchronous Events
Events occurring on the CEC shall be asynchronously notified to the subscribed clients as per the Redfish DMTF standards.
Following are the categories for the event.

* HMC-HMC notification
This shall be used by an HMC to notify the other connected HMC. The non-
HMC clients shall not be notified with this event. Typical events are as follows:
- Update in save area file
- Creation of new save area file
- Direct messaging between HMCs connected via a specific BMC (BMC will act as a passthrough)
* Generic Event
Generic events shall be notified to all the existing sessions (HMC and non HMC clients)
- Code update completed
- Dump creation
- Code update started
- Error log creation
- CEC state change
- Sensor data


# VMI interface IP configuration
* The Hardware Management Console can directly talk to the PHYP using the Virtual Management Interface.
* When the CEC initially powered on, the VMI interface would not have any network configurations. The HMC can configure and manage the VMI network.
* BMC will provide an interfaces to allow the admin to get and set the VMI network configuration.
 - Support static/dynamic IPv4/IPv6

# VMI certificate exchange
- Management Console can directly talk to the PHYP using th Virtual Management Interface.
- Management console requires its certifiate to be signed by the VMI to establish secure connection to VMI.
- VMI acts as a CA.
- Management console needs an API through which it can send the CSR to VMI(CA) and gets the signed certificate and the CA certificate from VMI.

# Acronyms
BMC: Baseboard Management Controller

HMC: Hardware Management Console

VMI: Virtual Management Interface

IOR: Input/Output Resources(IO Adapter Cards)

CRUD: Create/Read/Update/Delete

MTMS: Machine Type Machine Serial number

