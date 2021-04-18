#!/bin/bash

# ./build.sh -b $TRAVIS_BUILD_NUMBER -c $TRAVIS_COMPILER -x $TRAVIS_OS_NAME -n $TRAVIS_JOB_NAME
# ./build.sh -b 12 -c clang -x linux -n "Focal.20.04.clang"
# ./build.sh -b 12 -c gcc -x linux -n "Focal.20.04.GCC"

# tar zxf .

#+ colors const
#    colorGREEN='\033[0;32m'
#    colorRED='\033[0;31m'
#    colorBLUE='\033[0;34m'
#    colorDef='\033[0m'
#~ colors const

Patch=0

while getopts b:c:x:n: flag
do
    case "${flag}" in
        b) buildnum=${OPTARG};;
        c) compiler=${OPTARG};;
        x) osname=${OPTARG};;
        n) buildname=${OPTARG};;
    esac
done

if [ -z ${buildnum+x} ]; then
    echo "Build number is unset";
    exit 1;
fi

if [ -z ${compiler+x} ]; then
    echo "Compiler is unset";
    exit 1;
fi

if [ -z ${osname+x} ]; then
    echo "OS name is unset";
    exit 1;
fi

if [ -z ${buildname+x} ]; then
    echo "Build name is unset";
    exit 1;
fi

CURR_DIR=$(pwd)

osname=${osname,,}

chmod +x "${CURR_DIR}/debian/rules"

export PATH="$HOME/Qt/5.15.2/gcc_64/bin/:$PATH"

echo -e "Build number \t[$buildnum]";
echo -e "Compiler     \t[$compiler]";
echo -e "OS name      \t[$osname]";
echo -e "Build name   \t[$buildname]";
echo -e "PWD          \t[$CURR_DIR]";
echo -e "PATH         \t[$PATH]";

#windows
if [[ "$osname" == "windows" ]]; then
    echo "windows is unsupported";
    exit 1;
fi

echo ""
echo "***** Generate version";
echo "* Parse GIT attributes..."
echo "* TAG Expected format v000.000-release_descriptor"

revision="HEAD"
buildName="Auto-build"

FullCommit=$(git -C $CURR_DIR rev-parse $revision)
if [ -z ${FullCommit+x} ]; then
    echo "Failed on get commit for $revision revision [$FullCommit]";
    exit 1;
fi

echo "* FullCommit [$FullCommit]"

LastTag=$(git -C $CURR_DIR describe --tags --first-parent --match "*" $revision)

echo "* TAG [$LastTag]"

if [ -z ${LastTag+x} ]; then
  LastTag="0.0.0.$buildnum"
  echo "Failed on get tag for revision $revision - defaulting to $LastTag"
fi

parsestr="$(echo -e "${LastTag}" | sed -e 's/^[a-zA-Z]*//')"
Major=`echo "${parsestr}" | awk -F"[./-]" '{print $1}'`
Minor=`echo "${parsestr}" | awk -F"[./-]" '{print $2}'`
buildName=`echo "${parsestr}" | awk -F"[./-]" '{print $3}'`

echo -e "* [$LastTag] => [$Major.$Minor.$Patch.$buildnum]"
echo -e "* Release Name [$buildName]"

echo ""
echo "***** Test compiler";

if [[ $compiler == clang* ]]; then
    QMAKE_CXX=clang++
    QMAKE_CC=clang
    QMAKESPEC=linux-clang
elif [[ $compiler == gcc* ]]; then
    QMAKE_CXX=g++
    QMAKE_CC=gcc
    QMAKESPEC=linux-g++
    # gcc -v --help 2> /dev/null | grep -iv deprecated | grep "C++" | sed -n '/^ *-std=\([^<][^ ]\+\).*/ {s//\1/p}';
else
    echo "Compiler is not set or unsupported [$compiler]";
    exit 1;
fi

export CC=${QMAKE_CC}
export CXX=${QMAKE_CXX}

# print compiler version
${CC} --version
retval=$?; if ! [[ $retval -eq 0 ]]; then echo "${colorRED}Error [$retval]${colorDef}"; exit 1; fi

echo "***** Test Qt";

REQUIRES_INSTALL_QT_5=false

if ! [ -x "$(command -v qmake)" ]; then
    echo "Qt not found. Requires Qt 5 to be installed."
    # Qt missing...
    REQUIRES_INSTALL_QT_5=true
fi

if [[ "$REQUIRES_INSTALL_QT_5" = false ]]; then
    qmake -v | grep -q 'Using Qt version 5'
    if [[ $? -eq 1 ]]; then
        qmake -qt=qt5 -v | grep -q 'Using Qt version 5'
        if [[ $? -eq 0 ]]; then
            XQFLAG="-qt=qt5"
        else
            echo "Requires Qt 5 to be installed."
            REQUIRES_INSTALL_QT_5=true
        fi
    fi
