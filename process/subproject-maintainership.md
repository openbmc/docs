# Subproject Maintainership and Forward Progress

## Problem Description

OpenBMC is a Linux distribution specialised for Baseboard Management
Controllers (BMCs). While it exploits existing open-source projects where
possible, the OpenBMC community also maintains many integrated projects that
are specific to its use-cases.

By observation, BMC firmware encounters a lot of diversity in use-cases as well
as board and platform design. The shape of the project's community reflects the
fact that developing OpenBMC-based systems is most effectively done by the
companies designing the platforms. Reverse-engineering efforts
exist[[1][]][[2][]] but do not represent the typical path by which platform
support enters the project. As a consequence the community tends to be
dominated by people contributing to the project to support the needs of their
employer, and are broadly motivated by platform-specific requirements rather
than general capabilities. The motivation for contribution by those in the
community tends to be extrinsic.

[1]: https://github.com/Keno/openbmc/commit/327da25efde4fc54b27586be3914159136f03784
[2]: https://lore.kernel.org/lkml/7baebe77f0f8963e06d5ddeec6c737f5@rnplus.nl/

A consequence of this latter point is that engagement in the community can
greatly vary with time based on events beyond the control of contributors and
maintainers, through changes of roles, organisational direction or other factors
like completion of support for a given platform.

Extrinsic motivation driving variation in engagement is at odds with the
consistency required by maintainer roles for the many integrated projects owned
by the OpenBMC community.

---

The [Technical Oversight Forum (TOF) contract][tof-contract] paints the TOF's
role in the project with broad brush strokes:

[tof-contract]: /tof/contract.md

> The TOF handles the processes around accepting new features into OpenBMC,
> creating new subprojects (git repositories), approving subproject maintainers,
> handling and enforcement of subproject maintainer issues, and many other
> technical leadership concerns related to OpenBMC.

and:

> Issues the TOF handle include:
> - Approval of new bitbake meta layers.
> - Approval of new subprojects.
> - Supervising subproject maintainers.
> - Resolving subproject maintainer disputes.
> - Guidance and oversight of the technical direction of OpenBMC.

These broad brush strokes touch on the TOF's responsibilities towards
maintenance but the subject receives no further treatment.

---

