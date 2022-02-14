
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
community. Although Jenkins provides the solution that we need to build
OpenBMC images, it has some problems that can be improved. Most notably, we aim
to improve build success rates and faster build times by introducing an 
alternative CI system.

Here are some problems that Jenkins have:

1. Jenkins does not natively support Docker, hence decreasing build stability.
Builds can fail because of the different environment between the Jenkins server
and the OpenBMC Gerrit repositories

2. Jenkins is server-based. Hence the Jenkins based CI system requires
maintenance and configuration by the OpenBMC community. Also, scaling is
difficult with a server-based system, hence build times can be slow

3. Jenkins is old and does not have an elegant UI

We are providing an alternative: Google Cloudbuild. Hence, those that do not 
want to use Jenkins can use GCB as a CI system to build OpenBMC
images instead.

Ultimately, the introduction of GCB aims to address the issues that Jenkins
have.

## Background

# What advantages does GCB offer?

GCB is a cloud-based CI/CD system. It utilizes Docker, hence the build
environments are consistent, thus increasing build success rates. 

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
this CI system and report any issues that it will have in the future. Hence,
internally we will be able to quickly fix these issues and provide even better
features and stability for GCB as a CI system for OpenBMC. Thus overall, Google
will also have a better CI system internally through the iteration process that 
the open-source community is providing.

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

