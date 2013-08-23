#!/bin/bash -e

MAJOR_VERSION=$1 #get the version level to build
PATH_2_BUILD=$(readlink -f $2) #get the absolute path of the directory to build

exit_F() {
	cd $cwd
	if [ -z "$1" ]; then
		exit 0
	else
		echo $1 #1>&2
		exit 1
	fi
}

ACAPELLA_LIB=(/usr/local/PerkinElmerCTG/Acapella"$MAJOR_VERSION".*/lib)
#Get the latest version available
for path in $ACAPELLA_LIB; do
	regex="/Acapella"$MAJOR_VERSION"\.([0-9]{1,3})/lib"
	maxversion=0
	if [[ $ACAPELLA_LIB =~ $regex ]]; then
		subversion=${BASH_REMATCH[1]}
		[[ $maxversions -lt $subversion ]] && maxversion=$subversion
	fi
done

ACAPELLA_LIB=/usr/local/PerkinElmerCTG/Acapella"$MAJOR_VERSION"."$maxversion"/lib
SCRIPT_DIR=$(readlink -f ${0%/*})


ACAPELLA_SDK=$SCRIPT_DIR/Libraries/AcapellaSDK"$MAJOR_VERSION"/include


[[ $MAJOR_VERSION =~ [0-9]{1,2}.[0-9]{1,2} ]] || exit_F "Please provide the version to compile for in the first argument. '$MAJOR_VERSION' does not match a version"
[ ! -e "$PATH_2_BUILD" ] && exit_F "'$PATH_2_BUILD' does not exist. Please provide provide a valid folder to build."
[ ! -e "$ACAPELLA_LIB" ] && exit_F "'$ACAPELLA_LIB' does not exist. The acapella Libray for version $MAJOR_VERSION is missing. Make sure you are building the correct version"
[ ! -e "$ACAPELLA_SDK" ] && exit_F "'$ACAPELLA_SDK' does not exist. The acapella SDK for version $MAJOR_VERSION is missing. Make sure you are building the correct version."

[ -z "$ARCH" ] && ARCH=`uname -s`_`uname -m`
BIN_DIR=$PATH_2_BUILD/bin/v$MAJOR_VERSION/$ARCH
mkdir -p "$BIN_DIR"

moduleDirs=`dir -d1 "$PATH_2_BUILD"/* | perl -pe 's:/$::'`;
for moduleDir in $moduleDirs; do
	cppList=`find $moduleDir -name *.cpp`

	 # directory does not exists or does not contains cpp files
	[ -z "$cppList" ] && continue;

	module=${moduleDir##*/}
	cmd="g++ -Wall -fpic -D_GNU_SOURCE -MMD -fexceptions -pthread  -g -O2  -I"$ACAPELLA_SDK" $cppList -shared -Wl,-soname,"$module.so.2" -o "$BIN_DIR/lib$module.so.2.7" -L"$ACAPELLA_LIB"  -limacro -lbaseutil -lmemblock -lcells"

	echo -e "\n\nCompiling $module:\n\n\t$cmd\n"
	set +e
	$cmd
	if [ $? -eq 0 ]; then
		rm -f $BIN_DIR/lib$module.so.2.d 2> /dev/null;
		echo "Done compiling $module"
	else
		echo "Failed compiling $module"
	fi
	set -e
done

