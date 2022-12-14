# Firmware Handler to Support Update & Get Version

Author: Delphine CC Chiu <delphine_cc_chiu@wiwynn.com>

Other contributors: None

Created: 12/12/2022

## Problem Description

Almost every openbmc projects need firmware-related service to perform
firmware update, get firmware version etc. But currently most of the services
are implemented at project layer, which is hard to reuse the code if two
different projects use a same device, although they actually follow the same
workflow published by vendors. Furthermore, the foundation of firmware
update is a series of I/O operation, the code cannot be shared means there will
be tons of duplicated I/O operation code.

## Glossary

- **Workflow:** A series of commands defined by vendors to perform firmware
update, dump image, etc.
- **Command:** The commands be used in the workflow. For example, the image
below is the commands provided by Lattice.
([Source](https://www.latticesemi.com/view_document?document_id=50123))
![](https://user-images.githubusercontent.com/112851067/207290928-c87646f9-925d-441c-951e-a8d5be4c88b8.png)

## Background and References

There are plenty of devices need to update its firmware, in YV3.5, we need to
implement firmware update service for the CPLD, BIC, BMC, and VRs, we found
that the vendors who manufacture these devices will provide the workflow for
updating its firmware, for example, LCMXO3-6900H (manufactured by Lattice) CPLD
are used on Greatlakes, if we want to update the firmware, we need to implement
the workflow provided by its specification.
([MachXO3 Programming and Configuration User Guide](https://www.latticesemi.com/view_document?document_id=50123))

![](https://user-images.githubusercontent.com/112851067/206970732-79e92c25-5a89-4cfa-8e48-053becf5bcfa.png)

(The partial workflow for updating the firmware of LCMXO3 family CPLD.)

## Requirements

Since we want to maximum the reuseability of the firmware update workflow and
I/O operations, we need to exstract the whole implementation to the common
layer, i.e., create a new repository, it is reasonable because we only
implement the workflow and the commands it used, which is not project-specific
implementation.

### Firmware Handler needs to provide:
- A new repository for firmware-related service, such as update, get version,
etc.
- A design definition of a firmware-related service.
    - A generic architecture to extend different vendors' devices.
    - Maximum the reusability of workflows and I/O operations.
    - The projects can easily include the handler they need, and write a
service to pass project-specific parameters for customization.
- Functionalities for updating the firmware and get version for CPLD (Intel
MAX10 CPLD), BIC (Aspeed AST1030), BIOS(MXIC MX25L51245GMI-08G, WINBOND
W25Q512JVFIQ).

## Proposed Design

The image below is the overall architecture of Firmware Handler, shown as a
UML class diagram, it is designed to represent three core concepts:
- Workflows provided by vendors.
- The commands used in different workflows.
- Reusable I/O Operation.

### Overall Architecture

![](https://user-images.githubusercontent.com/112851067/206974008-1dd2963f-4aa9-4e86-a698-5f7ca26362c6.png)

#### ComponentManager
We use ***ComponentManager*** to define what functions should be implemented in a
firmware-related service, and leave the implementation, i.e., the workflow, to
its children classes.

#### FwOperation
There are many [commands](#Glossary) will be used while performing the workflow
, and it usually invoked multiple times, so we use ***FwOperation*** to reuse
the commands in the workflow, and use CompositeFwOperation to divide & compose
FwOperations for flexibility.


#### HardwareAccessor
We use ***HardwareAccessor*** to reuse the read/write behavior between
different transport interface, such as I2C or JTAG, and use it in FwOperation,
isolate commands and interfaces, i.e., a FwOperation is compatible with
multiple transport interfaces.

#### ComponentManagerFactory
ComponentManagers are categorized by "{VENDOR}{COMPONENT_TYPE}", e.g.,
LatticeCpldManager, AlteraCpldManager, AspeedBICManager, etc. We use
***ComponentManagerFactory*** to provide corresponding Managers accroding to
user's input.

### Implementation
The image below is our POC of the architecture, we implemented the firmware
update and get version for Lattice LCMXO3 CPLD, we successfully implement the
workflow and reuse the ***I2CAccessor***, extend some customized behavior
while conducting read operation.
![Implementation](https://user-images.githubusercontent.com/112851067/206981036-f69adb4c-5e92-4449-8094-29bf645a945e.png)

The implementation can refer to [Link](https://gerrit.openbmc.org/c/openbmc/openbmc/+/58885)
, since we don't have a standalone repository yet, you can consider that the
meta-facebook/recipes-phosphor is the common layer, not project layer.

### Hierarchical Directory Structure of This Repository
The directory structure of the new repository are shown below, it can easily
be extended to support more vendor's devices, and extract the common code for
reuseability.

![](https://user-images.githubusercontent.com/112851067/206982208-1e217aa9-fc15-4759-911b-a1eeac0dff22.png)

### Include in Project Layer
Projects can select what type of the Firmware Handler they need, install the
executable binary file, and write a .service file to bring the project-specific
pararmeter into the binary. The example code are shown below.

![](https://user-images.githubusercontent.com/112851067/207287870-296b48b4-8b67-4a9d-8b92-2be816dcc1d9.png)

## Alternatives Considered

We originally thought it should be implemented in project layer since there are
many hard-coded registers value, which considered as project-specific parameter
, after we got the big picture of the whole firmware-related service, we think
that extract to common layer is necessary.

## Impacts

- None.

## Organizational

- Does this repository require a new repository? --> Yes
- Who will be the initial maintainer(s) of this repository? --> Delphine CC Chiu
<delphine_cc_chiu@wiwynn.com>
- Which repositories are expected to be modified to execute this design? --> None.
- Make a list, and add listed repository maintainers to the gerrit review.
    - patrick@stwcx.xyz
    - garnermic@gmail.com
    - amithash@meta.com
    - delphine_cc_chiu@wiwynn.com
    - bonnie_lo@wiwynn.com

## Testing

- Unit tests
- The Firmware Handlers needs to conduct a series of intergration tests
depending on the functionalities it provides, for example, firmware update
needs to test the following scenarios, -update successfully, -check user input
failed, -check image file failed, etc.
    - Currently, Lattice LCMXO3 CPLD is supported.
    - Intel MAX10 CPLD, Aspeed AST1030, MXIC MX25L51245GMI-08G, WINBOND
W25Q512JVFIQ will be tested under the same scenarios.

