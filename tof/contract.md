# OpenBMC Technical Oversight Forum (TOF)

This is a working document and subject to change.

The OpenBMC TOF or Technical Oversight Forum represents the technical leadership
of the OpenBMC project.

The TOF handles the processes around accepting new features into OpenBMC,
creating new subprojects (git repositories), approving subproject maintainers,
handling and enforcement of subproject maintainer issues, and many other
technical leadership concerns related to OpenBMC. This source of this document
is in the [OpenBMC documentation repository](https://github.com/openbmc/docs).
The TOF updates and maintains this document, using the process documented
within, and it can be considered authoritative unless directly overridden by the
TSC.

## Working Principles

- Decision making is a spectrum between building broad consensus, in which
  everyone has agreement, and providing guidance quickly, even when there are
  strongly differing view-points. This group should intend to work towards broad
  consensus, with a balance towards moving forward when there is minor
  disagreement.
- Members within this forum are representatives of the development community as
  a whole and not as representatives for the entities they are employed by. As
  such, decisions should be made based on merits of the decision at hand and not
  on impacts to various member entities.
- Encouraging progress, experimentation, and decision making within the project
  is the core purpose of the TOF and not to create processes and barriers to
  development. Velocity should be favored over perfection, except as a rationale
  for ignoring well-reasoned issues.

## Role and responsibilities

Issues the TOF handle include:

- Approval of new bitbake meta layers.
- Approval of new subprojects.
- Supervising subproject maintainers.
- Resolving subproject maintainer disputes.
- Guidance and oversight of the technical direction of OpenBMC.

## Current members

The current TOF members and terms are maintained
[here](https://github.com/openbmc/docs/blob/master/tof/membership-and-voting.md#terms-and-elections).

The TOF shall have a minimum of 5 and maximum of 9 members at any given time.

The chair rotates month to month.

Chair responsibilities:

- Preparing the agenda.
- Taking meeting minutes.
- Documenting decisions on GitHub.

Members are elected by community contributors yearly. The voting process will be
determined by the TOF at a later date and updated in this document. TOF
candidates should have a breadth of knowledge about the OpenBMC project. Ideal
candidates will also have a public history of fostering collaboration between
teams.

Resignation of TOF members will be handled as an empty/reduced seat until the
next voting session.

## Github issues

The TOF tracks any ongoing decisions using GitHub issues in the
[TOF repository](https://github.com/openbmc/technical-oversight-forum/issues).
Issues can be opened by anyone, including TOF members themselves. Issues can be
requests for process or technical changes, or solicitations for the opinion of
the TOF. When an issue is opened the TOF will respond to a proposal or a
solicitation, or add it to the next TOF meeting agenda for TOF discussion.

Once an issue has a proposal, TOF members have 8 days to vote on the proposal in
one of three ways: for, against, or “needs discussion”. After 8 days, if the
proposal has at least three votes for and no other votes, the proposal is
approved. Alternatively, if the proposal has at least three votes against and no
other votes, the proposal is rejected and closed. Any other vote count results
in the issue being put on the next TOF meeting agenda. To ensure proposals do
not stagnate, if the initial 8 days elapses and the minimum number of votes has
not been met, the proposal is extended by an additional 6 days and then put onto
the next TOF meeting agenda after 2 weeks.

Issue vote indicators by reacting to the top post:

- For - 👍 (`:+1:`)
- Against - 👎 (`:-1:`)
- Needs discussion - 👀 (`:eyes:`)

## Meetings

The TOF meets bi-weekly. Any requests for consideration by the TOF should be
submitted via a GitHub issue using the process documented earlier in this
document.

Meetings require a quorum of the TOF to be present; quorum is defined as:

| Active TOF membership | Quorum |
| --------------------- | ------ |
| 5                     | 4      |
| 6                     | 5      |
| 7                     | 5      |
| 8                     | 5      |
| 9                     | 6      |

During the meeting, the TOF discusses proposals under dispute and votes on them.
A proposal is rejected if it does not reach majority approval or there is more
than one dissenting vote.

It is the responsibility of the TOF chairperson to make a public record of the
decisions of the meeting.

## Discord channel

The TOF has a private Discord channel for forum member coordination and, in rare
situations, potentially sensitive topics. Sensitive topics would be topics
having security or privacy concerns, such as those involving actions of an
individual developer. The TOF chairperson is responsible for coordinating the
public posting of any information or decisions that do not need to remain
private, using the same process as public issues.
