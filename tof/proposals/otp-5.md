---
OTP: 5
Title: "OpenBMC TOF Proposal" process
Author: Patrick Williams <patrick@stwcx.xyz>
Create: 2021-10-20
Type: Process
Requires: N/A
Replaces: N/A
Superseded-By: N/A
---

# OTP-5: "OpenBMC TOF Proposal" Process

## Background

When the [TOF Contract][1] was written it outlined using Github Issues to track
"requests for process or technical changes, or solicitations for the opinion of
the TOF" but did not give any guidelines on how this is to be accomplished. In
the process of discussing the [first proposal][2], the overall feeling of the
TOF was that the process needed to be better defined in order to codify the
expected process and to ensure community feedback was appropriately solicited.
This is that process.

## Definitions

- OTP: An "OpenBMC TOF Proposal". Any request to the TOF should be framed as an
  OTP as documented by this process.

- Author: The individual(s) who write the initial draft of the OTP.

- Champion: A member of the TOF, confirmed by the current TOF chair, who will
  work with the Author while this process is being followed to ensure that
  appropriate updates to the OTP are being made. If the Author is a member of
  the TOF it is expected that they will also act as Champion to their own OTP.

## Process

An OTP works through three (3) states on the way to adoption: Draft, Discussion,
Final. During the Draft state, the responsibility lies on the Author to ensure
the OTP is able to move further in the process. As the OTP enters Discussion
state it is the responsibility of the current TOF chair to find a member of the
TOF to act as Champion to move the document along further in the process. The
Author and Champion will work together to make changes to the proposal as might
be necessary as it moves to Final state (unless it is rejected earlier in the
process).

### Draft State

Draft state is when the Author creates the initial proposal. Any member of the
OpenBMC community may create OTPs by doing the following:

1. Creating an issue in the [repository][3] with a title and no contents. a. The
   issue number assigned by Github will become the OTP number for this proposal.

2. Use the OTP template for the proposal type to write a draft proposal and send
   a copy to the [OpenBMC mailing list][4] with the title "OTP-N: Title" and the
   contents being the filled-in OTP template.

3. Add a comment to the issue with the link to the mailing list archive entry on
   [lore.kernel.org][5].

4. Create a Gerrit commit to the docs repository with the OTP template contents
   named `tof/proposals/otp-N.md` (along with any corresponding changes
   elsewhere in the docs repository that might be necessary to enact the OTP).

Once all of these are accomplished, the OTP enters Discussion state.

### Discussion State

Any member of the OpenBMC community may give feedback by:

1. Expressing a vote to the top post of the Github issue.
2. Providing grammatical suggestions to the Gerrit commit.
3. Responding on the mailing list with opinions on non-grammatical OTP content.
4. (Least desirable) Providing off-line feedback to a TOF member(s).

Community members should refrain from:

1. Voting on any comment in the Github issues beyond the top post.
2. Cluttering the mailing list discussion with grammatical suggestions.

An OTP will remain in Discussion state for the timeline expressed in the [TOF
contract][1] with respect to issue voting. TOF members will vote by reacting to
the OTP Github issue's comment containing the mailing list archive.

An OTP will exit Discussion state when one of the following occurs:

1. The OTP is rejected due to the voting rules contained in the TOF contract.
2. The OTP is accepted due to the voting rules contained in the TOF contract.
3. The OTP is discussed at a regular TOF meeting and is either rejected,
   partially accepted, or fully accepted.

If the OTP is fully or partially accepted, the chair will confirm a Champion to
move the issue through Final state.

### Final State

The purpose of the Final state is to get the OTP wording finalized into
languages that matches the intent and spirit of what the TOF accepted. If the
OTP was automatically accepted without discussion, this might be as simple as
fixing minor grammatical issue. If the OTP was partially accepted by the TOF
this may require rewriting large portions of the OTP to match the intention of
the TOF. The Champion is suppose to represent the TOF's viewpoint to ensure that
the Author and Champion can formulate the final wording for the OTP.

When the Champion believes the Gerrit commit adequately represents what the TOF
had approved, they should comment as such on the commit and inform the TOF
members via Discord. TOF members will then have 48 hours to review the final
wording and either approve or reject the commit in Gerrit as having met the
intention of the previous TOF vote. An approval (or rejection) does not mean
that the TOF member agrees (or disagrees) with the proposal, but that the TOF
member believes the final wording matches the previous TOF agreement(s). Thus, a
TOF member should reject a "Final OTP" if it does not align with the intention
of the previous TOF votes on the OTP, even if they personally agree with the
proposed "Final" wording.

An OTP is complete when 48 hours have elapsed and no TOF member has expressed
disagreement with the "Final OTP" wording having met the intentions of the TOF,
at which time it is merged into the repository.

---

[1]: https://github.com/openbmc/docs/blob/master/tof/contract.md
[2]: https://github.com/openbmc/technical-oversight-forum/issues/4
[3]: https://github.com/openbmc/technical-oversight-forum/issues
[4]: https://lists.ozlabs.org/listinfo/openbmc
[5]: https://lore.kernel.org/openbmc/
