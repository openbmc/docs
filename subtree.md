# Subtree Architecture

All of these directories are subtrees and may not be contributed to by pushing
directly to the openbmc/openbmc top level repository

Subtrees are a way to nest repositories inside another as a sub-directory. This
allows us to contain all of the Yocto meta-data in individual repositories
(useful for developers who don't want the phosphor-distro), as well as one top
level directory - making it easy to get started.

To find a list of all current subtrees in the openbmc project, navigate to:
https://github.com/openbmc?utf8=✓&q=meta

Instead, please follow this workflow:
```
$ git clone git@github.com:openbmc/openbmc.git
# Make changes
$ bitbake obmc-phosphor-image # Test out your changes
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
$ git fetch ssh://openbmc.gerrit/openbmc/<repo_name> refs/changes/XX/XXXX/X
$ git checkout FETCH_HEAD
$ git log -n1 --oneline # Capture the SHA
$ git checkout master
$ git cherry-pick --strategy=subtree <SHA you just captured>
```

### Automation to test the ref
Where $1 is the repo name, and $2 is a ref to a pointer
```
git clone git@github.com:openbmc/openbmc.git
cd openbmc
git remote add subtree-remote git@github.com:openbmc/$1
git fetch ssh://openbmc.gerrit/openbmc/$1 refs/changes/$2
git cherry-pick --strategy=subtree FETCH_HEAD
```

### Automation to merge the subtree into openbmc/openbmc
Once +2 is given, this script will run where $1 is the repo name, and $2 is a
full path to subtree from top level
```
git clone git@github.com:openbmc/openbmc.git
cd openbmc
git remote add subtree-remote git@github.com:openbmc/$1

git subtree pull --prefix=$2 subtree-remote master
git push
```

To keep yourself up to date with the latest as changes are submitted, you can
simply rebase again the openbmc master, and you will automatically get the
changes made in the sub-directories.
```
git checkout master
git pull --rebase origin master
```
