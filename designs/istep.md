# istep and CLI debug tools

Author:
	Aravind T Nair

Other contributors:
	Dhruvaraj S

Created:
	September 12, 2022

## Problem Description

IBM's upcoming systems with multiple sleds would require a framework which
would enable a user to run 'istep' and general command line (CLI) tools on a
single sled as well as across all the sleds. The multiple sleds would be cabled
together into a composed system. Each sled in a composed system would have a
BMC which would manage that particular sled. One of the sled BMC's would host a
set of applications called system coordinator application (SCA).

The istep command is used to boot the system by steps defined in the IPL flow
document. There needs to be a mechanism to run istep commands concurrently on
all the sleds in a composed system. Similarly there is a list of commands that
need to run either on just the individual sled or across all the sleds in a
composed system and return the output to the user. These tools are used mainly
for debug purposes but one major use-case is manufacturing where such tools are
extensively used.

## Background and References


## Requirements

The following are the high level requirements:

- istep and other CLI tools will not create any error logs.

They are expected to return a command "return code" which would denote success
or failure of the executed command. In scenarios where the command is supposed
to return a result, those will be returned. When CLIs are run on a multi sled
system, the SCA BMC would be responsible for routing and collating the results
from the targeted sleds.

- There needs to be a framework or mechanism to forward istep requests and CLI
tool options from the SCA BMC to the satellite BMCs. Command line tool options
here refers to getscom, puscom, getcfam, putcfam, getgpr, putgpr, getspr,
putspr, memory commands (this list is not exhaustive). This framework could be
theoretically used for commands like the 'peltool' to collect and consolidate
PELs from all the sleds in a composed system.

This would depend on the sled(s) which are targeted by parsing out the input
arguments of the command executed. There would be cases where say a getscom
command is targeted at a particular unit within a specific sled OR in there
maybe a case where a getscom command is targeted at ALL units of a particular
type across ALL sleds in a composed system. The routing application on the SCA
BMC should have the intelligence to parse and route the requests accordingly.

- Mechanism to get the istep command / CLI tool status from each BMCs

As an addendum to the previous bullet, there needs to be a way to return the
command results to the SCA BMC. This could be via asynchronous events or tasks.
In all scenarios, a return code is expected from the targeted sled(s).
Additionally if the command is expected to return a result, that result from
the satellite BMCs need to be collated by the SCA BMC and presented to the
user.

## Proposed Design

The general direction for implementing this is to have a custom lightweight
framework based on scripting versus a heavy application or a daemon running on
the SCA BMC.

Command requests coming to the SCA BMC would be parsed. The framework would
then ssh into the SCA BMC and issue special SCA 'istep'.  The 'istep' could look
for the SCA app and run against all sleds if SCA. There is also a case where the
user may want to run istep on just the SCA sled. These would be handled based
on the arguments passed in. There needs to be a special 'SCA mode' environment
so that commands like 'istep' operate on SCA basis. This means, in the special
mode the command would get executed only on the SCA BMC and in other case, the
command would be routed to all the satellite BMCs and the result collated.

Current stake in the ground would be to avoid adding new Redfish API's. If we
already have a Redfish API, then those can be utilized but the direction is NOT
to add new APIs specifically for this requirement. The direction in such cases
would be an implementation via scripts.

## Alternatives Considered

The alternative proposal is to make use of Cronus and get rid of the
requirement of having special tooling to support istep and CLI tools on multi
sled systems. But given the challenges for a layperson in setting up the Cronus
environment and the relative lack of ease of use of the Cronus environment is a
detriment in pursuing this path. Setting up the Cronus environment is
challenging and requires access to LCBs (Linux Companion Boxes). Moreover
Cronus would need access to internal IP addresses to get its server running
which again is not trivial to set and run from a user perspective.

The general consideration for debug or lab only tools is the usability factor.
Unlike a full fledged customer environment, the priority here would be get the
necessary commands run against the right sleds in a composed system. To that
end, a simplified, easily expandable and lightweight framework is preferred
over Cronus.

## Impacts

TBD


## Organizational

 - Does this repository require a new repository? TBD
 - Who will be the initial maintainer(s) of this repository? TBD
 - Which repositories are expected to be modified to execute this design? TBD
 - Make a list, and add listed repository maintainers to the gerrit review. TBD

## Testing

TBD



