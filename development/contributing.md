# Contributing Guidelines

This document attempts to outline some basic rules to follow when contributing
to OpenBMC. It does *not* outline coding style; we follow the
[OpenBMC C++ style guide](https://github.com/openbmc/docs/blob/master/cpp-style-and-conventions)
for that.

## Organizing Commits

A good commit does exactly one thing. We prefer many small, atomic commits to
one large commit which makes many functional changes.
 - Too large: "convert foo to new API; also fix CVE 1234 in bar"
 - Too small: "move abc.h to top of include list" and "move xyz.h to bottom of
   include list"
 - Just right: "convert foo to new API" and "convert foo from tab to space"

Often, creating small commits this way results in a number of commits which are
dependent on prior commits; Gerrit handles this situation well, so feel free to
push commits which are based on your change still in review. However, when
possible, your commit should stand alone on top of master - "Fix whitespace in
bar()" does not need to depend on "refactor foo()". Said differently, ensure
that topics which are not related to each other semantically are also not
related to each other in Git until they are merged into master.

When pushing a stack of patches (current branch is >1 commits ahead of
origin/master), these commits will show up with that same relationship in
gerrit. This means that each patch must be merged in order of that relationship.
So if one of the patches in the middle needs to be changed, all the patches from
that point on would need to be pushed to maintain the relationship. This will
effectively rebase the unchanged patches, which would in turn trigger a new CI
build. Ideally, changes from the entire patchset could be done all in one go to
reduce unnecessary rebasing.

When someone makes a comment on your commit in Gerrit, modify that commit and
send it again to Gerrit. This typically involves `git rebase --interactive` or
`git commit --amend`, for which there are many guides online. As mentioned in
the paragraph above, when possible you should make changes to multiple patches
in the same stack before you push, in order to minimize CI and notification
churn from the rebase operations.

Commits which include changes that can be tested by a unit test should also
include a unit test to exercise that change, within the same commit. Unit tests
should be clearly written - even moreso than production code, unit tests are
meant primarily to be read by humans - and should test both good and bad
behaviors. Refer to the
[testing documentation](https://github.com/openbmc/docs/blob/master/testing/local-ci-build.md)
for help writing tests, as well as best practices.

## Formatting Commit Messages

Your commit message should explain:

 - Concisely, *what* change are you making? e.g. "docs: convert from US to UK
   spelling" This first line of your commit message is the subject line.
 - Comprehensively, *why* are you making that change? In some cases, like a
   small refactor, the why is fairly obvious. But in cases like the inclusion of
   a new feature, you should explain why the feature is needed.
 - Concisely, *how* did you test? This comes in the form of a "Tested:" footer
   in your commit message and is required for many OpenBMC projects. If the
   change required manual testing, describe what you did and what happened; if
   it used to do something else before your change, describe that too.  If the
   change can be validated entirely by running unit tests, say so in the
   "Tested:" tag.

Try to include the component you are changing at the front of your subject line;
this typically comes in the form of the class, module, handler, or directory you
are modifying. e.g. "apphandler: refactor foo to new API"

Loosely, we try to follow the 50/72 rule for commit messages - that is, the
subject line should not exceed 50 characters and the body should not exceed 72
characters. This is common practice in many projects which use Git.

All commit messages must include a Signed-off-by line, which indicates that you
the contributor have agreed to the Developer Certificate of Origin. This line
must include the name you commonly use, often a given name and a family name or
surname. (ok: A. U. Thor, Sam Samuelsson, robert a. heinlein; not ok:
xXthorXx, Sam, RAH)

## Sending Patches

Most projects in OpenBMC use Gerrit to review patches. Please check the
MAINTAINERS file to determine who needs to approve your review in order for your
change to be merged. Submitters will need to manually add their reviewers in
Gerrit; reviewers are not currently added automatically. Maintainers may not see
the commit if they have not been added to the review!

## Pace of Review

Contributors who are used to code reviews by their team internal to their own
company, or who are not used to code reviews at all, are sometimes surprised by
the pace of code reviews in open source projects. Try to keep in mind that those
reviewing your patch may be contributing to OpenBMC in a volunteer or
partial-time capacity, may be in a timezone far removed from your own, and may
have very deep review queues already of patches which have been waiting longer
than yours.

If you feel your patch has been missed entirely, of course it's
alright to email the maintainers (addresses available in MAINTAINERS file) - but
a reasonable timeframe to do so is on the order of a week, not on the order of
hours.

The maintainers' job is to ensure that incoming patches are as correct and easy
to maintain as possible. Part of the nature of open source is attrition -
contributors can come and go easily - so maintainers tend not to put stock in
promises such as "I will add unit tests in a later patch" or "I will be
implementing this proposal by the end of next month." This often manifests as
reviews which may seem harsh or exacting; please keep in mind that the community
is trying to collaborate with you to build a patch that will benefit the project
on its own.

## Code of Conduct

We enthusiastically adhere to our
[Code of Conduct](https://github.com/openbmc/docs/blob/master/code-of-conduct.md)
. If you have any concerns, please check that document for guidelines on who can
help you resolve them.
