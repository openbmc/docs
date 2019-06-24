# Local CI Build

These instructions pertain to running the upstream OpenBMC CI locally.

Please install and configure Docker.

Each repository is build locally within the CI using the bootstrap.sh and
automake toolchain.

The following example will checkout and build phosphor-hwmon. The upstream CI
will try building everything and this includes running `make check` if
available. It will also run `format-code.sh` and check if the code is formatted
properly if there is a `.clang-format` file present in the target repository.

```
mkdir ci_test_area
cd ci_test_area
git clone https://github.com/openbmc/openbmc-build-scripts.git
git clone https://github.com/openbmc/phosphor-hwmon
sudo WORKSPACE=$(pwd) UNIT_TEST_PKG=phosphor-hwmon \
./openbmc-build-scripts/run-unit-test-docker.sh
```

NOTE: When running 'run-unit-test-docker.sh' make sure you do not have any
uncommitted changes. The script runs git diff after 'format-code.sh' and
therefore any uncommitted changes show up as output and it then fails assuming
the code was improperly formatted.

#### Interactive Docker Session

To use an interactive session, you can change the `run-unit-test-docker.sh`.

Replace the following (or similar:)

```
docker run --cap-add=sys_admin --rm=true \
  --privileged=true \
  -w "${WORKSPACE}" -v "${WORKSPACE}":"${WORKSPACE}" \
  -e "MAKEFLAGS=${MAKEFLAGS}" \
  -t ${DOCKER_IMG_NAME} \
  ${WORKSPACE}/${DBUS_UNIT_TEST_PY} -u ${UNIT_TEST} \ -f ${DBUS_SYS_CONFIG_FILE}
```

with

```
docker run --cap-add=sys_admin --rm=true \
  --privileged=true \
  -w "${WORKSPACE}" -v "${WORKSPACE}":"${WORKSPACE}" \
  -e "MAKEFLAGS=${MAKEFLAGS}" \
  -it ${DOCKER_IMG_NAME} /bin/bash
```

When you rerun `run-unit-test-docker.sh` you will be dropped into an interactive
session. This is handy if you need to run gdb on a core dump.
