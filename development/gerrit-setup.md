# OpenBMC Gerrit Setup/Integration

**Document Purpose:** Walkthrough configuring your workstation and a Gerrit
                      account. This needed to participate in OpenBMC's
                      Gerrit-based code reviews.

**Prerequisites:** Current Linux, Mac, or Windows system. Git packages
                   installed. E-mail account (recommended to use the same e-mail
                   address for both Gerrit and GitHub)

## Initial Setup

##### Update Git Identity

* `git config --global --add user.name "Your name" (eg. John Smith)`
* `git config --global --add user.email "youremail@your-domain"
    (eg. jsmith@somedomain.com)`
* (Optional) `git config --global --add diff.tool "preferred diff tool"
    (eg. gvimdiff or meld)`


##### Setup SSH Keys
Create keys: ```ssh-keygen -t rsa -C "your_email@your-domain"```
* Recommended to use the defaults instead of picking your own directory/file
  names.

Add Keys to Github:
* <https://help.github.com/articles/adding-a-new-ssh-key-to-your-github-account/>

Add Keys to OpenBMC's Gerrit Server:
* Login to [Gerrit](https://gerrit.openbmc-project.xyz/) with your GitHub
  account.
* Go to
  <https://gerrit.openbmc-project.xyz/plugins/github-plugin/static/account.html>
* Your information should be auto-filled, so click "Next".
* If successful you should see a blank screen, feel free to exit out.

##### Add e-mail to Gerrit
* Login to [Gerrit](https://gerrit.openbmc-project.xyz/)
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
* **NOTE**: There is a bug in AFS that requires `AFSTokenPassing no` to be
            added to the SSH entry if using AFS.
* Your Gerrit ID can be found in Gerrit under Settings -> Profile -> Username
* Ensure proper permissions for for your .ssh directory: `chmod 600 ~/.ssh/*`

##### Confirm Setup Success
Test connectivity to Gerrit by attempting to clone a repo
* `git clone ssh://openbmc.gerrit/openbmc/docs`
* If successful you should see something like:
    `Checking out files: 100% (45/45), done.`

##### Add Hooks
Inside the repo you just cloned, enter the following commands:
```
gitdir=$(git rev-parse --git-dir)
scp -p -P 29418 openbmc.gerrit:hooks/commit-msg ${gitdir}/hooks
```
This will enhance the `git commit` command to add a `Change-Id` to your commit
message which Gerrit uses to track the review.

##### Conclusion
If you've completed all of the above steps successfully, that's it! You are now
set up to push your changes up to Gerrit for code review!
