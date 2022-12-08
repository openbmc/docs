# OpenBMC Maintainer/CLA Workflow

OpenBMC contributors are required to execute an OpenBMC CLA (Contributor License
Agreement) before their contributions can be accepted. This page is a checklist
for sub-project maintainers to follow before approving patches.

- Manually verify the contributor has signed the ICLA (individual) or is listed
  on an existing CCLA (corporate).
  - Executed CLAs can be found [in the CLA repository][1].
  - If you were not added to the appropriate CLA repository ACL send an email to
    openbmc@lists.ozlabs.org with a request to be added.
  - If a CLA for the contributor is found, accept the patch(1).
- If a CLA is not found, request that the contributor execute one and send it to
  manager@lfprojects.org.
  - Do not accept the patch(1) until a signed CLA (individual _or_ corporate)
    has been uploaded to the CLA repository.
  - The CCLA form can be found [here][2].
  - The ICLA form can be found [here][3].

An executed OpenBMC CLA is _not_ required to accept contributions to OpenBMC
forks of upstream projects, like the Linux kernel or U-Boot.

Review the maintainers' responsibilities in the
[contributing guidelines](./CONTRIBUTING.md). Maintainers are ultimately
responsible for sorting out open source license issues, issues with using code
copied from the web, and maintaining the quality of the code.

Repository maintainers ought to have the following traits as recognized by a
consensus of their peers:

- responsible: have a continuing desire to ensure only high-quality code goes
  into the repo
- leadership: foster open-source aware practices such as [FOSS][4]
- expertise: typically demonstrated by significant contributions to the code or
  code reviews

(1) The semantics of accepting a patch depend on the sub-project contribution
process.

- GitHub pull requests - Merging the pull request.
- Gerrit - +2.
- email - Merging the patch.

Ensure that accepted changes actually merge into OpenBMC repositories.

[1]: https://drive.google.com/drive/folders/1Ooi0RdTcaOWF1DWFJUAJDdN7tRKde7Nl
[2]: https://drive.google.com/file/d/1d-2M8ng_Dl2j1odsvZ8o1QHAdHB-pNSH
[3]: https://drive.google.com/file/d/1k3fc7JPgzKdItEfyIoLxMCVbPUhTwooY
[4]: https://en.wikipedia.org/wiki/Free_and_open-source_software
