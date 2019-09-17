#!/bin/sh

NEW_VERSION=$1
if [[ ! $1 =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]
  then echo "Version $1 is in valid format (should be e.g. 1.0.0)"; exit
fi

echo Updating version to v${NEW_VERSION}
#sed -e 's/\(<text \)\(.*v1\.0\.0\)/\1locked="yes" \2/' -i "" *.brd
#sed -e 's/\(<text locked=\"yes\".*v\)\(\d+\.\d+\.\d+\)/\1${NEW_VERSION}/' -i "" ./eagle/*.brd
sed -e "s/\(<text locked=\"yes\".*\)\(v[0-9]*\.[0-9]*\.[0-9]*\)/\1v${NEW_VERSION}/" -i "" eagle/*.brd
