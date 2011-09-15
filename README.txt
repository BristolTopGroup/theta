The theta subversion structure has been changed on September 15, 2011.
'trunk' is not used anymore. Instead, there are 'stable' and 'testing' tags:
 * users should usually use the 'stable' tag. To check out a new version of theta, use
   #  svn co https://ekptrac.physik.uni-karlsruhe.de/public/theta/tags/stable theta
   to switch from a setup currently using 'trunk', you can switch to the 'stable' tag by executing
   (in the theta directory):
   #  svn switch https://ekptrac.physik.uni-karlsruhe.de/public/theta/tags/stable
 * development is done in the 'testing' tag. Users usually don't need to care about that, unless they
   want to use / test new features.
   (while by usual subversion conventions, development would be done in 'trunk',
   many users currently use 'trunk' as default. Starting to develop new features there might
   break their setup.)

The code in trunk will be kept for a while, in order not to overwhlem users who want to
update with incoming deletes. However, it will be made uncompilable by replacing the
Makefile and CMakelists.txt.

