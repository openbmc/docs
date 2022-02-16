
OpenBMC CI on Google Cloudbuild

Author:
  Simon Li (s696li@uwaterloo.ca)

Primary assignee:
  Simon Li

Created:
  02/10/2022

## Problem Description

Currently, vendors must manually set up the metadata themselves.
The main problem is the lack of control over what the vendors 
actually installed when building the release.

In more details:
1. Since the process is manual, vendors could potentially install their ow
dependencies during the manual setup.

2. The vendors can include un-submitted code not verified in 
their final release, there would have no way of knowing if some code escaped
the review process.

3. Since some release builds might not follow the build's specifications,
issues can arise when testing the images.

4. Without knowledge of what was installed by the vendors, it would be very
difficult to properly debug the build if any issues arise.

# Definitions

Google Cloud Platform (GCP) provides the following services used by the project:

- Google Cloudbuild (GCB)
  - CI/CD workflow automation product
  - Executes your builds from your repositories like Github or Bitbucket

- Google Cloud Registry (GCR)
  - Service which provides private storage for Docker images

- Git on Borg (GOB)
  - Git version control system running in Google data centers


## Requirements


Functional requirements:

- On demand OpenBMC builds via GCB
  - Grant GCB access to vendors.
  - Grant GoB repo permission to GCB service account.
  - Setup the repository and config files for the vendor
  - Vendor creates an OpenBMC image with vendor private repositories 
  (not all upstream) from a single click of a button on GCB

- Versioning Support
  - Vendors can specify which branch to build from a parameter 
  (release branches, etc.)

- Artifacts automatically published
  - Upon successful build of an OpenBMC image, artifacts (binary) should be
  easily accessible by the vendor
  - Use the version as the name, check if file already exists or not.
  Want the name to be unique  when we save it.

- GCR image required
  - Install required Python dependencies on the new GCR image through Dockerfile

- Configurable environment file
  - Maintainers of the codebase should be able to change environment values such
  that no coding changes are required.

- Cloudbuild configuration file (Yaml file) specifics
  - git clone portion can only be done through the Yaml configuration file.
  Hence, we will need to generate some of the configuation file through a
  script (the git clone steps) (see end of proposal for example)

Non-functional requirements:
- Fast build time 15 mins, let the user decide if they want to use
Sstate-cache or full build.
- For better build performance on GCB, change machine type.
- To obtain a more powerful machine, request for a better CPU or more ,
then it will be faster to build. 
- Might need to setup our own machine for builds, private pools on GCB could be
an option

Security requirements:
- Artifacts should be stored in a secure location only accessible by the vendor


## Proposed Design

First, we can divide the major milestones as follows:

1. Understand how the manual process works, and why certain steps are necessary.

2. Identify any obstacles that could hinder the development of the project,
mainly credential issues.

3. Replicate the same manual steps with the vendors-builders project instead of
gbmc-builders project.

4. Develop the build scripts in Python 3.8.
  a. Project config file should contain vendor specific information
  (github repo links, vendor name, etc.)
  b. Code should integrate well with the config file. The only change required
  for individual vendors should be to change the values of the fields in
  the config file.
  c. Write unit tests to define the expected behaviors before full
  implementation.
  d. Write clean and re-usable code according to the Google Python Style Guide
  e. Use the sh module to replicate the structure of bash codes that 
  we’ve been writing.
  f. Calculate build times, mean time, etc.
  g. Note down every dependencies used during local build, then build a
  new image and push to gcr

5. Integration testing between Python code and GCB manual trigger function.
  a. This should happen at the same time as step 4. Every pull request should
  include a small integration test. The easiest way to test this
  is to use GCB’s build trigger.

6. Branch support by substitution variables on cloudbuild.

7. Artifacts, published as a link only accessible by vendors.

8. Setup a GCB trigger for the vendors, allowing them to
immediately use our project.

## Impacts
The implementation of this proposal will have the following impact:
- Greater control over the source code when building the image
- Faster build times
- Easier to debug issues in an image
## Alternatives Considered
None

## Testing
Perform unit tests using Pytest when building the image on GCB.
Perform integration testing on local GCB for faster iteration.
Build triggers will perform automatic integration testing with
the real GCB when we push new changes to our working repository.

# Examples

How we git clone using the yaml configuration file for GCB:

steps:
  - name: gcr.io/cloud-builders/git
    args:
      - clone
      - -b
      - google-dev
      - https://gbmc-internal.googlesource.com/gbmc

