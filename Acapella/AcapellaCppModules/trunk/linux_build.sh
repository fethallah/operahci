#!/bin/bash -e

MAJOR_VERSION=$1 #get the version level to build
PATH_2_BUILD=$(readlink -f $2) #get the absolute path of the directory to build

exit_F() {
	cd $cwd
	if [ -z "$1" ]; then
		exit 0
	else
		echo $1 1>&2
		exit 1
	fi
}

ACAPELLA_LIB=(/usr/local/PerkinElmerCTG/Acapella"$MAJOR_VERSION".?/lib)
ACAPELLA_LIB=${ACAPELLA_LIB[0]}
SCRIPT_DIR=$(readlink -f ${0%/*})


ACAPELLA_SDK=$SCRIPT_DIR/Libraries/AcapellaSDK"$MAJOR_VERSION"_Linux/include
[ -z "$ARCH" ] && ARCH=`uname -s`_`uname -m`
BIN_DIR=$PATH_2_BUILD/bin/v$MAJOR_VERSION/$ARCH

[[ $MAJOR_VERSION =~ [0-9]{1,2}.[0-9]{1,2} ]] || exit_F "Please provide the version to compile for in the first argument. '$MAJOR_VERSION' does not match a version"
[ -f "$PATH_2_BUILD" ] && exit_F "Please provide provide a valid folder to build."
[ -f "$ACAPELLA_LIB" ] && exit_F "The acapella Libray for version $MAJOR_VERSION is missing. Make sure you are building the correct version. Cannot find: '$ACAPELLA_LIB'"
[ -f "$ACAPELLA_SDK" ] && exit_F "The acapella SDK for version $MAJOR_VERSION is missing. Make sure you are building the correct version. Cannot find: '$ACAPELLA_SDK'"



moduleDirs=`dir -d1 "$PATH_2_BUILD"/* | perl -pe 's:/$::'`;
for moduleDir in $moduleDirs; do
	cppList=`find $moduleDir -name *.cpp`

	 # directory does not exists or does not contains cpp files
	[ -z "$cppList" ] && continue;

	module=${moduleDir##*/}
	echo working on: $module
	mkdir -p "$BIN_DIR"
	g++ -Wall -fpic -D_GNU_SOURCE -MMD -fexceptions -pthread  -g -O2  -l"$ACAPELLA_SDK" $cppList -shared -Wl,-soname,"$module.so.2" -o "$BIN_DIR/lib$module.so.2.7" -L"$ACAPELLA_LIB"  -limacro -lbaseutil -lmemblock -lcells;

	rm -f $BIN_DIR/lib$module.so.2.d 2> /dev/null;
done

