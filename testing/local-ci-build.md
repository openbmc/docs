# Local CI Build

These instructions pertain to running the upstream OpenBMC CI locally.

## Install Docker

Please install and configure Docker. The installation of Docker CE (Community
Edition) varies by platform and may differ in your organization, but the
[Docker Docs](https://docs.docker.com/install/) are a good place to start
looking.

Check that the installation was successful by running
`sudo docker run hello-world`.

Each repository is built locally within the CI using the bootstrap.sh and
automake toolchain.

## Download the CI Image

Start by making a place for your CI repositories to live, and clone the CI
scripts first:

```shell
mkdir ci_test_area
cd ci_test_area
git clone https://github.com/openbmc/openbmc-build-scripts.git
```

## Add a Read-Only Repo

If you just want to validate a project as it is upstream, without any of your
own changes, you can clone it directly into the Docker directory. This example
clones and builds phosphor-hwmon directly. The upstream CI will try building
everything and this includes running `make check` if available. It will also run
`format-code.sh` and check if the code is formatted properly if there is a
`.clang-format` file present in the target repository, or if there is a script
in the repo named `format-code.sh`.

```shell
cd /path/to/ci_test_area
git clone https://github.com/openbmc/phosphor-hwmon
WORKSPACE=$(pwd) UNIT_TEST_PKG=phosphor-hwmon \
./openbmc-build-scripts/run-unit-test-docker.sh
```

NOTE: When running 'run-unit-test-docker.sh' make sure you do not have any
uncommitted changes. The script runs git diff after 'format-code.sh' and
therefore any uncommitted changes show up as output and it then fails assuming
the code was improperly formatted.

NOTE: This technique is okay if you have only a quick change on a project you
don't work on very often, but is more difficult to use if you would rather use
an active development branch. If you plan to develop regularly on a repo, use
the next technique instead.

## Run CI on local changed Repo

If you have local changes in the repo, and you do not want to commit yet, it's
possible to run local CI as well by setting `NO_FORMAT_CODE=1` before running
the script.

E.g.

```shell
WORKSPACE=$(pwd) UNIT_TEST_PKG=phosphor-hwmon NO_FORMAT_CODE=1 \
./openbmc-build-scripts/run-unit-test-docker.sh
```

`NO_FORMAT_CODE=1` tells the script to skip the `format-code.sh` so that it will
not format the code and thus your repo could contain un-committed changes.

## Run CI for testing purposes only

To only invoke the test suite and not get caught up the various analysis passes
that might be run otherwise, set `TEST_ONLY=1`.

E.g.

```shell
WORKSPACE=$(pwd) UNIT_TEST_PKG=phosphor-hwmon TEST_ONLY=1 \
./openbmc-build-scripts/run-unit-test-docker.sh
```

## Reference an Existing Repo with `git worktree`

If you're actively developing on a local copy of a repo already, or you want to
start doing so, you should use `git worktree` instead so that the Docker
container can reference a specific branch of your existing local repo. Learn
more about worktree by running `git help worktree` if you're curious.

This example demonstrates how to make a worktree of `phosphor-host-ipmid` in
your Docker container.

```shell
cd /my/dir/for/phosphor-host-ipmid
git worktree add /path/to/ci_test_area/phosphor-host-ipmid
```

Now, if you `cd /path/to/ci_test_area`, you should see a directory
`phosphor-host-ipmid/`, and if you enter it and run `git status` you will see
that you're likely on a new branch named `phosphor-host-ipmid`. This is just for
convenience, since you can't check out a branch in your worktree that's already
checked out somewhere else; you can safely ignore or delete that branch later.

However, Git won't be able to figure out how to get to your main worktree
(`/my/dir/for/phosphor-host-ipmid`), so we'll need to mount it when we run. Open
up `/path/to/ci_test_area/openbmc-build-scripts/run-unit-test-docker.sh` and
find where we call `docker run`, way down at the bottom. Add an additional
argument, remembering to escape the newline ('\'):

```
PHOSPHOR_IPMI_HOST_PATH="/my/dir/for/phosphor-host-ipmid"

docker run --blah-blah-existing-flags \
  -v ${PHOSPHOR_IPMI_HOST_PATH}:${PHOSPHOR_IPMI_HOST_PATH} \
  -other \
  -args
```

Then commit this, so you can make sure not to lose it if you update the scripts
repo:

```shell
cd openbmc-build-scripts
git add run-unit-test-docker.sh
git commit -m "mount phosphor-host-ipmid"
```

The easiest workflow in this situation now looks something like this:

```shell
# Hack in the main worktree.
cd /my/dir/for/phosphor-host-ipmid
git checkout -b add-foo
# Make a change.
touch foo
# Make sure to commit it.
git add foo
git commit -sm "change foo"
# Update the Docker worktree to agree with it.
cd /path/to/ci_test_area/phosphor-host-ipmid
git checkout --detach add-foo
# Now run the Docker container normally
```

#### Interactive Docker Session

To use an interactive session, you can pass the flag `INTERACTIVE=true` to
`run-unit-test-docker.sh` which will drop you into a bash shell with a default
D-Bus instance. This is handy if you need to run gdb on a core dump or run a
daemon on your workstation.