fi

if [[ "$REQUIRES_INSTALL_QT_5" = true ]]; then
    # Qt 5 missing or wrong version...
    #linux
    if [[ "$osname" == "linux" ]]; then
        sudo apt-get -y --nodeps install qt5-qmake qtbase5-dev qttools5-dev-tools devscripts fakeroot debhelper;
#         sudo apt-get -y install sbuild schroot debootstrap
    fi
    #osx
    if [[ "$osname" == "osx" ]]; then
        brew update ;
        brew install qt5 ;
        export PATH=$(brew --prefix)/opt/qt5/bin:$PATH ;
        echo $PATH;
    fi
fi

if [[ "$REQUIRES_INSTALL_QT_5" = false ]]; then
    qmake -v | grep -q 'Using Qt version 5'
    if [[ $? -eq 1 ]]; then
        qmake -qt=qt5 -v | grep -q 'Using Qt version 5'
        if [[ $? -eq 0 ]]; then
            XQFLAG="-qt=qt5"
        fi
    fi
fi

qmake $XQFLAG --version
retval=$?; if ! [[ $retval -eq 0 ]]; then echo "${colorRED}Error [$retval]${colorDef}"; exit 1; fi

echo ""
echo "***** Prepare ver.h";

versionFile="\n#define FVER_NAME \"${buildName}\"\n#define FVER1 $Major\n#define FVER2 $Minor\n#define FVER3 $buildnum\n#define FVER4 0\n"
echo -e "$versionFile"
echo -e "$versionFile" > "${CURR_DIR}/ver.h"

echo "*** Create release note..."
 
gitTagList=$(git -C $CURR_DIR tag --sort=-version:refname)
gitTagListCount=$(echo -n "$gitTagList" | grep -c '^')

if [[ 1 -ge "$gitTagListCount" ]]; then
    echo "Use all entries for a release note:"
     releaseNote=$(git -C $CURR_DIR log --date=short --pretty=format:"  * %ad [%aN] %s")
else
    tmpval=$(echo "${gitTagList}" | awk 'NR==2{print}')
    gitTagRange="${tmpval}..${revision}"
    echo "Release note range [$gitTagRange]"
    releaseNote=$(git -C $CURR_DIR log "$gitTagRange" --date=short --pretty=format:"  * %ad [%aN] %s")
fi

echo ""
echo "***** Prepare distr changelog";
echo ""

echo "logview (${Major}.${Minor}.${buildnum}) unstable; urgency=medium"         > "${CURR_DIR}/debian/changelog"
echo ""                                                                        >> "${CURR_DIR}/debian/changelog"
echo "$releaseNote"                                                            >> "${CURR_DIR}/debian/changelog"
echo ""                                                                        >> "${CURR_DIR}/debian/changelog"
echo " -- chipmunk.sm <dannico@linuxmail.org>  $(LANG=C date -R)"              >> "${CURR_DIR}/debian/changelog"

cat "${CURR_DIR}/debian/changelog"

echo ""
echo "***** Run $ debuild -- binary";
echo ""

debuild -- binary
retval=$?; if ! [[ $retval -eq 0 ]]; then echo "${colorRED}Error [$retval]${colorDef}"; exit 1; fi

exit 0

# echo ""
# echo "***** Prepare orig.tar.gz";
# echo ""

# tar czf "${CURR_DIR}/../logview_${Major}.${Minor}.${buildnum}.orig.tar.gz" "${CURR_DIR}"

# archxx=amd64 
# sudo apt-get install sbuild schroot debootstrap
# sudo sbuild-adduser $LOGNAME
# cp /usr/share/doc/sbuild/examples/example.sbuildrc $HOME/.sbuildrc # copy example config into your home as suggested
# sudo apt install apt-cacher-ng
# newgrp sbuild
# sudo sbuild-createchroot --include=eatmydata,ccache,gnupg unstable /srv/chroot/unstable-${archxx}-sbuild http://127.0.0.1:3142/deb.debian.org/debian
# 
# sudo sbuild-update -udcar u
# sbuild

