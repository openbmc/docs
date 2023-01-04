# Community membership

This doc outlines the various responsibilities of contributor roles in OpenBMC.
The OpenBMC project is subdivided into subrepositories. Responsibilities for
most roles are scoped to these subrepos.

| Role                                          | Responsibilities                                       | Requirements                                                                                      | Defined by                                   |
| --------------------------------------------- | ------------------------------------------------------ | ------------------------------------------------------------------------------------------------- | -------------------------------------------- |
| Member                                        | Active contributor in the community                    | Executed Contributor License agreement                                                            | OpenBMC GitHub org member                    |
| Reviewer                                      | Review contributions from other members                | History of review and authorship in a subproject                                                  | [OWNERS] file reviewer entry                 |
| Platform maintainer                           | Review and maintain contributions to a single platform | History of testing and ownership of a given platform instance                                     | [OWNERS] entry                               |
| within the meta-\* subfolder for the platform |
| Subproject owner                              | Set direction and priorities for a subproject          | Demonstrated responsibility and excellent technical judgement for the subproject across platforms | subproject [OWNERS] file _owners_ entry      |
| Root owner                                    | Set direction and priorities for the project           | Demonstrated responsibility and excellent technical judgement for the project                     | openbmc/openbmc [OWNERS] file _owners_ entry |

## New contributors

[New contributors] should be welcomed to the community by existing members,
helped with PR workflow, and directed to relevant documentation and
communication channels.

## Established community members

Established community members are expected to demonstrate their adherence to the
principles in this document, familiarity with project organization, roles,
policies, procedures, conventions, etc., and technical and/or writing ability.
Role-specific expectations, responsibilities, and requirements are enumerated
below.

## Member

Members are continuously active contributors in the community. They can have
issues and PRs assigned to them, participate in design reviews through Gerrit,
and pre-submit tests are automatically run for their reviews. Members are
expected to remain active contributors to the community.

**Defined by:** Member of the OpenBMC GitHub organization and on a current
[contributor license agreement].

### Requirements

- Have made multiple contributions to the project or community. Contribution may
  include, but is not limited to:
  - Authoring or reviewing code reviews on Gerrit
  - Filing or commenting on issues on GitHub
  - Contributing to design review, subproject, or community discussions (e.g.
    meetings, Discord, mailing list discussions)
- Subscribed to [openbmc@lists.ozlabs.org]
- Have read the [contributor guide]
- Actively contributing to 1 or more subprojects.
- **[Submit a CLA][contributor license agreement] to the openbmc mailing list**
  - Ensure your sponsors are @mentioned on the issue
  - Complete every item on the checklist ([preview the current version of the
    template][membership template])
  - Make sure that the list of contributions included is representative of your
    work on the project.
- Have your sponsoring reviewers reply confirmation of sponsorship: `+1`
- Once your sponsors have responded, your request will be reviewed by the
  [OpenBMC GitHub Admin team]. Any missing information will be requested.

\* _Excluding the [Contributor Playground repository]. It is configured to allow
non-org members to be included in OWNERS files for contributor tutorials and
workshops._

## Reviewer

Reviewers are able to review code for quality and correctness on some part of a
subproject. They are knowledgeable about both the codebase and software
engineering principles.

**Defined by:** _reviewers_ entry in an OWNERS file in a repo owned by the
OpenBMC project.

Reviewer status is scoped to a part of the codebase.

**Note:** Acceptance of code contributions requires at least one approver in
addition to the assigned reviewers.

### Requirements

The following apply to the part of codebase for which one would be a reviewer in
an [OWNERS] file (for repos using the bot).

- member for at least 3 months
- Primary reviewer for at least 5 PRs to the codebase
- Reviewed or merged at least 20 substantial PRs to the codebase
- Knowledgeable about the codebase
- Sponsored by a subproject approver
  - With no objections from other approvers
  - Done through PR to update the OWNERS file
- May either self-nominate, be nominated by an approver in this subproject.

### Responsibilities and privileges

The following apply to the part of codebase for which one would be a reviewer in
an [OWNERS] file (for repos using the bot).

- Tests are automatically run for Pull Requests from members of the OpenBMC
  GitHub organization
- Code reviewer status may be a precondition to accepting large code
  contributions
- Responsible for project quality control via [code reviews]
  - Focus on code quality and correctness, including testing and factoring
  - May also review for more holistic issues, but not a requirement
- Expected to be responsive to review requests as per [community expectations]
- Assigned PRs to review related to subproject of expertise
- Assigned test bugs related to subproject of expertise

## Platform Maintainer

Platform maintainers are able to review code for quality and correctness on meta
layers and subsystems that apply to a single platform subproject. They are
knowledgeable about the specific constraints on a given platform.

