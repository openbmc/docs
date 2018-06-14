# OpenBMC release planning security checklist

This is a list of security work items for OpenBMC release planning.
The idea is to work through this list at least once per release.

Checklist items:
 1. Read through the security docs, identify applicable items, and
    perform them.  (This item is a place-holder.)

Ideas:
 - Create work items for the release's security goals.
 - Identify vulnerabilities that were caused by OpenBMC code and
   document them (preferably as CVEs) along with their mitigation .
 - Compile the list of OpenBMC's upstream packages (or one list per
   configuration?) and document each package along with a link to its
   security info (such as the CVEs the new release fixed).
 - Update OpenBMC (bitbake recipes) to retrieve newer upstream
   packages that fix security vulnerabilities.
 - Perform security testing (with selected configurations).
 - Perform static code analysis.
 - Review project configuration items such as:
    + GitHub/repo owners and maintainers: has someone left the
      project?
    + Jenkins and Gerrit server authorizations
    + Remove obsolete repositories
