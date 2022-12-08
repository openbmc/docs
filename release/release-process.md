# OpenBMC Release Process

The OpenBMC release process document is the output of the Release Planning work
group. This documents the set of topics that have been discussed and agreed upon
to date, defining such things as the release process and schedule, milestones,
content definition and feature prioritization. All the decisions were made in
the work group meetings, with representatives from each member company.

https://github.com/openbmc/openbmc/wiki/Release-Planning

## Release Cycle

### Schedule

OpenBMC has its initial formal release branch as a Linux Foundation project in
February 2019, with a new release branch following every 6 months. Releases are
purposefully offset from Yocto releases by a few weeks to allow for integration
and testing. Member companies are expected to test a release candidate on their
platform and report their findings.

### Versioning

The OpenBMC release will follow the Yocto numbering with slight modification to
allow for build information. The release version will follow the form:
Major.Minor.Fix.obmc-build. For example:

`2.6.4.obmc-2`

This would be interpreted as major release 2, minor release 6, fix release 4,
build 2.

### Milestones

Milestones are important dates within a release cycle. The milestone dates
agreed to are as follows: Design, Code, Freeze, and Release. The milestones will
each have entry and exit criteria. Milestone dates currently are not yet
implemented.

#### Design

This is the date in the release cycle when a feature's design must be discussed
openly and completed. The design should follow the published design template and
be merged before this milestone. If this is not met, then the feature is at risk
for not being merged for the forming release cycle.

The Design Template can be found here:
https://github.com/openbmc/docs/blob/master/designs/design-template.md

#### Code

This milestone represents some major functionality that has to be completed by
this date in order for the feature to be delivered on time. It is a checkpoint
for the developers doing the work to evaluate and commit to for having that part
complete. Globally there can be one or more code milestones established, but
currently these are TBD.

#### Freeze

This is the date where the release content is frozen, that is, no new major
content will be accepted. This is necessary to allow time for the new features
to be fully tested. Any defects found will be evaluated to be included or not,
based on how close the release milestone is, and how much the defect impacts
other components and features. Feature freeze will occur 2 weeks before the
release milestone. Release tagging/branching can accommodate master development
efforts to continue while a release candidate is being tested.

The [security working group][] should provide guidance to the community about
security aspects of the planned release. The idea is to provide input for the
[release notes][] and actionable advice to the [test work group][].

The [test work group][] should indicate what testing was performed and the
results of that testing. The idea is to fix problems and provide input to the
[release notes][].

[security working group]:
  https://github.com/openbmc/openbmc/wiki/Security-working-group
[test work group]: https://github.com/openbmc/openbmc/wiki/Test-work-group
[release notes]:
  https://github.com/openbmc/docs/blob/master/release/release-notes.md

#### Release

The release milestone is the release date. It is defined as happening some time
within a targeted release week. It is desired to have the release happen as soon
as possible within the release week, but the week is given to allow for typical
infrastructure problems to occur and be fixed in order for the release to
happen.

The release date is a fixed time frame and cannot be moved. If a feature is not
done by the freeze milestone, then it will not be in that release, it will have
to be included in the next release cycle. The release date will not float out
until the feature is complete.

The updated [release notes][] should be merged into the project.

## Release Content

### Content Input

Community members define release content by inputting needed work into the
release management tool(s) specified below. Typically the content is a feature
that a particular company needs and one that they are willing to allocate some
resources to see completed. The feature does not need to be fully staffed by
that company, unless it is critical to that company and no other community
member is willing to contribute.

Release content is disclosed regardless of resource commitment because the
community consists of not just member companies, but individuals interested in
furthering OpenBMC. These community members may be willing to contribute to a
feature.

### Prioritization

Prioritization happens naturally with listing the content proposed for a release
cycle. If every community member participating in the Release Planning work
group wants a particular feature and agree to help staff the effort, it
automatically becomes a higher priority than an item that is mildly needed and
not staffed.

Just as with the content being fully disclosed, it is important to maintain a
list of medium priority work that didn't make the release cut, as it may be an
area that a new community member would like to start working on.

### Work Group Formation

Work groups form in the open around features for a release, not around company
boundaries. Work groups would definitely be needed for work spanning releases,
or high priority features with multiple community members. These work groups
need a lead to form and guide the group through the release milestones and to
deliver a well designed and tested feature.

Work groups can be formal or not depending on the work needing to be done and
the community member style. They can consist of email to the list or formal
weekly meetings via Discord or call to discuss designs and progress.

### Backlog

The backlog is simply the proposed release features that did not make the
release plan. They will be additional input for the next release cycle. They are
also available for new community members looking for a new project to drive if
none of the current work is within their expertise.

### Release Management Tools

Initially a Google spreadsheet was used, then Github issues/labels, but neither
found acceptance or traction in the community. Currently broad features are
listed in the wiki:

https://github.com/openbmc/openbmc/wiki/Current-Release-Content
