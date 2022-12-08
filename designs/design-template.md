---

# Document Guidelines - _Delete this section_

## Document Content

- Not all new features need a design document. If a feature can be contributed
  in a single reasonably small patchset that has little impact to any other
  areas, it doesn't need a design discussion and documentation.

- The focus of the document is to define the problem we need to solve and
  analyse the trade-offs of different solutions. You should concentrate on
  interactions between components, though analysing the trade-offs often
  requires you to explore how each might be implemented.

- Write this document as an [argumentative essay][argumentative-essay]. Good
  design proposals compel the reviewers agree with the proposal's conclusions.

[argumentative-essay]: https://www.grammarly.com/blog/argumentative-essay/

- This is not intended to be extensive documentation for a new feature.

- You should get your design reviewed and merged before writing your code.
  However you are free to prototype the implementation, but remember that you
  may learn of new requirements during the design review process that could
  result in a very different solution.

## Document Formatting

- Your spec should be in Markdown format, like this template.

- Please wrap text at 79 columns.

- Please do not delete any of the sections in this template. If you have nothing
  to say for a whole section, just write: None

- To view your .md file, see: https://stackedit.io/

- If you would like to provide a diagram with your spec, ASCII diagrams are
  required. http://asciiflow.com/ is a very nice tool to assist with making
  ASCII diagrams. Plain text will allow the review to proceed without having to
  look at additional files which can not be viewed in Gerrit. It will also allow
  inline feedback on the diagram itself.

- Once ready for review, submit to gerrit and set the topic of the review to
  "design"

---

# Example design - this is the design title

Author: < Name and Discord nickname >

Other contributors: < Name and/or Discord nickname or `None` >

Created: < Date initially created in Month D, Yr format (e.g. June 17, 2020 or
Dec 19, 2019)>

## Problem Description

(1 paragraph) What are we doing and why? What problem are you trying to solve?
What are the goals and NON-goals? Please make the objective understandable for
someone unfamiliar with this project by including the necessary context, but
keep it short. Elaborate on the details below in the Background and Requirements
sections.

## Background and References

(1-2 paragraphs) What background context is necessary? You should mention
related work inside and outside of OpenBMC. What other Open Source projects are
trying to solve similar problems? Try to use links or references to external
sources (other docs or Wikipedia), rather than writing your own explanations.
Please include document titles so they can be found when links go bad. Include a
glossary if necessary. Note: this is background; do not write about your design,
specific requirements details, or ideas to solve problems here.

## Requirements

(2-5 paragraphs) What are the constraints for the problem you are trying to
solve? Who are the users of this solution? What is required to be produced? What
is the scope of this effort? Your job here is to quickly educate others about
the details you know about the problem space, so they can help review your
implementation. Roughly estimate relevant details. How big is the data? What are
the transaction rates? Bandwidth?

## Proposed Design

(2-5 paragraphs) A short and sweet overview of your implementation ideas. If you
have alternative solutions to a problem, list them concisely in a bullet list.
This should not contain every detail of your implementation, and do not include
code. Use a diagram when necessary. Cover major structural elements in a very
succinct manner. Which technologies will you use? What new components will you
write? What technologies will you use to write them?

## Alternatives Considered

(2 paragraphs) Include alternate design ideas here which you are leaning away
from. Elaborate on why a design was considered and why the idea was rejected.
Show that you did an extensive survey about the state of the art. Compares your
proposal's features & limitations to existing or similar solutions.

## Impacts

API impact? Security impact? Documentation impact? Performance impact? Developer
impact? Upgradability impact?

### Organizational

- Does this repository require a new repository? (Yes, No)
- Who will be the initial maintainer(s) of this repository?
- Which repositories are expected to be modified to execute this design?
- Make a list, and add listed repository maintainers to the gerrit review.

## Testing

How will this be tested? How will this feature impact CI testing?
