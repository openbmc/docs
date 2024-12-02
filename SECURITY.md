# Security Policy

## How to report a security vulnerability

This describes how you can report an OpenBMC security vulnerability privately to
give the project time to address the problem before public disclosure.

The main ideas are:

- You have information about a security problem which is not yet publicly
  available.
- You want the problem fixed before public disclosure and you are willing to
  help make that happen.
- You understand the problem will eventually be publicly disclosed.

To begin the process:

- Send an email to `openbmc-security at lists.ozlabs.org` with details about the
  security problem such as:
  - the version and configuration of OpenBMC the problem appears in
  - how to reproduce the problem
  - what are the symptoms
- As the problem reporter, you will be included in the email thread for the
  problem.

The OpenBMC security response team (SRT) will respond to you and work to address
the problem. Activities may include:

- Privately engage community members to understand and address the problem.
  Anyone brought onboard should be given a link to the OpenBMC [security
  response team guidelines][].
- Work to determine the scope and severity of the problem, such as [CVSS
  metrics][].
- Work to create or identify an existing [CVE][].
- Coordinate workarounds and fixes with you and the community.
- Coordinate announcement details with you, such as timing or how you want to be
  credited.
- Create an OpenBMC security advisory.

Please refer to the [CERT Guide to Coordinated Vulnerability Disclosure][],
(SPECIAL REPORT CMU/SEI-2017-SR-022) for additional considerations.

Alternatives to this process:

- If the problem is not severe, please write an issue to the affected repository
  or email the list.
- Join the OpenBMC community and fix the problem yourself.
- If you are unsure if the error is in OpenBMC (contrasted with upstream
  projects such as the Linux kernel or downstream projects such as a customized
  version of OpenBMC), please report it and we will help you route it to the
  correct area.
- Discuss your topic in other
  [OpenBMC communication channels](https://github.com/openbmc/openbmc).

[security response team guidelines]: ./obmc-security-response-team-guidelines.md
[cvss metrics]: https://www.first.org/cvss/calculator/3.0
[cve]: http://cve.mitre.org/about/index.html
[cert guide to coordinated vulnerability disclosure]:
  https://resources.sei.cmu.edu/asset_files/SpecialReport/2017_003_001_503340.pdf
