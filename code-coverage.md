
# Machine Per-Build Code Coverage (Design Document Draft)

The following report details the code coverage system implemented for OpenBMC builds.

OpenBMC currently relies on a manual system for detailing code coverage of any repositories used for image building. With each machine, there is a repo combination unique to it. This means that currently, if one wants to view the status and changes of those repositories, they would have to manually clone it and run unit tests on the correct version. This system automates the process and allows the aggregation of code coverage for the selected build.

This process is done through a modified version of OBMC’s `get_unit_test_report.py` script, adding per-build capabilities and improving lcov data generation.

## Running Files

Start by creating a repository for CI and CI scripts to be stored.

```
mkdir ci_test_area
cd ci_test_area
git clone [https://github.com/openbmc/openbmc-build-scripts.git](https://github.com/openbmc/openbmc-build-scripts.git)
```

To run with specified , image config files have to be modified to inherit the new file.
```
${BUILDDIR}/conf/local.conf
```
 
Add
```
INHERIT += “buildhistory”
BUILDHISTORY_COMMIT = “1”
```

Run the following command to run without a specified url_file or buildhistory path.

```
python3 openbmc-build-scripts/scripts/get_unit_test_report <target_dir>
```

Running with a buildhistory path

```
python3 openbmc-build-scripts/scripts/get_unit_test_report <target_dir> -build_history <build_history_path>
```

## Source File and Revision Generation

The file `code_coverage.sh` generates a repositories.txt file, which stores the src_url and src_rev of repositories used during image creation. The format is stored in the following format:

```
<src_url> <src_rev>
```

The resulting file is intended to be used for any automated test suite runner, such as OBMC’s `get_unit_test_report.py`’s url_file parameter.

## Automated Test Cases

Test case is automated through the use of `get_unit_test_report.py`, which takes care of cloning the specific repositories and running unit tests on it. This file was pre-existing in the codebase, but was slightly modified to work with the new files, as well as implementing optional source revision.

Optional parameters were modified to allow for more variables. The extra parameters added were

```
-build_history - Path to buildhistory that was created during image generation.
```

## LCOV Aggregation

LCOV data is freely available after running the `get_unit_test_report.py` script. `code_coverage.py` works by taking all the available data and aggregating it into an easy to access file. This simplifies the process of lcov analysis, as the necessary data/files can be found from one simplified file.

Code_coverage.py works by taking in the path of directories leading to `<repo>/build/<coveragereport_path>/index.html.` It sifts through each file and reads the resulting lcov data from the index.html file.