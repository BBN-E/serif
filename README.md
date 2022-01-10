Serif
-----

Raytheon BBN Technologies uses Serif to perform various NLP processing of text documents.


Building the Code
-----------------

- The builds are automated through Gitlab CI, and will run every time a branch is pushed to origin or merged into master.
See `.gitlab-ci.yml` as the starting point.

- If the CI build fails for any reason, there will be a directory left behind in the pkgmgr account's RAID space.  This is
helpful for debugging to determine what caused the failure.  However, over time these directories can accumulate and take up
a large amount of space.  To clear out any old, stale directories in this space, simply create a branch with the name `clean`,
and push it to origin.  This signals Gitlab CI to run a special cleaning step to clear out any old build directories.

Example:
```
git checkout master
git checkout -b clean
git push origin clean
```


Creating a Release
------------------

Released versions of the software will be stored under `/d4m/ears/releases/serif`, in a directory containing the date and time
stamp of the release.  To create a release, you will create a tag in Git, then push that tag to origin.  This tells Gitlab CI
to run the build as usual, but then run an extra step at the end to copy the contents of the build to `/d4m/ears/releases/serif`.

Example:
```
git checkout master
git tag R2020_07_28
git push origin --tags
```
