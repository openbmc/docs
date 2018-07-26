# Security response team guidelines

Guidelines for security response team member activity.

Every situation is different.  Here are the primary expectations:
 - Keep problems private until announce
 - Work diligently
 - Keep stakeholders informed

Work flow highlights:

  1. Handle problem reports
    a. Record every report (in a private database?) - TO DO: Do we
       need to disclose that we are keeping this information, etc?
    b. Communicate progress with the security response team.
    c. Within a day, acknowledge you received the report

  2. Analyze the report
    - Is this problem new or known?
    - Is this problem in OpenBMC, upstream, or downstream?
    - Assess CVSS metrics:
      * CVSSv2 / Base / Access vector
      * CVSSv2 / Base / Attack complexity
      * CVSSv2 / Base / Authentication
      * CVSSv2 / Impact / Confidentiality
      * CVSSv2 / Impact / Integrity
      * CVSSv2 / Impact / Availability
      * CVSSv2 / Temporal / Exploitability
      * CVSSv2 / Temporal / Remediation Level
      * CVSSv2 / Temporal / Report Confidence
    - Gather data for the security advisory.

  3. Bring in folks as needed (upstream, downstream, and OpenBMC)
    a. Use private channels.
    b. Inform contacts this is private work as part of the OpenBMC
       security response team.  You can link to the guidelines here.
    c. Coordinate with all stakeholders and keep them informed.
    d. Gather CVEs or create new ones.

  4. For OpenBMC problems:
    a. Determine if this is a high severity problem.  Example using
       CVSS metrics: a remotely exploitable or low complexity attack that has
       high impact to the BMC's confidentiality, integrity, or availability.
    b. Avoid pre-announcing problems.  Be especially careful with high
       severity problems.  When fixing the problem, use the contribution
       process but limit the details in the issue and code review, or use a
       private channel to discuss.
    c. When agreed, publish a security vulnerability advisory to:
       https://github.com/openbmc/openbmc/issues.
    d. Improve OpenBMC processes to avoid future problems.

## DRAFT Template: Initial response to the problem submitter
TODO: Should this be emailed in plain text or encrypted?

The OpenBMC security response team has received the problem <date>
<title>.  Thank you for reporting this.
<Share preliminary results of the analysis.>
<Share preliminary OpenBMC plans, or minimally "we are analyzing".>
<Set expectations for follow-up communications.>

## DRAFT Template: OpenBMC Security Advisory

OpenBMC Security Advisory <id> <title> <date>
Summary: <include CVEs>
Releases affected: <release IDs, commit-ids>
Description: 
Solution: <typically: pick up latest code from: repo, commit-id>
Problems fixed: <link to issues>
Contact info: <boilerplate link to OpenBMC security>
 
## Reference
Some of these guidelines were collected from:
 - https://bestpractices.coreinfrastructure.org/en/projects/34