[TOF issue #20][tof-issue-20] motivates exploration of a mechanism for it to
introduce maintainers to subprojects to ensure the community can make forward
progress when the existing maintainers become unresponsive.

[tof-issue-20]: https://github.com/openbmc/technical-oversight-forum/issues/20

## Scope

While all unresponsive maintainers are problematic maintainers to some extent,
it's not true that all problematic maintainers are unresponsive maintainers.
This discussion specifically considers defining a mechanism to allow forward
progress at the subproject scope in the face of unresponsive maintainers. It
does not consider the more general problem of problematic maintainers.

## Considerations

The social, technical and security impacts of the TOF changing a subproject's
maintainers are considered in the context of the following principle:

> [Many decisions and actions are reversible and do not need extensive
> study.][bias-for-action]

[bias-for-action]: https://www.aboutamazon.com/about-us/leadership-principles

The principle is used to prompt the question "Are the consequences easily
reversed?" for each of the concerns below.

### Social

The [maintainer-workflow][] document recognises the personal effort required to
become a maintainer:

[maintainer-workflow]: /maintainer-workflow.md

> Repository maintainers ought to have the following traits as
> recognized by a consensus of their peers:
> - responsible: have a continuing desire to ensure only high-quality
>   code goes into the repo
> - leadership: foster open-source aware practices such as [FOSS][]
> - expertise: typically demonstrated by significant contributions to
>   the code or code reviews

[FOSS]: https://en.wikipedia.org/wiki/Free_and_open-source_software

Further, the community of maintainers inside the project [isn't broad enough to
accommodate the review load][ed-is-only-human]. While unresponsive maintainers
can be frustrating for contributors and a concern for the project, the problem
needs to be weighed against the risk of alienating those who have (previously)
put in the effort to build the project to its current capabilities. Healing
alienation is at best intensive if not a futile effort; it is hard to reverse.
This suggests that the TOF stepping in to remove maintainership responsibilities
from community members without their consent is likely counter-productive,
[contrary to the initial proposal][initial-proposal].

[ed-is-only-human]: https://lore.kernel.org/all/CAH2-KxAsq8=+kYZHb9n_fxE80SuU29yT90Hb0k72bKfY8pnWEQ@mail.gmail.com/
[initial-proposal]: https://github.com/openbmc/technical-oversight-forum/issues/20#issuecomment-1272667701

### Technical

At least two technical concerns exist:

1. Contributors promoted to maintainers may lack or have no way to learn the
   nuances and context for the current state of the codebase in question. There
   may be a period (or periods) of instability until these issues are resolved.

2. An unresponsive maintainer's knowledge of the codebase may age to the point
   that it no-longer applies in the event that they attempt to return to the
   subproject

By the bias-for-action thought framework, 1 shouldn't be much of a concern in
practice - it should be feasible to revert changes if necessary. This same
approach applies in the case of 2 so long as the work is done in good faith. The
process does need to consider the scenario where changes are not made in good
faith.

### Security

Security concerns cut both ways. It's possible that:

1. Contributors promoted to maintainers knowingly make changes that impact the
   security posture of the project, or behave in a way that erodes or fails to
   build the community's trust in their decision making.

2. Not removing rights of an unresponsive maintainer from the subproject exposes
   the project to a security hazard in the event that their account is
   compromised.

The first concern suggests that the contributors must have established trust
relationships in the community before earning the responsibility of
maintainership. It's hard to reverse a problem that isn't known to exist (though
it may be possible to quickly revert the problematic code once its existence
is known). Security problems can have an impact on the project's reputation;
damage to reputation is also difficult to reverse, therefore the TOF needs to
have confidence in any promotions.

The second concern exists even for active maintainers and the process required
to handle it might be the same: Removing rights until it's established that the
compromise has been addressed. Unannounced activity from an otherwise
unresponsive maintainer should itself be notable.

### Synthesis of Considerations

By the discussion above, a method for enabling forward progress should focus on
putting guard-rails in place for the TOF to safely on-board new maintainers to a
subproject. This is in contrast to not concerning itself too much with
unresponsive maintainers, beyond recognising that they have met some bar of
unresponsiveness in order to trigger the on-boarding process for new
maintainers.

## Defining and Determining Unresponsiveness

Defining unresponsiveness sets expectations for both contributors and
maintainers. Contributors can expect to have their work reviewed inside this
period, and maintainers should endeavour to not let patches lapse.
Responsiveness is a social contract.

A tension exists between contributors needing feedback to maintain momentum
against maintainers' available time both inside and outside the project.
Maintainers are people: Life happens. Sometimes it is possible to communicate
expected absences or hand over responsibilities, other times that may not
happen. It's likely that unresponsiveness requires requires a fair margin in
favour of maintainers, and that more than one instance should be needed before
triggering the ability for the TOF to on-board new maintainers. Further, as it
is a social contract it should be a socially driven-process: Inspection of the
state of affairs in a subproject should only be performed if there are
complaints about it, rather than using automation to detect the conditions.

Unresponsiveness can appear at multiple scopes:

1. Patch scope: Maintainers may ignore a given patch while attending to others
2. Subproject scope: Maintainers may ignore a given repository while attending
   to others
3. Project scope: Maintainers may cease involvement in the project entirely

Unresponsiveness at the patch scope would suggest a failure by the contributor
and the maintainer to build consensus. It is already the role of the TOF to
mediate these cases; this proposal considers such problems as out of scope.

Unresponsiveness at the subproject scope but not the project scope suggests its
at least possible to discuss on-boarding new maintainers to affected
subprojects. Where possible, this is the preferred route to enabling forward
progress. Failure of this process is again a failure of consensus, and is
handled using existing mechanisms.

The proposal concentrates on unresponsiveness at the project scope: Without
regard to reason it is not possible to hold discussion with the maintainers in
question.

Lack of activity in a subproject is not evidence of lack of responsiveness. A
pre-condition must be that unmerged contributions exist and remain unaddressed
for the defined period.

Further, multiple instances of unresponsiveness should be measured serially.
Unresponsiveness is a property of behaviour in time: while the number of
unaddressed contributions increases the impact and may increase frustrations,
for any one period of unresponsiveness there is only one instance of the
behaviour.

Unresponsiveness also needs to be measured in terms of the set of maintainers.
If any maintainer of a subproject is responsive in the project it's possible to
make forward progress or to consider unaddressed contributions in terms of
failed consensus. Therefore it is a pre-condition that all maintainers of a
subproject are unresponsive at the project scope before the TOF may on-board new
maintainers to enable forward progress.

The following straw-person values are proposed when the pre-conditions are met:

1. Maintainers are considered unresponsive after failing to address
   contributions for a period of 1 month
2. The TOF may on-board new maintainers to the subproject after 3 consecutive
   periods of unresponsiveness

## Proposed Process

1. A complaint is raised to the TOF about lack of forward progress in a
   subproject
2. The TOF validates the complaint against the pre-conditions and definitions
3. The complaint is valid if all the pre-conditions are met and the behavior
   exceeds the limits outlined in the definitions above
4. The complaint is partially valid if the pre-conditions are met and but the
   behaviour does not exceed the limits outlined in the definitions above
5. The complaint is invalid if the pre-conditions are not met
6. If the complaint is valid the TOF notifies the current maintainers of the
   impending addition of maintainers
7. The TOF identifies the set of candidate maintainers
8. If willing candidates exist, one or more are added as subproject maintainers
9. Otherwise, the TOF members mentor the complainant on maintenance until there
   is consensus they would be considered a candidate
10. The TOF introduces the complainant as a maintainer
11. If the complaint is partially valid then the TOF must notify the maintainers
    of the impending addition of maintainers in the event of continued
    unresponsiveness
12. If the complaint is invalid then no further action is taken