# echo "***** Prepare path";
# 
# RELEASE_DIR=${CURR_DIR}/Release/logview_${buildname,,}_${Major}.${Minor}.${buildnum}
# echo $RELEASE_DIR;
# 
# mkdir -p $RELEASE_DIR
# 
# cd $RELEASE_DIR
# 
# echo "Build path [$(pwd)]" ;
# 
# echo "***** Run qmake";
# 
# qmake $XQFLAG -Wall -spec $QMAKESPEC QMAKE_CC=$QMAKE_CC QMAKE_CXX=$QMAKE_CXX QMAKE_LINK=$QMAKE_CXX ../../*.pro ;
# retval=$?; if ! [[ $retval -eq 0 ]]; then echo "${colorRED}Error [$retval]${colorDef}"; exit 1; fi
# 
# echo "***** Run make";
# 
# make -j4
# retval=$?; if ! [[ $retval -eq 0 ]]; then echo "${colorRED}Error [$retval]${colorDef}"; exit 1; fi
# 
# echo "***** Cleanup ";
# 
# rm -f ./*.o
# rm -f ./*.cpp
# rm -f ./*.h
# rm -f ./Makefile
# rm -f ./.qmake.stash
# rm -rf "${RELEASE_DIR}/translations"
# 
# rm -rf "${RELEASE_DIR}/usr"
# rm -rf "${RELEASE_DIR}/debian"
# rm -rf "${RELEASE_DIR}/DEBIAN"
# 
# echo "***** create deb ";
# 
# mkdir -p                               "${RELEASE_DIR}/usr/bin"
# retval=$?; if ! [[ $retval -eq 0 ]]; then echo "${colorRED}Error [$retval]${colorDef}"; exit 1; fi
# mv "${RELEASE_DIR}/logview"            "${RELEASE_DIR}/usr/bin/"
# retval=$?; if ! [[ $retval -eq 0 ]]; then echo "${colorRED}Error [$retval]${colorDef}"; exit 1; fi
# 
# mkdir -p                               "${RELEASE_DIR}/usr/share/applications"
# retval=$?; if ! [[ $retval -eq 0 ]]; then echo "${colorRED}Error [$retval]${colorDef}"; exit 1; fi
# cp "${CURR_DIR}/data/logview.desktop"  "${RELEASE_DIR}/usr/share/applications/"
# retval=$?; if ! [[ $retval -eq 0 ]]; then echo "${colorRED}Error [$retval]${colorDef}"; exit 1; fi
# 
# mkdir -p                               "${RELEASE_DIR}/usr/share/icons/hicolor/scalable/apps"
# retval=$?; if ! [[ $retval -eq 0 ]]; then echo "${colorRED}Error [$retval]${colorDef}"; exit 1; fi
# cp "${CURR_DIR}/data/logview_logo.svg" "${RELEASE_DIR}/usr/share/icons/hicolor/scalable/apps/"
# retval=$?; if ! [[ $retval -eq 0 ]]; then echo "${colorRED}Error [$retval]${colorDef}"; exit 1; fi
# 
# cp -R "${CURR_DIR}/debian/" "${RELEASE_DIR}/DEBIAN/"
# retval=$?; if ! [[ $retval -eq 0 ]]; then echo "${colorRED}Error [$retval]${colorDef}"; exit 1; fi
# 
# chmod +x "${RELEASE_DIR}/DEBIAN/rules"
# 
# echo "***** Collect shlib ";
# 
# rm -f "${RELEASE_DIR}/debian"
# ln -s "${RELEASE_DIR}/DEBIAN" "${RELEASE_DIR}/debian"
# shlibsDepends=$(dpkg-shlibdeps "${RELEASE_DIR}/usr/bin/logview" -O)
# rm -f "${RELEASE_DIR}/debian"
# 
# shlibsDepends=${shlibsDepends/shlibs:Depends=/}
# # echo "dpkg-shlibdeps [${shlibsDepends}]"
# 
# echo "***** Update control ";
# 
# sed -i "s/\${shlibs:Depends}/${shlibsDepends}/"   "${RELEASE_DIR}/DEBIAN/control"
# sed -i "s/, \${misc:Depends}//"                   "${RELEASE_DIR}/DEBIAN/control"
# sed -i "s/Architecture: any/Architecture: amd64/" "${RELEASE_DIR}/DEBIAN/control"
# 
# echo "Version: ${Major}.${Minor}-${buildnum}~$(date +%Y%m%d%H%M%S)~$(lsb_release -si)$(lsb_release -sr)" >> "${RELEASE_DIR}/DEBIAN/control"
# 
# cat "${RELEASE_DIR}/DEBIAN/control"
# 
# dpkg-deb --build --root-owner-group "${RELEASE_DIR}"
# retval=$?; if ! [[ $retval -eq 0 ]]; then echo "${colorRED}Error [$retval]${colorDef}"; exit 1; fi
# 
# md5sum ${RELEASE_DIR}.deb
# retval=$?; if ! [[ $retval -eq 0 ]]; then echo "${colorRED}Error [$retval]${colorDef}"; exit 1; fi
# 
# exit 0

