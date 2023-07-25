# TOF Membership and Voting

## Technical Oversight Forum

### Members

The TOF body shall consist of between 5 and 9 members selected by the OpenBMC
Development Community by vote. The current number of members is **7** and this
shall only be changed, prior to a TOF election, by a unanimous decision at a
regular meeting of the current TOF body.

TOF members must themselves be members of the OpenBMC Development Community.
This is determined by voting eligibility; ie. an individual is only eligible to
be elected a member of the TOF if they are eligible to vote in their own
election.

To encourage a diverse viewpoint, no more than 2 TOF members may be employed by,
or working under a contract relationship for, the same entity unless (one of):

- The entity is the Linux Foundation.
- The TOF members have been unopposed in an election.

### Terms and Elections

Members are elected by Ranked Choice Voting of the OpenBMC Development Community
at twice yearly elections. Members of the TOF typically serve a 1 year term
before their seat is up for re-election; members have no term limits.

To facilitate continuity of the TOF body, these elections are held every 6
months in which half (+/- 1) of the seats are re-elected. Due to additions or
subtractions in seats and membership resignations, more than half (+1) of the
seats may need to be filled in a single election. Prior to the election the
current TOF may determine a certain number of seats will be 6 month terms, to
return the number of seats per election to a more equal number, and these seats
will be given to the individuals with the later results in the RCV.

Elections are to begin on March 1st and September 1st at 00:00 UTC and conclude
seven days later. Election schedule is as follows:

| Q1 Elections | Q3 Elections | Action on or by 00:00 UTC.                                                                                 |
| :----------: | :----------: | :--------------------------------------------------------------------------------------------------------- |
|   Jan 1st    |   July 1st   | Developer contributions close for OpenBMC Development Community membership eligibility (See “Metrics”).    |
|   Jan 15th   |  July 15th   | Current TOF must publish a list of eligible voting members.                                                |
|   Jan 30th   |  July 30th   | Nominations (self or peer) for TOF seats must be sent to the mailing list.                                 |
|   Jan 30th   |  July 30th   | Developers disputing membership eligibility must submit a petition request to the current TOF.             |
|   Feb 15th   |   Aug 15th   | Current TOF must publish a final list of eligible voting members and upcoming candidates for TOF seats.    |
|  March 1st   |   Sept 1st   | Election begins.                                                                                           |
|  March 7th   |   Sept 7th   | Election concludes.                                                                                        |
|  March 15th  |  Sept 15th   | Current TOF publishes election results and updates the TOF membership document with new members and terms. |
|   Apr 1st    |   Oct 1st    | TOF member terms conclude / begin.                                                                         |

This document shall be maintained with a list of current members and their term
end (sorted by term conclusion followed by alphabetically by preferred name):

| Name               | Term Conclusion |
| :----------------- | :-------------: |
| Brad Bishop        |   2023-10-01    |
| Ed Tanous          |   2023-10-01    |
| Zev Weiss          |   2023-10-01    |
| Andrew Jeffery     |   2024-04-01    |
| Jason Bills        |   2024-04-01    |
| Patrick Williams   |   2024-04-01    |
| William Kennington |   2024-04-01    |

## OpenBMC Development Community

Membership in the OpenBMC development community is determined by development
contributions to the project. By contributing to the project, developers gain a
voice in the technical direction of the project by shaping the membership in the
TOF.

Membership is determined using data from the previous 6 months of development
contributions. Using this data, the TOF will publish a list of Active Members of
the two tiers. An individual who was an Active Member of a tier in the preceding
6 months, but does not qualify in the most recent 6 months, will be listed as an
Member Emeritus for 6 months. Both Active and Emertius members are eligible for
elections.

There are two tiers of membership in the development community: normal and
highly-productive. It is the responsibility of the TOF to set metrics for
determining community and tier membership. The normal membership tier is
expected to maintain a low-bar to encourage a diverse and vibrant membership,
while the highly-productive membership tier is expected to represent between
12.5% (1/8) and 20% (1/5) of the community. (Whenever the highly-productive tier
has representation outside of this percentage, the TOF should adjust the
determining metrics for the next election / membership cycle.) In any election
cycle, normal developers are given a vote weight of 1 and highly-productive
developers are given a vote weight of 3.

Any individual who feels their contributions to the project were not recognized
by the existing metrics may petition the TOF for inclusion in either tier and
the TOF will make a determination. Examples of these types of contributions
might be: development in upstream Open Source communities not directly
controlled by OpenBMC, but for features leveraged by the OpenBMC codebase, and
significant support activities in areas not covered by existing metrics such as
Wikis and Discord.

Currently, work on the following projects requires an explicit petition for
recognition of ToF membership eligibility:

- Linux
- u-boot
- QEMU
- bitbake
- open-embedded
- Poky

### Metrics

Determination of membership in the community is made by a point system for
activities.

- Development of a non-trivial commit (10+ lines) - 3 points.
- Non-trivial code review (3 or more code suggestions) - 1 point.

The TOF may decide to exempt commits which are only machine configuration or
sub-sections of a repository only intended for use by one entity. For example
repositories named `company-ipmi-oem-provider`, subdirectories named
`oem/company`, Yocto recipes in `meta-company`, and entity-manager configuration
might all be exempted. Any such exemptions should be applied consistently to all
members.

Points required for Active membership (in the preceding 6 months):

- Normal - 15 points.
- Highly-Productive - 80 points.
