# OpenBMC Gerrit Setup/Integration

**Document Purpose:** Walkthrough setting up your development workflow to utilize Gerrit

**Prerequisites:** Current Linux, Mac, or Windows system. Git packages installed.

## Initial Setup

##### Update Git Identity
```
git config --global --add user.name "Your name" (eg. John Smith)
git config --global --add user.email "youremail@some-domain" (eg. jsmith@somedomain.com)
git config --global --add diff.tool "preferred diff tool" (eg. gvimdiff or meld or tkdiff)
```

##### Setup SSH Keys
Create key: ```ssh-keygen -t rsa -C "your_email@some-domain"```
* Recommended to use the defaults instead of picking your own directory/file names.

Add Keys to Gerrit:
* Login to Gerrit (https://gerrit.openbmc-project.xyz/) with your GitHub account.
* Go to Settings -> SSH Public Keys -> Paste the contents of your public key file into the box -> Add

##### Add e-mail to Gerrit
* Login to Gerrit (https://gerrit.openbmc-project.xyz/)
* Enter e-mail in Settings -> Contact Information -> Register New E-Mail
* Check e-mail for confirmation and click the link to confirm

##### Add SSH config entry
Add the following to `~/.ssh/config`:
```
Host openbmc.gerrit
    Hostname gerrit.openbmc-project.xyz
    Port 29418
    User <YOUR-GERRIT-ID>
```
* **NOTE**: There is a bug in AFS that requires `AFSTokenPassing no` to be added to the SSH entry if using AFS.
* Your Gerrit ID can be found in Gerrit under Settings -> Profile -> Username
* Ensure proper permissions for for your .ssh directory: ```chmod 600 ~/.ssh/*```

##### Confirm Setup Success
Test connectivity to Gerrit by attempting to clone a repo
* `git clone ssh://openbmc.gerrit/openbmc/docs`
* If successful you should see something like: `Checking out files: 100% (45/45), done.`

##### Add Hooks
Inside the repo you just cloned, enter the following commands:
```
gitdir=$(git rev-parse --git-dir)
scp -p -P 29418 openbmc.gerrit:hooks/commit-msg ${gitdir}/hooks
```
##### Conclusion
If you've completed all of the above steps successfully, that's it! You are now set up to push your changes up to
Gerrit for code review!