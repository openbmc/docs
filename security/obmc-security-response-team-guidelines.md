# Security response team guidelines

These are the guidelines for the security response team members
including OpenBMC community members who are responding to problems
reported by the [security vulnerability reporting process][].

The security response team (SRT) coordinates activity to address privately
disclosed security vulnerabilities, engages resources to address them,
and creates security advisories.

Here are the primary expectations:
 - Keep problems private until announce
 - Work with diligence
 - Keep stakeholders informed

Workflow highlights:

1. Handle new problem reports.
    - Within a day, acknowledge you received the report.
      Note that reports are archived in the mailing list.
    - Communicate within the security response team, typically be
      cc'ing the openbmc-security email list.

2. Analyze the problem and engage collaborators as needed (upstream,
   downstream, and OpenBMC).
    - Determine if the problem is new or known.
    - Determine if the problem is in OpenBMC.
       - If the problem is in a project that OpenBMC uses, re-route
         the problem to that upstream project.
       - Note that the problem may be in a customized version of
         OpenBMC but not in OpenBMC itself.
    - Determine which OpenBMC areas should address the problem.
    - Draft a CVE-like report which includes only:
       * the vulnerability description: omit OpenBMC specifics
       * [CVSS metrics][] with explanations as needed
       * CVE identifiers, if known
    - Gather data for the security advisory (see template below).
    - Use private channels, e.g., email.
    - Inform contacts this is private work as part of the OpenBMC
      security response team.  For example, link to these guidelines.
    - Coordinate with all stakeholders and keep them informed.

   Considerations in the [CERT Guide to Coordinated Vulnerability
   Disclosure][] (SPECIAL REPORT CMU/SEI-2017-SR-022) may guide the process.

   Example collaborations:
    - Submit the problem to another security response team, for example, the
      [UEFI Security Response Team (USRT)][].
    - Privately engage an OpenBMC maintainer or subject matter expert.

3. For OpenBMC problems.
    1. Determine if this is a high severity problem.  Example using
       CVSS metrics: a remotely exploitable or low complexity attack that has
       high impact to the BMC's confidentiality, integrity, or availability.
    2. Avoid pre-announcing problems.  Be especially careful with high
       severity problems.  When fixing the problem, use the contribution
       process but limit the details in the issue or use a
       private channel to discuss.
    3. Negotiate how the code review will proceed.
        - Consider [contributing][] using a Gerrit [private change][] if
          everyone has access to Gerrit.
        - Consider using [Patch set][] emails to make reviews accessible to
          all stakeholders.
    4. When agreed:
        - Publish a security advisory to
          https://github.com/openbmc/openbmc/issues and email list
          openbmc@lists.ozlabs.org.
        - Make the Gerrit review publicly viewable.
        - Email the Security Advisory to the OpenBMC community (see below).
    5. Improve OpenBMC processes to avoid future problems.

[security vulnerability reporting process]: ./obmc-security-response-team.md
[CVSS metrics]: https://www.first.org/cvss/calculator/3.0
[UEFI Security Response Team (USRT)]: https://uefi.org/security
[CERT Guide to Coordinated Vulnerability Disclosure]: https://resources.sei.cmu.edu/asset_files/SpecialReport/2017_003_001_503340.pdf
[contributing]: https://github.com/openbmc/docs/blob/master/CONTRIBUTING.md#submitting-changes-via-gerrit-server
[private change]: https://gerrit-review.googlesource.com/Documentation/intro-user.html#private-changes
[Patch set]: https://en.wikipedia.org/wiki/Patch_(Unix)

## Template: Initial response to the problem submitter
The OpenBMC security response team has received the problem.
- Thank you for reporting this.
- Share preliminary results of the analysis.
- Share preliminary OpenBMC plans or that we are analyzing the problem.
- Set expectations for follow-up communications.

## Template: OpenBMC Security Advisory
```
OpenBMC Security Advisory
Title: ...

...summary: include CVEs, releases affected, etc....

The CVSS score for these vulnerabilities is "...", with temporal score
"...", with the following notes:
https://www.first.org/cvss/calculator/3.0

The fix is in the https://github.com/openbmc/... repository as git
commit ID ....

For more information, see OpenBMC contact information at
https://github.com/openbmc/openbmc file README.md.

Credit for finding these problems: ...
```

## Template: Security Advisory notice
When the Security Advisory is created, inform the OpenBMC community by
sending email like this:

```
TO: openbmc-security@lists.ozlabs.org, openbmc@lists.ozlabs.org
SUBJECT: [Security Advisory] ${subject}

The OpenBMC Security Response team has released an OpenBMC Security Advisory:
${url}

An OpenBMC Security Advisory explains a security vulnerability, its severity,
and how to protect systems that are built on OpenBMC.  For more information
about OpenBMC Security Response, see:
https://github.com/openbmc/docs/blob/master/security/obmc-security-response-team.md
```

## Reference
Some of these guidelines were collected from:
 - https://bestpractices.coreinfrastructure.org/en/projects/34
 - https://www.kernel.org/doc/html/v4.16/admin-guide/security-bugs.html
 - https://oss-security.openwall.org/wiki/mailing-lists/distros
 - [ISO/IEC 29147:2018 vulnerability disclosure](https://www.iso.org/standard/72311.html)

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
the list's membership public.

The email list identification is `for privately reporting OpenBMC security
vulnerabilities` with description: This email list is for privately reporting
OpenBMC security vulnerabilities.  List membership is limited to the OpenBMC
security response team.  For more information, see
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
