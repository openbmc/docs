# OpenBMC meta layer guidelines

While most of these could be called "rules", in specific scenarios might be
actively against the intended goals. This is why each guideline has a very large
"Why" section, to ensure that the intent is being met, and that if exceptions to
the rules exist, then can be understood and managed by the project. In general,
if there's a question, and the community agrees, these guidelines can be
overridden on a case by case basis.

## Meta layers should not patch projects that exist within the openBMC tree

**Why?**

In general, keeping the codebase building in the long term is difficult. Opening
the possibility that patches exist that repo maintainers aren't aware of makes
it much more likely that a single machine breaks, or we have behavior
differences between two repos.

Also, in general, the maintainer is there to ensure that the greater community,
features, and codebase are prioritized over any one patch, and generally does so
in code review. If patches are checked into meta layers, generally the
maintainer is not a reviewer, thereby defeating most of the purpose of the role
of the maintainer.

**What should I do instead?**

Discuss with the project maintainers and the community about whether or not the
feature you're building needs to be configurable, or if it can be applied to all
projects. If it can be applied to all, simply check it into the master branch
through a gerrit review, and follow the processes outlined for the repository.
If it needs to be per-project or per-machine configurable, check it in under a
compile time option, at the suggestion of the maintainer, and add a
PACKAGECONFIG entry that can be set to enable it.

## Meta layers should not patch Yocto recipes and projects

**Why?**

Yocto itself is an open source project that accepts contributions. The more
changes that OpenBMC stacks against Yocto recipes, the more unmaintainable it
becomes, and the longer it takes to rebase to new Yocto versions. In general,
the Yocto community is as responsive (sometimes much faster) than the OpenBMC
community in regards to pull requests.

**What should I do instead?**

Submit any changes needed to the Yocto upstream repositories, using their
process. If the Yocto process has gone several weeks without responses,
cherry-pick the commit into the OpenBMC tree, with a pointer to the review in
the commit message, and documentation. The patch commit message should follow
the Yocto requirements for patch comments, including a statement of the patch
upstream status.
[yocto patch requirements](https://wiki.yoctoproject.org/wiki/Best_Known_Methods_%28BKMs%29_for_Package_Updating#Patch_Comments)

## Meta layers should avoid using EXTRA_OEMAKE and EXTRA_OEMESON

**Why?**

There are some OpenBMC projects that are utilized outside of OpenBMC. As such,
there are configuration items that are not intended to be used in OpenBMC, or
configuration items that would pose a security risk. Also, as options change and
are deprecated, the project needs a single place to update the available config
items and dependencies.

In addition, subprojects might change their build tooling, for example from
autotools to meson, in pursuit of other goals. Having tool-specific
configurations makes that change far more difficult to do.

**What should I do instead?**

In the root recipe, add a PACKAGECONFIG entry for the feature in question, then
use that to enable said feature in your meta layer.

## Meta layers should not have recipes that point to proprietary licensed code

**Why?**

OpenBMC is an open source project, and is intended to be built from source, with
appropriate distribution licenses such that it can be reused. Pointing to
commercially licensed repositories actively opposes that goal.

**What should I do instead?**

Find an equivalent open source project that meets the needs, or request that the
project owner relicenses their project.

## Meta layer recipes should only point to well maintained open source projects

**Why?**

Without this guideline, a loophole is present that allows OpenBMC developers to
bypass code review by pointing the upstream recipe to a public repository that
they control, but which OpenBMC has no input on the content of. This splits the
discussion forums in unproductive ways, and prevents all the other good
processes within OpenBMC like bug tracking and continuous integration from
having an effect.

**What should I do instead?**

The advice tends to be on a case by case basis, but if the code is only intended
for use on OpenBMC, then push a design doc, and push the code to openbmc gerrit
under the openbmc/openbmc repository where it can be reviewed, along with an
OWNERS file, signaling your willingness to maintain this project. Then, once the
community has looked through your design, a repo will be created for code to be
pushed to. If you're pulling in code from a dead project, inquire to the
community through the mailing list or discord whether or not the OpenBMC
community would be willing to adopt support and maintenance of said project.

## Meta layers should not point to OpenBMC specific repositories outside of

https://github.com/openbmc

**What defines an OpenBMC specific repository?**

Generally an OpenBMC specific repository is something that does any of the
following:

- Relies on or implements on OpenBMC defined Dbus interfaces.
- Is not designed with configurability to be used outside of OpenBMC
- Relies on OpenBMC specific layouts, configuration information to accomplish
  its primary function.

**Why?**

OpenBMC as a codebase needs to remain buildable in the long term. Companies,
maintainers, and contributors come and go over the course of the project.
Keeping all long-term-supported code in the OpenBMC github ensures that the
project will remain useful. In addition, having significant portions of
OpenBMC-specific code outside of github limits the ability of the project to do
refactoring that might be necessary to keep up with the latest best practices,
Yocto updates, and package revisions, as there isn't always a clear path to
getting patches accepted when repositories are not within the OpenBMC github
project

**What should I do instead?**

Discuss the code you'd like to write with the community using the mailing list
and/or discord. If the design would require a new repository, request one using
the OpenBMC technical oversight committee process.

## Don't use SRCREV="${AUTOREV}" in a recipe

**Why?**

Repository branches can change at any time. Pointing to an autorev revision
increases the likelihood that builds break, and makes builds far less
reproducible.

In addition, having an accounting of exactly what is in your build prevents
errors when a repo is quietly updated while working, and suddenly changes
significantly.

**What should I do instead?**

Point SRCREV to a specific commit of the repository, and increase the revision
either via the autobump script in CI, which can be requested on the mailing
list, or manually as new revisions exist.