**Defined by:** _owner_ entry in an OWNERS file in a machine meta layer owned by
the OpenBMC project.

### Requirements

The following apply to the part of codebase for which one would be a platform
owner in an [OWNERS] file.

- member for at least 6 months
- Primary reviewer for at least 5 PRs to the codebase
- Knowledgeable about the specific platforms constraints
- Access to a platform to test
- Sponsored by a root approver
  - With no objections from other approvers
  - Done through PR to update the OWNERS file
- May either self-nominate, be nominated by an approver in this subproject.

### Responsibilities and privileges

The following apply to the part of codebase for which one would be a reviewer in
an [OWNERS] file (for repos using the bot).

- Platform maintainer status may be a precondition to accepting a new platform
- Responsible for platform stability
  - Testing on a regular cadence
  - Providing results and insight to the community on platform-specific issues.
- Expected to be responsive to review requests as per [community expectations]
- Assigned PRs to review related to the platform

## Approver

Code approvers are able to both review and approve code contributions. While
code review is focused on code quality and correctness, approval is focused on
holistic acceptance of a contribution including: backwards / forwards
compatibility, adhering to API conventions, subtle performance and correctness
issues, interactions with other parts of the system, etc.

**Defined by:** _approvers_ entry in an OWNERS file in a repo owned by the
OpenBMC project.

Approver status is scoped to a part of the codebase.

### Requirements

The following apply to the part of codebase for which one would be an approver
in an [OWNERS] file (for repos using the bot).

- Reviewer of the codebase for at least 9 months
- Primary reviewer for at least 10 substantial PRs to the codebase
- Reviewed or merged at least 30 PRs to the codebase
- Nominated by two a subproject owners
  - With no objections from other subproject owners
  - Done through PR to update the top-level OWNERS file
  - **Note the following requirements for sponsors**:
    - Sponsors must have close interactions with the prospective member - e.g.
      code/design/proposal review, coordinating on issues, etc.
    - Sponsors must be reviewers or approvers in at least one OWNERS file within
      one of the [OpenBMC GitHub organization]\*.
    - Sponsors must be from multiple member companies to demonstrate integration
      across community.

### Responsibilities and privileges

The following apply to the part of codebase for which one would be an approver
in an [OWNERS] file (for repos using the bot).

- Approver status may be a precondition to accepting large code contributions
- Demonstrate sound technical judgement
- Responsible for project quality control via [code reviews]
  - Focus on holistic acceptance of contribution such as dependencies with other
    features, backwards / forwards compatibility, API and flag definitions, etc
- Expected to be responsive to review requests as per [community expectations]
- Mentor contributors and reviewers
- May approve code contributions for acceptance

## Subproject Owner

**Note:** This is a generalized high-level description of the role, and the
specifics of the subproject owner role's responsibilities and related processes
_MUST_ be defined for individual subprojects.

Subproject Owners are the technical authority for a subproject in the OpenBMC
project. They _MUST_ have demonstrated both good judgement and responsibility
towards the health of that subproject. Subproject Owners _MUST_ set technical
direction and make or approve design decisions for their subproject - either
directly or through delegation of these responsibilities.

**Defined by:** _owners_ entry in subproject [OWNERS]

### Requirements

The process for becoming an subproject Owner should be defined in the OWNERS
file of the subproject. Unlike the roles outlined above, the Owners of a
subproject are typically limited to a relatively small group of decision makers
and updated as fits the needs of the subproject.

The following apply to the subproject for which one would be an owner.

- Deep understanding of the technical goals and direction of the subproject
- Deep understanding of the technical domain of the subproject
- Sustained contributions to design and direction by doing all of:
  - Authoring and reviewing proposals
  - Initiating, contributing and resolving discussions (emails, GitHub issues,
    meetings)
  - Identifying subtle or complex issues in designs and implementation PRs
  - Ensure through testing that the subproject is functional on one or more platforms
- Directly contributed to the subproject through implementation and / or review

### Responsibilities and privileges

The following apply to the subproject for which one would be an owner.

- Make and approve technical design decisions for the subproject.
- Set technical direction and priorities for the subproject.
- Define milestones and releases.
- Mentor and guide approvers, reviewers, and contributors to the subproject.
- Ensure continued health of subproject
  - Adequate test coverage to confidently release
  - Tests are present and passing reliably (i.e. not flaky) and are fixed when
    they fail
- Ensure a healthy process for discussion and decision making is in place.
- Work with other subproject owners to maintain the project's overall health and
  success holistically

[code reviews]: /contributors/guide/expectations.md#code-review
[community expectations]: /contributors/guide/expectations.md
[contributor license agrement]: /CONTRIBUTING.md#starting-out
[contributor guide]: /CONTRIBUTING.md
