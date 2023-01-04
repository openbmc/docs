# Community membership

This doc outlines the various expectations of contributor roles in OpenBMC. The
OpenBMC project is subdivided into subrepositories. Responsibilities for most
roles are scoped to these subrepos.

| Role                | Expectations                                           | Requirements                                                                                           | Defined by                                                   |
| ------------------- | ------------------------------------------------------ | ------------------------------------------------------------------------------------------------------ | ------------------------------------------------------------ |
| Code Contributor    | Abide by the code of conduct                           | Executed Contributor License Agreement                                                                 | Merged Code                                                  |
| Reviewer            | Review contributions from other members                | History of review in a subproject                                                                      | [OWNERS] file reviewer entry                                 |
| Platform Maintainer | Review and maintain contributions to a single platform | History of testing and ownership of a given single platform                                            | [OWNERS] entry within the meta-\* subfolder for the platform |
| Approver            | Review and maintain contributions to a portion of code | Demonstrated responsibility and excellent technical judgement for the portion of code across platforms | subproject [OWNERS] file _owners_ entry                      |
| Subproject owner    | Set direction and priorities for a subproject          | Demonstrated responsibility and excellent technical judgement for the subproject across platforms      | subproject [OWNERS] file _owners_ entry                      |
| TOF Member          | Set direction and priorities for the project           | Elected via [TOF election cycle]                                                                       | Entry in TOF documentation in docs repository                |

## New contributors

[New contributors] should be welcomed to the community by existing members,
helped with review workflow, and directed to relevant documentation and
communication channels.

## Established community members

Established community members are expected to demonstrate their adherence to the
principles in this document, familiarity with project organization, roles,
policies, procedures, conventions, etc., and technical and/or writing ability.
Role-specific expectations, responsibilities, and requirements are enumerated
below.

## Member

Members are continuously active contributors in the community. They can have
issues and reviews assigned to them, participate in design reviews through
Gerrit, and pre-submit tests are automatically run for their reviews. Members
are expected to remain active contributors to the community.

**Defined by:** Listed on a current contributor license agreement.

### Expectations

- Have made multiple contributions to the project or community. Contribution may
  include, but is not limited to:
  - Authoring or reviewing code reviews on Gerrit
  - Filing or commenting on issues on GitHub
  - Contributing to design review, subproject, or community discussions (e.g.
    meetings, Discord, mailing list discussions)
- Subscribed to the [mailing list]
- Have read the [contributor guide]
- Actively contributing to 1 or more subprojects.
- **[Submit a CLA]**

## Reviewer

Reviewers are capable of reviewing code for quality and correctness on some part
of a subproject. They are knowledgeable about both the codebase and software
engineering principles.

**Defined by:** _reviewers_ entry in an OWNERS file in a repo owned by the
OpenBMC project.

Reviewer status is scoped to a part of the codebase.

**Note:** Acceptance of code contributions requires at least one approver in
addition to the assigned reviewers.

### Expectations

The following apply to the part of the codebase for which one would be a
reviewer in an [OWNERS] file.

- Member for at least 3 months
- Primary reviewer for at least 5 changes to the codebase
- Reviewed or merged at least 5 substantial changes to the codebase
- Knowledgeable about the codebase
- Sponsored by a subproject approver
  - With no objections from other approvers
  - Done through PR to update the OWNERS file
- May either self-nominate or be nominated by an approver in this subproject.

### Responsibilities and privileges

The following apply to the part of codebase for which one would be a reviewer in
an [OWNERS] file.

- Code reviewer status may be a precondition to accepting large code
  contributions
- Responsible for project quality control via [code reviews]
  - Focus on code quality and correctness, including testing and factoring
  - May also review for more holistic issues, but not a requirement
- Expected to be responsive to review requests as per [community expectations]
- Assigned changes to review related to subproject of expertise
- Assigned test bugs related to subproject of expertise

## Platform Maintainer

Platform maintainers are able to review configuration changes for quality and
correctness on meta layers and subsystems that apply to a single platform. They
are knowledgeable about the specific constraints on a given platform, and have
access to an instance of said platform to test.

**Defined by:** _owners_ entry in an OWNERS file in a machine meta layer in the
[main OpenBMC repository].

### Expectations

The following apply to the part of codebase for which one would be a platform
owner in an [OWNERS] file.

- Member for at least 3 months
- Primary reviewer for at least 5 reviews to the codebase
- Knowledgeable about the specific platforms constraints
- Access to a platform to test and run code
- Demonstrated knowledge of bitbake metadata
- Sponsored by a root approver from openbmc/openbmc OWNERS file
  - With no objections from other approvers
  - Done through PR to update the OWNERS file
- May either self-nominate or be nominated by an approver in this subproject.

### Responsibilities and privileges

The following apply to the part of codebase for which one would be a reviewer in
an [OWNERS] file.

- Having an owner with platform maintainer status may be a precondition to
  accepting a new platform
- Responsible for platform stability
  - Testing on a regular cadence (base expectation is every quarter)
  - Providing results and insight to the community on platform-specific issues.
- Expected to be responsive to review requests as per [community expectations]
- Assigned changes to review and test related to the platform

## Approver

Code approvers are able to both review and approve code contributions. While
code review is focused on code quality and correctness, approval is focused on
holistic acceptance of a contribution including: backwards / forwards
compatibility, adhering to API conventions, subtle performance and correctness
issues, interactions with other parts of the system, etc.

**Defined by:** _owners_ entry in an OWNERS file in a repo owned by the OpenBMC
project.

