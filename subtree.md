# Subtree Architecture

In the past, meta-layer repositories were added to the repo as subtrees. This is
no longer the case. Although the individual meta-\* trees still exist in gerrit
and github for the sake of keeping a complete history, they should not be used,
and new meta layers will simply be checked in within the openbmc/openbmc code
tree.
