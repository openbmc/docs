Host Clear GARD
================

The host maintains a record of bad or non-working components on the GARD partition. This record is referenced by the host on subsequent boots to determine which parts should be ignored.

The BMC implements a function that simply clears this partition. This function can be called as follows:

  * Method 1: From the BMC command line:

      ```
      busctl call org.open_power.Software.Host.Updater \
        /org/open_power/control/gard \
        xyz.openbmc_project.Common.FactoryReset Reset
      ```

  * Method 2: Using the REST API:

      ```
      curl -b cjar -k -H 'Content-Type: application/json' -X POST -d '{"data":[]}' https://$BMC_IP/org/open_power/control/gard/action/Reset
      ```

### Implementation

https://github.com/openbmc/openpower-pnor-code-mgmt
