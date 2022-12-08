# Run OpenBMC Test Automation Using Docker

Running OpenBMC automation using Docker involves creating a Docker image and
then running automation tests inside the Docker container.

## Build Docker Image

`Note: Prerequisite is to have Docker installed.`

1. Create a workspace directory.

   `mkdir ${HOME}/OpenBMC_Automation`

2. Change directory to workspace created.

   `cd ${HOME}/OpenBMC_Automation`

3. Clone openbmc-build-scripts repository.

   `git clone https://github.com/openbmc/openbmc-build-scripts`

4. Change directory to openbmc-build-scripts.

   `cd openbmc-build-scripts`

5. Build the Docker image required to execute the robot tests (it may take close
   to 15 mins for the first time). The default Docker image name is
   "openbmc/ubuntu-robot-qemu". You can check images using "docker images"
   command.

   `./scripts/build-qemu-robot-docker.sh`

   ###### _Note: When your Docker is behind a proxy, add the following parameters to the build command (use proper IP and PORT values.)_

   ```
   --build-arg http_proxy=<IP>:<PORT> --build-arg https_proxy=<IP>:<PORT>
   ```

## Code update process using robot test code

1. Change directory to HOME workspace.

   `cd ${HOME}/OpenBMC_Automation`

2. Clone openbmc-test-automation repository.

   `git clone https://github.com/openbmc/openbmc-test-automation`

3. Execute docker run to initiate BMC code update.

   ###### _Note: Download BMC fw image file (_.all.tar) before executing this. BMC_IMG_PATH below points to this downloaded file.

   ```
   docker run --user root \
              --env HOME=${HOME} \
              --workdir ${HOME} \
              --volume ${HOME}/OpenBMC_Automation:${HOME} \
              --tty openbmc/ubuntu-robot-qemu python -m robot \
                  -v OPENBMC_HOST:<BMC IP> \
                  -v FILE_PATH:<BMC_IMG_PATH> \
                  -i Initiate_Code_Update_BMC \
                  ${HOME}/openbmc-test-automation/extended/code_update/update_bmc.robot
   ```

   Example to run BMC code update using witherspoon-20170614071422.all.tar image
   file from HOME directory of the system where docker run command is executed:

   ```
   docker run --user root \
              --env HOME=${HOME} \
              --workdir ${HOME} \
              --volume ${HOME}/OpenBMC_Automation:${HOME} \
              --tty openbmc/ubuntu-robot-qemu python -m robot \
                  -v OPENBMC_HOST:1.11.222.333 \
                  -v FILE_PATH:/home/witherspoon-20170614071422.all.tar \
                  -i Initiate_Code_Update_BMC \
                  ${HOME}/openbmc-test-automation/extended/code_update/update_bmc.robot
   ```

4. On code update completion, logs generated from robot framework execution will
   be available in the following location:

   ```
   ${HOME}/OpenBMC_Automation/log.html
   ${HOME}/OpenBMC_Automation/report.html
   ${HOME}/OpenBMC_Automation/output.xml
   ```

## Executing Automation Test

1. Execute docker run to execute OpenBMC automation test cases.

   ###### _Note: This runs a Docker container using openbmc/ubuntu-robot-qemu image._

   ###### _Robot test code is extracted and ran on this container using run-robot.sh script._

   ```
   docker run --user root \
              --env HOME=${HOME} \
              --env IP_ADDR=<BMC IP> \
              --env SSH_PORT=22 \
              --env HTTPS_PORT=443 \
              --env ROBOT_TEST_CMD="tox -e <System Type> -- <Robot Cmd>" \
              --workdir ${HOME} \
              --volume ${WORKSPACE}:${HOME} \
              --tty openbmc/ubuntu-robot-qemu \
                  ${HOME}/openbmc-build-scripts/scripts/run-robot.sh
   ```

   Example to run entire test suite:

   ```
   docker run --user root \
              --env HOME=${HOME} \
              --env IP_ADDR=1.11.222.333 \
              --env SSH_PORT=22 \
              --env HTTPS_PORT=443 \
              --env ROBOT_TEST_CMD="tox -e witherspoon -- tests" \
              --workdir ${HOME} \
              --volume ${HOME}/OpenBMC_Automation:${HOME} \
              --tty openbmc/ubuntu-robot-qemu \
                  ${HOME}/openbmc-build-scripts/scripts/run-robot.sh
   ```

2. After the execution, test results will be available in below files.

   ```
   ${HOME}/OpenBMC_Automation/log.html
   ${HOME}/OpenBMC_Automation/report.html
   ${HOME}/OpenBMC_Automation/output.xml`
   ```
