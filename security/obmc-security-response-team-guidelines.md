# Security response team guidelines

These are the guidelines for the security response team members
including OpenBMC community members who are responding to problems
reported by the [security vulnerability reporting process](./obmc-security-response-team.md).

The security response team coordinates activity to address privately
disclosed security vulnerabilities, engages resources to address them,
and creates security advisories.

Here are the primary expectations:
 - Keep problems private until announce
 - Work with diligence
 - Keep stakeholders informed

Work flow highlights:

  1. Handle new problem reports
    - Within a day, acknowledge you received the report.
      Note that reports are archived in the mailing list.
    - Communicate within the security response team, typically be
      cc'ing the openbmc-security email list.

  2. Analyze the problem
    - Determine if the problem new or known.
    - Determine if the problem is in OpenBMC.
       - If the problem is in a project that OpenBMC uses, re-route
         the problem to thaat upstream project.
       - Note that the problem may be in a customized version of
         OpenBMC but not in OpenBMC itself.
    - Determine which OpenBMC areas should address the problem.
    - Draft a CVE-like report which includes only:
       * the vulnerability description: omit OpenBMC specifics
       * [CVSS metrics](https://www.first.org/cvss/calculator/3.0)
       * CVE identifiers, if known
    - Gather data for the security advisory (see template below).

  3. Bring in contributors as needed (upstream, downstream, and OpenBMC)
    - Use private channels, e.g., email.
    - Inform contacts this is private work as part of the OpenBMC
      security response team.  For example, link these guidelines.
    - Coordinate with all stakeholders and keep them informed.
    - TODO: How do we know what downstream teams to contact when we
      have a high-severity issue?

  4. For OpenBMC problems:
    a. Determine if this is a high severity problem.  Example using
       CVSS metrics: a remotely exploitable or low complexity attack that has
       high impact to the BMC's confidentiality, integrity, or availability.
    b. Avoid pre-announcing problems.  Be especially careful with high
       severity problems.  When fixing the problem, use the contribution
       process but limit the details in the issue or use a
       private channel to discuss.
    c. Negotiate how the code review will proceed, typically:
        - As a series of [Patch set](https://en.wikipedia.org/wiki/Patch_(Unix)) emails
        - Use a Gerrit [private change](https://gerrit-review.googlesource.com/Documentation/intro-user.html#private-changes).
    d. When agreed, publish a security advisory to:
       https://github.com/openbmc/openbmc/issues.
    e. Improve OpenBMC processes to avoid future problems.

## DRAFT Template: Initial response to the problem submitter
The OpenBMC security response team has received the problem <date>
<title>.  Thank you for reporting this.
<Share preliminary results of the analysis.>
<Share preliminary OpenBMC plans, or minimally "we are analyzing".>
<Set expectations for follow-up communications.>

## DRAFT Template: OpenBMC Security Advisory
OpenBMC Security Advisory <id> <title> <date>
Summary: <include CVEs>
Releases affected: <release IDs, commit-ids>
Description: ...
Solution: <typically: pick up latest code from: repo + commit-id>
Problems fixed: <link to issues>
Contact info: https://github.com/openbmc/openbmc

## Reference
Some of these guidelines were collected from:
 - https://bestpractices.coreinfrastructure.org/en/projects/34
 - https://www.kernel.org/doc/html/v4.16/admin-guide/security-bugs.html
 - https://oss-security.openwall.org/wiki/mailing-lists/distros

## Team composition and email maintenance

The security response team is controlled by the OpenBMC Technical
Steering Committee.  Membership is restricted to a core group, with
selection based upon their community role(s), experience, and
expertise responding to security incidents.

The security response team uses the `openbmc-security at
lists.ozlabs.org` private email list as a channel for confidential
communication, so its membership reflects the composition of the
security response team.  The list membership should be reviewed
periodically and can be managed from
`https://lists.ozlabs.org/listinfo/openbmc-security`.

The email list subscribers should be reminded periodically to protect
access to the emails from the list because of the sensitive
information they contain.

The email list membership is not intended to be secret. For example,
we can discuss it a public forum. However, no effort is made to make
the list public.

The email list identification could be `for privately reporting
OpenBMC security vulnerabilities` and its description could be: This
email list is for privately reporting OpenBMC security
vulnerabilities.  List membership is limited to the OpenBMC security
response team.  For more information, see
https://github.com/openbmc/docs/blob/master/security/how-to-report-a-security-vulnerability.md

Sample response for denying list membership:
```
Thanks for your interest in OpenBMC security.  Subscriptions to the
openbmc-security@lists.ozlabs.org email list are by invitation only
and are typically extended only to security response team members.
For more information, see https://github.com/openbmc/docs/security or
attend a security working group meeting:
https://github.com/openbmc/openbmc/wiki/Security-working-group.

Yours truly,
OpenBMC security response team
```
