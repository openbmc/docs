# OpenBMC Maintainer/CLA Workflow
OpenBMC contributors are required to execute an OpenBMC CLA (Contributor
License Agreement) before their contributions can be accepted.  This page is a
checklist for sub-project maintainers to follow before approving patches.

* Manually verify the contributor has signed the ICLA (individual) or is
listed on an existing CCLA (corporate).
	* Executed CLAs can be found [in the CLA repository](https://drive.google.com/drive/folders/1Ooi0RdTcaOWF1DWFJUAJDdN7tRKde7Nl).
	* If you were not added to the appropriate CLA repository ACL send an
email to openbmc@lists.ozlabs.org with a request to be added.
	* If a CLA for the contributor is found, accept the patch(1).
* If a CLA is not found, request that the contributor execute one and send it
to openbmc@lists.ozlabs.org.
	* Do not accept the patch(1) until a signed CLA (individual _or_
corporate) has been uploaded to the CLA repository.
	* The CCLA form can be found [here](https://github.com/openbmc/openbmc/files/1860741/OpenBMC.CCLA.pdf).
	* The ICLA form can be found [here](https://github.com/openbmc/openbmc/files/1860742/OpenBMC.ICLA.pdf).

(1) The semantics of accepting a patch depend on the sub-project contribution
process.

* Github pull requests - Merging the pull request.
* Gerrit - +2.
* email - Merging the patch.

** An executed OpenBMC CLA is _not_ required to accept contributions to
OpenBMC forks of upstream projects, like the Linux kernel or U-Boot.
