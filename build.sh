#!/bin/bash

# ./build.sh -b $TRAVIS_BUILD_NUMBER -c $TRAVIS_COMPILER -o $TRAVIS_OS_NAME -n $TRAVIS_JOB_NAME

# ./build.sh  -c gcc   -d -o linux -n "Focal.20.04.GCC"   -b 15
# ./build.sh           -a -o linux -n "Android"           -b 16
# ./build.sh  -c clang -e -o linux -n "Focal.20.04.clang" -b 17

# tar zxf .

#+ colors const
#    colorGREEN='\033[0;32m'
#    colorRED='\033[0;31m'
#    colorBLUE='\033[0;34m'
#    colorDef='\033[0m'
#~ colors const


while getopts b:c:o:d:a:e:n: flag
do
    case "${flag}" in
        b) buildnum=${OPTARG};;
        c) compiler=${OPTARG};;
        o) osname=${OPTARG};;
        d) debbuild=true;;
        a) androidbuild=true;;
        e) extdbuild=true;;
        n) buildname=${OPTARG};;
    esac
done

if [ -z ${buildnum+x} ]; then buildnum=0; fi
if [ -z ${compiler+x} ]; then compiler=gcc; fi
if [ -z ${osname+x} ]; then osname=$(uname -s); fi
if [ -z ${debbuild+x} ]; then debbuild=false; fi
if [ -z ${androidbuild+x} ]; then androidbuild=false; fi
if [ -z ${extdbuild+x} ]; then extdbuild=false; fi
if [ -z ${buildname+x} ]; then
    VERSION_CODENAME=$(awk -F= '$1 == "VERSION_CODENAME" {gsub(/"/, "", $2); print $2}' /etc/os-release)
    VERSION_ID=$(awk -F= '$1 == "VERSION_ID" {gsub(/"/, "", $2); print $2}' /etc/os-release)
    buildname="${VERSION_CODENAME}.${VERSION_ID}.${compiler}"
else
    buildname="${buildname}"
fi

osname=${osname,,}

chmod +x "./debian/rules"

echo -e "Build number \t[$buildnum]";
echo -e "Compiler     \t[$compiler]";
echo -e "OS name      \t[$osname]";
echo -e "Build name   \t[$buildname]";
echo -e "PATH         \t[$PATH]";

#windows
if [[ "$osname" == "windows" ]]; then
    echo "windows is unsupported";
    exit 1;
fi

echo "***** Generate version";

revision="HEAD"

FullCommit=$(git rev-parse $revision)
if [ -z ${FullCommit+x} ]; then
    echo "Failed on get commit for revision [$revision] commit [$FullCommit]";
    echo "* TAG Expected format v000.000-release_description"
    exit 1;
fi

echo "* FullCommit [$FullCommit]";

LastTag=$(git describe --tags --first-parent --match "*" $revision)

echo "* TAG [$LastTag]";

if [ -z ${LastTag+x} ]; then
  LastTag="0.0.0.$buildnum";
  echo "Failed on get tag for revision [$revision] - defaulting to $LastTag";
  echo "* TAG Expected format v000.000-release_description";
fi

parsestr="$(echo -e "${LastTag}" | sed -e 's/^[a-zA-Z]*//')"
Major=`echo "${parsestr}" | awk -F"[./-]" '{print $1}'`
Minor=`echo "${parsestr}" | awk -F"[./-]" '{print $2}'`
releaseName=`echo "${parsestr}" | awk -F"[./-]" '{print $3}'`

echo -e "* [$LastTag] => [$Major.$Minor.$buildnum]";
releaseName="${buildname}.${releaseName}";
echo -e "* Release Name [$releaseName]";

echo "*** Create release note...";
 
gitTagList=$(git tag --sort=-version:refname)
gitTagListCount=$(echo -n "$gitTagList" | grep -c '^')

if [[ 1 -ge "$gitTagListCount" ]]; then
    echo "Use all entries for a release note:";
    releaseNote=$(git log --date=short --pretty=format:"  * %ad [%aN] %s")
else
    tmpval=$(echo "${gitTagList}" | awk 'NR==2{print}')
    gitTagRange="${tmpval}..${revision}"
    echo "Release note range [$gitTagRange]";
    releaseNote=$(git log "$gitTagRange" --date=short --pretty=format:"  * %ad [%aN] %s")
