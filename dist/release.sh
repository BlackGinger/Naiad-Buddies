#!/bin/bash


if [ $# -eq 0 ]; then
	echo "Error: need to specify version number for release."
	echo "Usage: `basename $0` 0.4.0"
	exit
fi

VER=$1

#CURPWD is where the tar archive will be created.
CURPWD=`pwd`

SCRIPTBASE=`dirname $CURPWD/$0`

pushd $SCRIPTBASE > /dev/null
#GITREPO is the full path to the repository that will be released
GITREPO=`git rev-parse --show-toplevel`
popd > /dev/null

if [ ! -d "$GITREPO/.git" ]; then
	echo "Could not find [$GITREPO.git]. Invalid repository. Aborting."
	exit
fi

TMPDIR=`mktemp -d /tmp/geo2emp_release_tmp.XXXXXXXXXXXX`

#Clone the repo from which this script was run into the tmp directory.
git clone $GITREPO $TMPDIR/geo2emp
pushd $TMPDIR > /dev/null
git checkout -b release-$VER v$VER
popd > /dev/null


#Remove the git metadata
rm -rf $TMPDIR/geo2emp/.git

#Create a tar archive
ARCNAME=geo2emp-src-.$VER.tar.gz
rm -f $ARCNAME
pushd $TMPDIR > /dev/null
echo "Excluding: $SCRIPTBASE/exclude.txt"
echo "Archiving ..."
echo "-------------"
tar -cvzf $CURPWD/geo2emp-src-$VER.tar.gz -X $SCRIPTBASE/exclude.txt *
popd > /dev/null
echo "-------------"
echo "Done."

rm -rf $TMPDIR