Approver status is scoped to a part of the codebase.

### Expectations

The following apply to the part of the codebase for which one would be an
approver in an [OWNERS] file.

- Reviewer of the codebase for at least 9 months
- Primary reviewer for at least 10 substantial changes to the codebase
- Reviewed or merged at least 30 changes to the codebase
- Access to a suitable platform environment
- Nominated by two subproject or TOF root owner sponsors
  - With no objections from other subproject owners
  - Done through PR to update the subprojects top-level OWNERS file
  - **Note the following expectations for sponsors**:
    - Sponsors must have close interactions with the prospective member - e.g.
      code/design/proposal review, coordinating on issues, etc.
    - Sponsors must be approver in at least one subproject.
    - Sponsors must be from multiple member companies to demonstrate integration
      across community.

### Responsibilities and privileges

The following apply to the part of the codebase for which one would be an
approver in an [OWNERS] file.

- Approver status may be a precondition to accepting large code contributions
- Demonstrate sound technical judgement
- Responsible for project quality control via [code reviews]
  - Focus on holistic acceptance of contribution such as dependencies with other
    features, backwards / forwards compatibility, API and flag definitions, etc
  - Maintain a codebase that is free from unnecessary or unused code, and remove
    previous contributions that are no longer used and/or maintained.
- Expected to be responsive to review requests as per [community expectations]
- Mentor contributors and reviewers
- May approve code contributions for acceptance

## Subproject Owner

**Note:** This is a generalized high-level description of the role, and the
specifics of the subproject owner role's responsibilities and related processes
may be more specific for individual subprojects, as defined in their respective
OWNERS files.

Subproject Owners are the technical authority for a subproject in the OpenBMC
project. They _MUST_ have demonstrated both good judgement and responsibility
towards the health of that subproject. Subproject Owners _MUST_ set technical
direction and make or approve design decisions for their subproject - either
directly or through delegation of these responsibilities.

**Defined by:** _owners_ entry in subproject [OWNERS]

### Expectations

The per-repository requirements for becoming an subproject Owner should be
defined in the OWNERS file of the subproject. Unlike the roles outlined above,
the Owners of a subproject are typically limited to a relatively small group of
decision makers and updated as fits the needs of the subproject.

The following apply to the subproject for which one would be an owner.

- Deep understanding of the technical goals and direction of the subproject
- Deep understanding of the technical domain of the subproject
- Sustained contributions to design and direction by doing all of:
  - Authoring and reviewing proposals
  - Initiating, contributing and resolving discussions (emails, GitHub issues,
    meetings)
  - Identifying subtle or complex issues in designs and implementation reviews
  - Ensure through testing that the subproject is functional on one or more
    platforms
- Directly contributed to the subproject through implementation and/or review
- Meet all subproject specific requirements outlined in the OWNERS file

### Responsibilities and privileges

The following apply to the subproject for which one would be an owner.

- Make and approve technical design decisions for the subproject.
- Set technical direction and priorities for the subproject.
- Mentor and guide approvers, reviewers, and contributors to the subproject.
- Ensure continued health of subproject
  - Adequate test coverage to confidently release
  - Tests are present and passing reliably (i.e. not flaky) and are fixed when
    they fail
- Ensure a healthy process for discussion and decision making is in place.
- Work with other subproject owners to maintain the project's overall health and
  success holistically.
- Keep abreast of technical changes to the overall project and maintain and/or
  delegate maintenance of the subproject to keep it aligned with the overall
  project.

## Technical Oversight Forum Member

The Technical Oversight Forum is the technical authority for the OpenBMC
project. They _MUST_ have demonstrated both good judgement and responsibility
towards the health of the OpenBMC project and be elected by the community. TOF
Members _MUST_ set technical direction and make or approve design decisions for
the project, ensure adherence to the rules in this document and others, and
promote engagement with the open source community at large.

**Defined by:** TOF membership in docs repository

### Expectations

- Deep understanding of the technical goals and direction of the project
- Knowledge of community members, technical experts within and outside the
  project, along with a history of engagement.
- Deep understanding of the technical domain of OpenBMC and BMCs in general.
- Sustained contributions to design and direction by doing all of:
  - Authoring, reviewing, and voting on proposals to the
    technical-oversight-forum repository
  - Initiating, contributing and resolving discussions that involve project-wide
    changes and expectations (emails, GitHub issues, meetings)
  - Identifying subtle or complex issues in designs and implementation that
    occur cross-project.
  - Ensure through testing that the project is functional (to the extent
    possible) for all platforms
  - Ensure that coding standards, project norms, and community guidelines are
    documented and adhered to throughout the project.
- Directly contributed to the project through implementation and/or review

### Responsibilities and privileges

- Make and approve technical design decisions for OpenBMC.
- Define milestones and releases.
- Mentor and guide approvers, reviewers, and contributors to the project.
- Create new subprojects, and ensure their addition continues the growth and
  health of the project.
- Define and maintain a healthy process for discussion and decision making.
- Work with subproject owners and platform maintainers to maintain the project's
  overall health and success holistically.

[new contributors]: /CONTRIBUTING.md#starting-out
[code reviews]: https://gerrit.openbmc.org/
[mailing list]: https://lists.ozlabs.org/listinfo/openbmc
[main OpenBMC repository]: https://github.com/openbmc/openbmc
[community expectations]: /code-of-conduct.md
[submit a cla]: /CONTRIBUTING.md#starting-out
[contributor guide]: /CONTRIBUTING.md
[tof election cycle]: /tof/membership-and-voting.md
