
OpenBMC CI on Google Cloudbuild

Author:
  Simon Li (s696li@uwaterloo.ca)

Primary assignee:
  Simon Li

Created:
  02/10/2022

## Definitions

Google Cloud Platform (GCP) provides the following services used by the project:

- Google Cloudbuild (GCB)
  - CI/CD workflow automation product
  - Executes your builds from your repositories like Github or Bitbucket

- Google Cloud Registry (GCR)
  - Service which provides private storage for Docker images


## Problem Description

Currently, Jenkins is the only CI/CD system available for the OpenBMC
community. It is an old product we believe could be improved on. It's main
disadvantage is its lack of scaling capability as it is server-based.

We offer an alternative serverless solution: Google Cloudbuild. GCB is a
cloud solution that can help scale the OpenBMC CI system for building OpenBMC
images. We can decrease significantly decrease build times by setting up
powerful machine executors for GCB.

Overall we hope to provide a better user experience through a fast service,
as well as a better UI experience with a modern UI.


## Requirements

Functional requirements:

- On demand OpenBMC builds via GCB
  - Setup the repository and config files for GCB
  - User can create an OpenBMC image from a single click of a button on GCB

- Versioning Support
  - User can specify which branch to build from a parameter
  (release branches, etc.)

- Artifacts automatically published
  - Upon successful build of an OpenBMC image, artifacts (binary) should be
  easily accessible by the user.
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
- Fast build time ~15 mins
- Let the user decide if they want to use Sstate-cache or full build.
- Increase build performance on GCB by changing the machine type (CPU, RAM)
- Setup our own machine for builds, private pools on GCB could be
an option


## Proposed Design

We can divide the major milestones as follows:

1. Develop the build scripts in Python 3.8.
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

2. Integration testing between Python code and GCB manual trigger function.
  a. This should happen at the same time as step 4. Every pull request should
  include a small integration test. The easiest way to test this
  is to use GCB’s build trigger.

3. Branch support by substitution variables on cloudbuild.

4. Artifacts, published as a link only accessible by the user.


## Impacts
Introducing GCB as a CI solution to the OpenBMC community will provide a good
alternative to Jenkins. The open-source community will be able to freely use
this CI system and report any issues that it will have in the future to Google.
We can fix them internally and release better upgrades.

Through this iteration process, the ultimate goal is to provide an upgrade to
Jenkins for the benefit of both Google and the OpenBMC community.

## Alternatives Considered
- Amazon Web Services
  - AWS CodePipeline is the CI/CD system used by AWS

- Azure
  - Azure Pipelines is the CI/CD system used by Microsoft

## Testing
Perform unit tests using Pytest when building the image on GCB.
Build triggers will perform automatic integration testing with
the real GCB when we push new changes to our working repository.

# Examples

How we git clone using the yaml configuration file for GCB:

steps:
  - name: gcr.io/cloud-builders/git
    args:
      - clone
      - b (flag for specifying branch)
      - a_branch (specify your branch)
      - any OpenBMC Gerrit repo (specify the repo url)