fi

echo ""
echo "***** Create distr changelog...";
echo -e "logview (${Major}.${Minor}.${buildnum}) unstable; urgency=medium\n$releaseNote\n -- chipmunk.sm <dannico@linuxmail.org>  $(LANG=C date -R)" > "debian/changelog"
cat "debian/changelog";

echo ""
echo "***** Create ver.h";

versionFile="\n#define FVER_NAME \"${releaseName}\"\n#define FVER1 ${Major}\n#define FVER2 ${Minor}\n#define FVER3 ${buildnum}\n#define FVER4 0\n"
echo -e "$versionFile" > "ver.h"
cat "ver.h";

if [[ "$debbuild" == false ]] && [[ "$androidbuild" == false ]] && [[ "$extdbuild" == false ]]; then
    echo "";
    echo "***** Build flag not set - exit";
    echo "";
    exit 0;
fi

# ************************************************************************
export PATH="$HOME/Qt/5.15/gcc_64/bin/:$HOME/Qt/5.15.2/gcc_64/bin/:$PATH"

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

echo -e "PWD \t[$(pwd)]";

export CC=${QMAKE_CC}
export CXX=${QMAKE_CXX}

# print compiler version
${CC} --version
retval=$?; if ! [[ $retval -eq 0 ]]; then echo "Error [$retval]"; exit 1; fi

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
        sudo apt-get -y install --no-install-recommends qt5-qmake qtbase5-dev qttools5-dev-tools devscripts fakeroot debhelper;
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
retval=$?; if ! [[ $retval -eq 0 ]]; then echo "Error [$retval]"; exit 1; fi

# ************** build with debuild ****************
if [[ "$debbuild" == true ]]; then
    echo "";
    echo "***** Run $ debuild -- binary";
    echo "";
    debuild -- binary
    retval=$?; if ! [[ $retval -eq 0 ]]; then echo "Error [$retval]"; exit 1; fi
    echo "";
    echo "***** End $ debuild -- binary";
    echo "";
fi

