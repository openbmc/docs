# AI Coding Assistants

This document provides guidance for AI tools and developers using AI assistance
when contributing to the OpenBMC.

AI tools helping with OpenBMC development should follow the standard OpenBMC 
development process documented in CONTRIBUTING.md and other docs within this
repository.

## Licensing and Legal Requirements

All contributions must comply with the individual subprojects licensing
requirements, and must be compatible with the license of the repository in
which it is submitted.

## Signed-off-by and Developer Certificate of Origin

AI agents MUST NOT add Signed-off-by tags. Only humans can legally certify the
Developer Certificate of Origin (DCO). The human submitter is responsible for:

- Reviewing all AI-generated code
- Ensuring compliance with licensing requirements
- Adding their own Signed-off-by tag to certify the DCO
- Taking full responsibility for the contribution

## Attribution

When AI tools contribute to OpenBMC development, proper attribution helps track
the evolving role of AI in the development process. Contributions should include
an Assisted-by tag in the following format:

Assisted-by: AGENT_NAME:MODEL_VERSION [TOOL1] [TOOL2]

Where:

- `AGENT_NAME` is the name of the AI tool or framework
- `MODEL_VERSION` is the specific model version used
- `[TOOL1] [TOOL2]` are optional specialized analysis tools used (e.g.,
  coccinelle, sparse, smatch)

Basic development tools (git, gcc, editors) should not be listed.

Example:

Assisted-by: Claude:claude-4.7-opus coccinelle sparse
