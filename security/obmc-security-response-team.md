# The OpenBMC security vulnerability reporting process

This describes the OpenBMC security vulnerability reporting process
which is intended to give the project time to address security
problems before public disclosure.

The main pieces are:
 - a procedure to privately report security vulnerabilities
 - a security response team to address reported vulnerabilities
 - the openbmc-security email address for the response team
 - guidelines for security response team members

The basic workflow is:
 1. A community member reports a problem privately to the security
    response team.
 2. The response team works to understand the problem and privately
    engages community members to resolve it.
 3. Workarounds and fixes are created, reviewed, and approved.
 4. The security response team creates an OpenBMC security advisory
    which explains the problem, its severity, and how to protect
    your systems that were built on OpenBMC.

Note that the OpenBMC security response team is distinct from the
OpenBMC security working group which remains completely open.

The [How to privately report a security vulnerability](./how-to-report-a-security-vulnerability.md)
web page explains how OpenBMC community members can report a security
vulnerability and get a fix for it before public announcement of the
vulnerability.

The `openbmc-security@lists.ozlabs.org` email address is the primary
communication vehicle between the person who reported the problem and
the security response team, and the initial communication between the
security response team members.

The [Guidelines for security response team members](./obmc-security-response-team-guidelines.md)
contain collected wisdom for the response team and community members
who are working to fix the problem.
