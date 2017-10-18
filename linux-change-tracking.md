# Linux Change Tracking

## Commit Messages

In addition to what is specified in the
[kernel documentation](https://github.com/torvalds/linux/blob/master/Documentation/process/submitting-patches.rst),
we also have a set of commit footers for changes that are applied locally.

### Internal Commit Footers

*   `Bug-Id` - a bug tracking identifier used to provide context for a change;
    this will likely be a Github issue id.
*   `Effort` - used to specify who the work is for. For code that is to support
    a specific machine, it should be the machine or company name. This makes the
    company/organization responsible for supplying people to address problems
    supporting these commits.
*   `Origin` - Specifies the original version of an internal commit; used when
    an internal commit is rebased or cherry-picked. See
    [Submitting Code](#submitting-code) for more information.
*   `Dropped` - Specifies that a commit is to be dropped in any rebase. See
    [Submitting Code](#submitting-code) for more information.
*   `Upstream` - Specifies that a commit is to be dropped if the commit can be
    found in the branch being rebased to. See
    [Submitting Code](#submitting-code) for more information.
*   `Git-repo` - used in conjunction with `Upstream`, used to specify a
    non-mainline upstream repository, this should probably always be an upstream
    maintainer's for-next repository. See [Submitting Code](#submitting-code)
    for more information.
*   `In-review` - used to specify where the most recent version of a patch is
    being reviewed upstream. See [Submitting Code](#submitting-code) for more
    information.
*   `Maintainer` - used in exceptionally rare circumstances to identify who is
    responsible for maintaining an internal patch. See
    [Submitting Code](#submitting-code) for more information.

## Submitting Code

All code should go upstream; there are some reasons why code may not be able to
go upstream immediately, but in these cases, there must be a plan to get code
upstream as quickly as possible. Under most circumstances, a patch should
immediately get sent to the appropriate upstream mailing list. In most cases, if
you send a kernel patch to OpenBMC, you will be told to forward it to the
appropriate upstream mailing list. If you are not sure what list a patch should
be mailed to, it should go to OpenBMC and then we can sort it out there.

If you are unfamiliar with sending patches to mailing lists, definitely send it
to OpenBMC first. The OpenBMC people are super nice and can give you pointers on
sending patches upstream. Some time ago I put a document (I can share it if
anyone is interested) together on what I thought was non-obvious about doing
this; nevertheless, the kernel has an
[official document](https://github.com/torvalds/linux/blob/master/Documentation/process/submitting-patches.rst)
on this as well.

### Normal Submission

If the code change is *normal*, then the patch should be low urgency (but not
necessarily low priority), meaning that we can afford to wait a bit for the
patch; it should be uncontroversial, rather, it is unlikely that anyone would
outright reject the patch going upstream in some form or another. If these
things are true, there is no reason to make an internal submission, just send
the patch directly to the appropriate upstream mailing lists and the discussion
will take place there. The patch will eventually be pulled into the OpenBMC
kernel when we rebase at some point in the future.

### High Urgency Submission

In some cases, we will make changes that are uncontroversial and have no reason
to not go upstream, but need to get into the OpenBMC kernel before it can make
it in via the normal case.

#### Accepted Upstream Submission

Ideally, code should still get *accepted* upstream before making an internal
submission, this means that the appropriate maintainer sent you an email that
says "applied to for-next"; if the patch can wait long enough for this to
happen, all you have to do is wait for this email and then cherry-pick the patch
out of the maintainer's repository and then add a footer to the commit message:

```
Git-repo: <URL to repository>
Upstream-<kernel version>-SHA1: <commit hash>
```

Where `URL to repository` is the URL to the maintainer's git repository.
`kernel version` is the version of the kernel the patch is in; if it is
for-next, then put `for-next-<expected version>`, where `expected version` is
the version you expect the commit to be released in. `commit hash` is the commit
hash of the upstream commit. Example:

```
i2c: aspeed: added driver for Aspeed I2C

Added initial master support for Aspeed I2C controller. Supports fourteen
busses present in AST24XX and AST25XX BMC SoCs by Aspeed.

Effort: google
Bug-Id: 30640883
Signed-off-by: Brendan Higgins <brendanhiggins@google.com>
Upstream-4.12.0-SHA1: f327c686d3ba44eda79a2d9e02a6a242e0b75787
```

#### Unaccepted Upstream Submission

Sometimes we need a change before we could reasonably expect a maintainer to
sign off on it; these changes should be handled with care as this categorization
inherently means that the change is incomplete and volatile. These changes
should be in a state such that they are ready for upstream submission, so they
should be sent both upstream and to internal review. Also, the internal
submission should have a special footer designating the URL of the last patch
version sent upstream for review:

```
In-review: <URL to patch>
```

Example:

```
i2c: aspeed: add proper support for 24xx clock params

24xx BMCs have larger clock divider granularity which can cause problems
when trying to set them as 25xx clock dividers; this adds clock setting
code specific to 24xx.

This also fixes a potential issue where clock dividers were rounded down
instead of up.

Effort: google
Bug-Id: 63352197
In-review: https://www.spinics.net/lists/kernel/msg2555325.html
```

Once the patch is accepted upstream, assuming it had already been accepted
internally, the internal version must be marked to be dropped, because the
commit cannot be edited we instead create and empty commit with a list of
patches to be dropped with an explanation:

```
Dropped-<version>-SHA1: <commit hash>
```

An empty commit can be created with the following command:

```
git commit --allow-empty
```

However, if the commit has diverged in a serious way, fixes a bug, or something
else necessitates the upstream version being cherry-picked in, the above dropped
message should be included in a revert commit that reverts the internal version.
The revert commit should also be marked as dropped as well, of course. The
upstreamed version can then be cherry-picked in the
[method described above](#accepted-upstream-submission). Example:

```
Revert "i2c: aspeed: added driver for Aspeed I2C"

This reverts commit 437bb89242293187a824471634ec232461dc5bc7. The upstream
version has substantially diverged from what was committed here.

Effort: google
Bug-Id: 30640883
Dropped-4.7.10-SHA1: 437bb89242293187a824471634ec232461dc5bc7
Dropped-4.7.10-SHA1: this
```

### Internal Only Submission

In some cases, it may be unfeasible or impossible to make an upstream
submission; these changes should have this reason explicitly justified. Also,
these changes must have an explicit internal maintainer. This maintainer must be
active and available to respond to issues regarding his changes; the maintainer
will be responsible for any issues that are caused by attempting to rebase his
changes and will likely be asked to rebase his changes if any issues occur.
Designating an internal maintainer can be accomplished in a couple of different
ways.

For changes that add new drivers, frameworks, in other words are adding new
files that represent a fairly self contained feature or are modifications to
files that were introduced in this manner, the internal maintainer will be
specified by the @docs://MAINTAINERS file.

It is possible that a change could entirely affect an existing file. First off,
such changes should be *extremely rare*, as they are extremely difficult to
maintain and probably mean that some form of the change should go upstream.
Regardless, if this happens, you can put a special footer on the change:

```
Maintainer: <my-name>@<my-company>.com
```

### Submissions to Multiple Internal Branches

In some cases, an internal submission needs to be applied to several branches;
in these cases, one submission should be the canonical submission, the one that
goes through a full code review; once it is submitted it can be cherry picked to
other branches. These cherry-picks should be identical to the original with the
exception of possible merge conflict resolution. The cherry-picks should also
have a special footer:

```
Origin-<version>-SHA1: <commit hash>
```

Where `commit hash` is the hash of the original commit and `version` is the
version of the kernel that the original commit is in.

## Rebasing

Frequently the kernel will need to be rebased to pull in both features that were
developed the normal way, described above, as well as features developed by
upstream. We should aim to do a rebase for every version of the Linux kernel.

When the current development branch is to be rebased, all commits that have been
dropped (have been listed as `Dropped` in a commit footer); will be
automatically dropped. Each commit that has been accepted upstream (marked
`Upstream` in the commit footer) will be dropped if that commit is in the branch
we are rebasing to and will otherwise be rebased like all other commits. If a
commit, or series of commits that are internal only cannot be rebased cleanly,
the internal maintainer will be responsible for rebasing those commits. The
internal maintainer is also responsible for validating the things that she
maintains.
