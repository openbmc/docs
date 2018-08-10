# Subtree Architecture

All of these directories are subtrees and may not be contributed to by pushing
directly to the openbmc/openbmc top level repository

```
poky/

meta-openembedded/
meta-security/
meta-arm/
meta-openpower/
meta-x86/
meta-phosphor/

meta-aspeed/
meta-nuvoton/
meta-raspberrypi/

meta-intel/
meta-mellanox/
meta-portwell/
meta-quanta/
meta-ibm/
meta-ingrasys/
meta-inventec/
meta-rackspace/
meta-qualcomm/
```

Instead, please follow this workflow:
```
$ git clone git@github.com:openbmc/openbmc.git
# Make changes
$ git commit
$ git checkout -b featureBranch ssh://openbmc.gerrit/openbmc/<repo_name>/master
$ git cherry-pick --strategy=subtree <SHA> # My commit from master
$ git push ssh://openbmc.gerrit/openbmc/<repo_name> HEAD:refs/for/master

$ git checkout master # To continue work on something unrelated
```

If you have lost the commit from the parent repo and need to do work on your
subtree code review:
```
$ git checkout featureBranch
$ git log -n1 --oneline # capture the SHA
$ git checkout master
$ git cherry-pick --strategy=subtree <SHA you just captured>
```

If for some reason you have lost the featureBranch:
```
$ git checkout -b featureBranch
$ git fetch ssh://openbmc.gerrit/openbmc/<repo_name> refs/changes/XX/XXXX/X && git checkout FETCH_HEAD
$ git log -n1 --oneline # Capture the SHA
$ git checkout master
$ git cherry-pick --strategy=subtree <SHA you just captured>
```

Automation on the back end will then:
```
$ git fetch internal #In openbmc/openbmc
$ git cherry-pick --strategy=subtree <SHA of latest subtree push>
$ git push origin master
```