# ************** build custom ****************
if [[ "$extdbuild" == true ]]; then

     echo "";
     echo "***** Run $ custom build";
     echo "** Prepare path";
    SRC_DIR=$(pwd)
    RELEASE_DIR=$(pwd)/Release/logview_${releaseName,,}_${Major}.${Minor}.${buildnum}
    echo $RELEASE_DIR;

    rm -rf "${RELEASE_DIR}"
    mkdir -p $RELEASE_DIR

    cd $RELEASE_DIR

    echo "Build path [$(pwd)]";

    echo "** Run qmake";

    qmake $XQFLAG -Wall -spec $QMAKESPEC QMAKE_CC=$QMAKE_CC QMAKE_CXX=$QMAKE_CXX QMAKE_LINK=$QMAKE_CXX ../../*.pro ;
    retval=$?; if ! [[ $retval -eq 0 ]]; then echo "Error [$retval]"; exit 1; fi

    echo "** Run make";

    make -j4
    retval=$?; if ! [[ $retval -eq 0 ]]; then echo "Error [$retval]"; exit 1; fi

#     echo "** Cleanup ";
# 
#     rm -f ./*.o
#     rm -f ./*.cpp
#     rm -f ./*.h
#     rm -f ./Makefile
#     rm -f ./.qmake.stash
#     rm -rf "${RELEASE_DIR}/translations"
# 
#     rm -rf "${RELEASE_DIR}/usr"
#     rm -rf "${RELEASE_DIR}/debian"
#     rm -rf "${RELEASE_DIR}/DEBIAN"
# 
#     echo "***** create deb ";
# 
#     mkdir -p "${RELEASE_DIR}/usr/bin"
#     retval=$?; if ! [[ $retval -eq 0 ]]; then echo "Error [$retval]"; exit 1; fi
#     mv "${RELEASE_DIR}/logview" "${RELEASE_DIR}/usr/bin/"
#     retval=$?; if ! [[ $retval -eq 0 ]]; then echo "Error [$retval]"; exit 1; fi
# 
#     mkdir -p "${RELEASE_DIR}/usr/share/applications"
#     retval=$?; if ! [[ $retval -eq 0 ]]; then echo "Error [$retval]"; exit 1; fi
#     cp "${SRC_DIR}/data/logview.desktop" "${RELEASE_DIR}/usr/share/applications/"
#     retval=$?; if ! [[ $retval -eq 0 ]]; then echo "Error [$retval]"; exit 1; fi
# 
#     mkdir -p "${RELEASE_DIR}/usr/share/icons/hicolor/scalable/apps"
#     retval=$?; if ! [[ $retval -eq 0 ]]; then echo "Error [$retval]"; exit 1; fi
#     cp "${SRC_DIR}/data/logview_logo.svg" "${RELEASE_DIR}/usr/share/icons/hicolor/scalable/apps/"
#     retval=$?; if ! [[ $retval -eq 0 ]]; then echo "Error [$retval]"; exit 1; fi
# 
#     cp -R "${SRC_DIR}/debian/" "${RELEASE_DIR}/DEBIAN/"
#     retval=$?; if ! [[ $retval -eq 0 ]]; then echo "Error [$retval]"; exit 1; fi
# 
#     chmod +x "${RELEASE_DIR}/DEBIAN/rules"
# 
#     echo "***** Collect shlib ";
# 
#     rm -f "${RELEASE_DIR}/debian"
#     ln -s "${RELEASE_DIR}/DEBIAN" "${RELEASE_DIR}/debian"
#     shlibsDepends=$(dpkg-shlibdeps "${RELEASE_DIR}/usr/bin/logview" -O)
#     rm -f "${RELEASE_DIR}/debian"
# 
#     shlibsDepends=${shlibsDepends/shlibs:Depends=/}
#     # echo "dpkg-shlibdeps [${shlibsDepends}]"
# 
#     echo "***** Update control ";
# 
#     sed -i "s/\${shlibs:Depends}/${shlibsDepends}/"   "${RELEASE_DIR}/DEBIAN/control"
#     sed -i "s/, \${misc:Depends}//"                   "${RELEASE_DIR}/DEBIAN/control"
#     sed -i "s/Architecture: any/Architecture: amd64/" "${RELEASE_DIR}/DEBIAN/control"
# 
# #     echo "Version: ${Major}.${Minor}-${buildnum}~$(date +%Y%m%d%H%M%S)~$(lsb_release -si)$(lsb_release -sr)" >> "${RELEASE_DIR}/DEBIAN/control"
# 
#     cat "${RELEASE_DIR}/DEBIAN/control"
# 
#     dpkg-deb --build --root-owner-group "${RELEASE_DIR}"
#     retval=$?; if ! [[ $retval -eq 0 ]]; then echo "Error [$retval]"; exit 1; fi
# 
#     md5sum ${RELEASE_DIR}.deb
#     retval=$?; if ! [[ $retval -eq 0 ]]; then echo "Error [$retval]"; exit 1; fi
    
     echo "";
     echo "***** End $ custom build";

fi;

if [[ "$androidbuild" == true ]]; then
    echo "";
    echo "***** Run $ android build";
    
    echo "** Prepare path";
    SRC_DIR=$(pwd)
    RELEASE_DIR=$(pwd)/Release/logview_${releaseName,,}_${Major}.${Minor}.${buildnum}
    echo $RELEASE_DIR;

    rm -rf "${RELEASE_DIR}"
    mkdir -p $RELEASE_DIR

    cd $RELEASE_DIR

    echo "Build path [$(pwd)]";

    echo "** Run qmake";

    qmake $XQFLAG -Wall -spec $QMAKESPEC QMAKE_CC=$QMAKE_CC QMAKE_CXX=$QMAKE_CXX QMAKE_LINK=$QMAKE_CXX ../../*.pro ;
    retval=$?; if ! [[ $retval -eq 0 ]]; then echo "Error [$retval]"; exit 1; fi

    echo "** Run make";

    make -j4
    retval=$?; if ! [[ $retval -eq 0 ]]; then echo "Error [$retval]"; exit 1; fi
   
    echo "";
    echo "***** End $ android build";
fi;

# 
# exit 0

# echo ""
# echo "***** Prepare orig.tar.gz";
# echo ""

# tar czf "./../logview_${Major}.${Minor}.${buildnum}.orig.tar.gz" "$(pwd)"

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
