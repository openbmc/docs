# OpenBMC CNA Application

OpenBMC is applying to become a CVE Numbering Authority (CNA) through
the [Distributed Weakness Filing Project](https://distributedweaknessfiling.org).

Please review the proposed answers for the [DWF CNA Registration Form](https://cna-form.distributedweaknessfiling.org).
The form has additional details for each question.

## Proposed answers

Organization name: OpenBMC
Organization URL: https://www.openbmc.org
Organization CVE/Security URL: https://github.com/openbmc/docs/blob/master/security/how-to-report-a-security-vulnerability.md
Organization Primary Email: openbmc-security@lists.ozlabs.org
Organization Secondary Email: openbmc@lists.ozlabs.org
Organization Primary GitHub account: openbmc
Organization Secondary GitHub accounts: joseph-reynolds
Organization PGP/GPG Key Fingerprint: NONE
Organization Secondary PGP/GPG Key Fingerprint: NONE
Statement of CNA coverage (e.g. "All of Foo"):
  The OpenBMC project, not including vendor, company, or hardware
  specific elements covered by another CNA.

I certify that none of answers contain Personally Identifiable
Information:
No, there is PII (e.g. I used my personal email address)

I certify that I allow the DWF to retain the entered information for
business processing reasons and that I cannot revoke consent
Yes, I'm ok with the above information being public


## Additional details CNA coverage

This section is not part of the application.  It has some of the
ideas that went into the "Statement of CNA coverage" question.  For
each area listed below, ask yourself: if there was a vulnerability in
that area, would OpenBMC or someone else write the CVE?

See existing CNA scopes here:
https://cve.mitre.org/cve/request_id.html

The main ideas are:
 - Include the entire OpenBMC project (github.com/openbmc)
 - Exclude code cloned from (upstream) Yocto or OpenEmbedded
 - Include meta-data that the OpenBMC project customized
 - Exclude forked or cloned (downstream) versions of OpenBMC
 - Include code for specific hardware or from vendors or companies,
   except when they claim coverage.

The "OpenBMC project" includes code under https://github.com/openbmc.
Its CNA includes:
 - the obmc-phosphor-image reference implementation and other
   supported implementations
 - configurations for any supported machine
 - meta-data (such as BitBake recipes) that affects which projects are
   fetched, and how they are patched and configured, except when covered
   by the OpenEmbedded or Yocto CNAs
 - patches held in forked repos, unless the patch is accepted upstream

Fetched projects (such as the Linux kernel or busybox) are not in
scope.  They would be covered by an open source CNA.  Meta-data that
says how the projects are fetched, configured, or used is part of the
OpenBMC project and may be in scope.

Forked projects for which OpenBMC carries patches (such as the Linux
kernel) may be in scope, especially for patches not accepted upstream.

Code for specific hardware (machines, boards (BSPs), board elements,
etc.) or code from specific vendors or companies is in scope.  Except
where another CNA covers the code.

OpenBMC expects that other groups which contribute to OpenBMC (such as
vendors, companies, or hardware specific elements) may have their own
security programs. OpenBMC intends to coordinate with those groups
security response teams and CNAs to resolve issues with overlapping
scope.

Versions outside of the OpenBMC organization are not in scope, except
where the problem is also present in the OpenBMC project.
