#!/bin/bash

# ./build.sh -b $TRAVIS_BUILD_NUMBER -c $TRAVIS_COMPILER -o $TRAVIS_OS_NAME -n $TRAVIS_JOB_NAME

# ./build.sh  -c gcc   -d true -o linux -n "Focal.20.04.GCC"   -b 15
# ./build.sh           -a true -o linux -n "Android"           -b 16
# ./build.sh  -c clang -e true -o linux -n "Focal.20.04.clang" -b 17

# ./build.sh -a true

# tar zxf .

#+ colors const
#    colorGREEN='\033[0;32m'
#    colorRED='\033[0;31m'
#    colorBLUE='\033[0;34m'
#    colorDef='\033[0m'
#~ colors const

buildnum=$APPVEYOR_BUILD_NUMBER
buildname=$APPVEYOR_JOB_NAME
VERSION_CODENAME=$(awk -F= '$1 == "VERSION_CODENAME" {gsub(/"/, "", $2); print $2}' /etc/os-release)
VERSION_ID=$(awk -F= '$1 == "VERSION_ID" {gsub(/"/, "", $2); print $2}' /etc/os-release)
appBranch=$(git rev-parse --abbrev-ref HEAD)
osname=$(uname -s)

while getopts b:c:o:d:a:e:n: flag
do
    case "${flag}" in
        b) buildnum=${OPTARG};;
        c) compiler=${OPTARG};;
        o) osname=${OPTARG};;
        d) debbuild=${OPTARG};;
        a) androidbuild=${OPTARG};;
        e) extdbuild=${OPTARG};;
        n) buildname=${OPTARG};;
    esac
done

if [ -z ${buildnum} ];       then buildnum=0; fi
if [ -z ${compiler+x} ];     then compiler=gcc; fi
if [ -z ${debbuild+x} ];     then debbuild=false; fi
if [ -z ${androidbuild+x} ]; then androidbuild=false; fi
if [ -z ${extdbuild+x} ];    then extdbuild=false; fi
if [ -z ${buildname} ];      then buildname="${VERSION_CODENAME}.${VERSION_ID}.${compiler}"; fi

osname=${osname,,}

chmod +x "./debian/rules"

echo -e "Build number \t[$buildnum]";
echo -e "Compiler     \t[$compiler]";
echo -e "OS name      \t[$osname]";
echo -e "Build name   \t[$buildname]";
echo -e "PATH         \t[$PATH]";
echo -e "PWD          \t[$(pwd)]";

#windows
if [[ "$osname" == "windows" ]]; then
    echo "windows is unsupported";
    exit 1;
fi

echo "***** Generate version";
LastTag=$(git describe --tags --first-parent --match "*" "HEAD")
if [ -z ${LastTag+x} ]; then LastTag="0.0-NotSet"; fi

parsestr="$(echo -e "${LastTag}" | sed -e 's/^[a-zA-Z]*//')"
Major=`echo "${parsestr}" | awk -F"[./-]" '{print $1}'`
Minor=`echo "${parsestr}" | awk -F"[./-]" '{print $2}'`
releaseName=`echo "${parsestr}" | awk -F"[./-]" '{print $3}'`
echo -e "* [$LastTag] => [$Major.$Minor.$buildnum-$releaseName]";

echo "*** Create release note...";
gitTagList=$(git tag --sort=-version:refname)

if [[ 1 -ge "$(echo -n "$gitTagList" | grep -c '^')" ]]; then
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



versionFile="\n#define FVER_NAME \"${releaseName}\"\n#define FVER1 ${Major}\n#define FVER2 ${Minor}\n#define FVER3 ${buildnum}\n#define FVER4 0\n#define FBRANCH \"$appBranch\"\n#define FDISTR \"$buildname\"\n#define RELEASENOTE \"$releasenote\"\n"
echo -e "$versionFile" > "ver.h"
cat "ver.h";

echo "***** Update build version";


appveyor UpdateBuild -version "$Major.$Minor.$buildnum.$appBranch"

if [[ "$debbuild" == false ]] && [[ "$androidbuild" == false ]] && [[ "$extdbuild" == false ]]; then
    echo "";
    echo "***** Build flag not set - exit";
    echo "";
    exit 0;
fi


# ************************************************************************

echo "***** Test Qt";

REQUIRES_INSTALL_QT_5=false

if ! [ -x "$(command -v qmake)" ] || [[ "$androidbuild" == true ]]; then
    echo "Qt not found. Search for standalone path..."
    if [[ "$androidbuild" == false ]]; then
        tmpcompiler=gcc_64
    else
        tmpcompiler=android
#         compiler=gcc
        compiler=clang
    fi
    if [ -x "$(command -v $HOME/Qt/5.15/${tmpcompiler}/bin/qmake)" ]; then
        PATH="$HOME/Qt/5.15/${tmpcompiler}/bin/:$PATH"
        export PATH
    elif [ -x "$(command -v $HOME/Qt/5.15.2/${tmpcompiler}/bin/qmake)" ]; then
        PATH="$HOME/Qt/5.15.2/${tmpcompiler}/bin/:$PATH"
        export PATH
    fi
fi

if ! [ -x "$(command -v qmake)" ]; then
    echo "Qt not found. Requires Qt 5 to be installed."
    REQUIRES_INSTALL_QT_5=true
fi

if [[ "$REQUIRES_INSTALL_QT_5" == false ]]; then
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
        sudo apt-get install -y --no-install-recommends qt5-qmake qtbase5-dev qttools5-dev-tools;
#         sudo apt-get -y install sbuild schroot debootstrap 
#         sudo apt-get -y devscripts fakeroot debhelper
        REQUIRES_INSTALL_QT_5=false
    #osx
    elif [[ "$osname" == "osx" ]] || [[ "$osname" == "darwin" ]] ; then
        brew update ;
        brew install qt5 ;
        export PATH=$(brew --prefix)/opt/qt5/bin:$PATH ;
        echo $PATH;
        REQUIRES_INSTALL_QT_5=false
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

QTDIR=$(command -v qmake)
echo "qmake path QTDIR = [${QTDIR}]"

qmake $XQFLAG --version
retval=$?; if ! [[ $retval -eq 0 ]]; then echo "Error [$retval]"; exit 1; fi

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
retval=$?; if ! [[ $retval -eq 0 ]]; then echo "Error [$retval]"; exit 1; fi
  
SRC_DIR=$(pwd)

# ************** build with debuild ****************
if [[ "$debbuild" == true ]]; then
    echo "";
    echo "***** Run debuild -- binary";
#     sudo apt-get update;
#     sudo apt-get -y install --no-install-recommends qtbase5-dev qttools5-dev-tools devscripts fakeroot debhelper;

    echo "";
    if [[ "$VERSION_ID" == "16.04" ]]; then debuild binary ; else debuild -- binary ; fi
    retval=$?; if ! [[ $retval -eq 0 ]]; then echo "Error [$retval]"; exit 1; fi
    
    mkdir -p $SRC_DIR/Artifacts

    mv -v $SRC_DIR/../*.deb $SRC_DIR/Artifacts/
#     2>/dev/null || true

    mv -v $SRC_DIR/../*.ddeb $SRC_DIR/Artifacts/
#     2>/dev/null || true

    pushd $PWD

    cd $SRC_DIR/Artifacts
    
    for file in `find -name "*.*deb"`; do mv "$file" "${file/_amd64/_amd64.$buildname}" ; done

    ls -l
    
    popd
    
    mv -v $SRC_DIR/Artifacts/*deb $SRC_DIR/

    echo "";
    echo "***** End debuild -- binary";
    echo "";
fi
    
# ************** build custom ****************
if [[ "$extdbuild" == true ]]; then

    echo "";
    echo "***** Run custom build";
     
    echo "** Prepare path";
    tmpName="${releaseName}.${buildname}"

    RELEASE_DIR=$(pwd)/Release/logview_${Major}.${Minor}.${buildnum}_${tmpName,,}
    echo $RELEASE_DIR;

    rm -rf "${RELEASE_DIR}"
    mkdir -p $RELEASE_DIR

    cd $RELEASE_DIR

    echo "Build path [$(pwd)]";

    echo "** Run qmake";

    qmake $XQFLAG -Wall -spec $QMAKESPEC QMAKE_CC=$QMAKE_CC QMAKE_CXX=$QMAKE_CXX QMAKE_LINK=$QMAKE_CXX ../../*.pro ;
    retval=$?; if ! [[ $retval -eq 0 ]]; then echo "Error [$retval]"; exit 1; fi

    echo "** Run make";

    make -j$(nproc)
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

# tar czf "./../logview_${Major}.${Minor}.${buildnum}.orig.tar.gz" "$(pwd)"

# archxx=amd64 
# sudo apt-get install sbuild schroot debootstrap
# sudo sbuild-adduser $LOGNAME
# cp /usr/share/doc/sbuild/examples/example.sbuildrc $HOME/.sbuildrc # copy example config into your home as suggested
# sudo apt install apt-cacher-ng
# newgrp sbuild
# sudo sbuild-createchroot --include=eatmydata,ccache,gnupg unstable /srv/chroot/unstable-${archxx}-sbuild http://127.0.0.1:3142/deb.debian.org/debian
# # sudo sbuild-update -udcar u
# sbuild 


     echo "";
     echo "***** End custom build";
     cd ${SRC_DIR}
fi;

if [[ "$androidbuild" == true ]]; then
    echo "";
    echo "***** Run android build";
    
    echo "** Prepare path";
    
    RELEASE_DIR=$(pwd)/AndroidRelease

    rm -rf "${RELEASE_DIR}"
    mkdir -p $RELEASE_DIR
    
    cd $RELEASE_DIR

    echo "Android build path [$(pwd)]";

if [ -z "${JAVA_HOME+x}" ]; then
    export JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64
fi

if [ -z "${ANDROID_SDK_ROOT+x}" ]; then
     if [ -x "$(command -v $HOME/Android/Sdk/tools/bin/sdkmanager)" ]; then
         echo "Use local SDK"
         export ANDROID_SDK_ROOT=$HOME/android-sdk
     elif [ -x "$(command -v $HOME/android-sdk/cmdline-tools/tools/bin/sdkmanager)" ]; then
         echo "Use cached SDK"
         export ANDROID_SDK_ROOT=$HOME/android-sdk
     else
         if ! [ -f "$HOME/androidsdk.zip" ]; then
            echo "download SDK"
            curl -L -k -s -o "$HOME/androidsdk.zip" https://dl.google.com/android/repository/commandlinetools-linux-6200805_latest.zip;
         fi
         echo "unzip SDK"
         export ANDROID_SDK_ROOT=$HOME/android-sdk
         mkdir -p $ANDROID_SDK_ROOT
         
         unzip -qq "$HOME/androidsdk.zip" -d "$ANDROID_SDK_ROOT/cmdline-tools";
         touch "$ANDROID_SDK_ROOT/repositories.cfg"
         echo "Installing packages"
         cd $ANDROID_SDK_ROOT/cmdline-tools/tools/bin
         yes | ./sdkmanager --licenses 
         yes | ./sdkmanager --update
         ./sdkmanager "platform-tools" "platforms;android-30" "build-tools;30.0.2"
         cd -
     fi
fi

if [ -z "${ANDROID_NDK_ROOT+x}" ]; then
     if [ -x "$(command -v $HOME/Android/Sdk/ndk/21.3.6528147/prebuilt/linux-x86_64/bin/make)" ]; then
         echo "Use local NDK"
         export ANDROID_NDK_ROOT=$HOME/Android/Sdk/ndk/21.3.6528147
     elif [ -x "$(command -v $HOME/android-ndk/build/ndk-build)" ]; then
         echo "Use cached NDK"
         export ANDROID_NDK_ROOT=$HOME/android-ndk
     else
         if ! [ -f "$HOME/androidndk.zip" ]; then
            echo "download NDK"
            curl -L -k -s -o "$HOME/androidndk.zip" https://dl.google.com/android/repository/android-ndk-r21b-linux-x86_64.zip;
         fi
         echo "unzip NDK"
         export ANDROID_NDK_ROOT=$HOME/android-ndk
         unzip -qq "$HOME/androidndk.zip" -d "$(pwd)/tmp";
         mv -v $(pwd)/tmp/android-ndk-r* "$ANDROID_NDK_ROOT"
     fi
fi


#   $HOME/Android/Sdk/android_openssl

    echo "** JAVA_HOME        = [$JAVA_HOME]";
    echo "** ANDROID_SDK_ROOT = [$ANDROID_SDK_ROOT]";
    echo "** ANDROID_NDK_ROOT = [$ANDROID_NDK_ROOT]";

    echo "** Run qmake";

    qmake $XQFLAG ../*.pro -spec android-clang CONFIG-=debug CONFIG+=release  CONFIG+=qtquickcompiler ANDROID_ABIS="armeabi-v7a arm64-v8a x86 x86_64"
    retval=$?; if ! [[ $retval -eq 0 ]]; then echo "Error [$retval]"; exit 1; fi

    echo "** Run make qmake_all";
    make -f $RELEASE_DIR/Makefile qmake_all
    retval=$?; if ! [[ $retval -eq 0 ]]; then echo "Error [$retval]"; exit 1; fi

    echo "** Run make ";
#     2>&1 > /dev/null
    make -j$(nproc)
    retval=$?; if ! [[ $retval -eq 0 ]]; then echo "Error [$retval]"; exit 1; fi

    echo "** Run make apk_install_target";
    make -j$(nproc) apk_install_target
    retval=$?; if ! [[ $retval -eq 0 ]]; then echo "Error [$retval]"; exit 1; fi
# --verbose --android-platform android-30
    androiddeployqt --input android-logview-deployment-settings.json --no-gdbserver --gradle --aab --jarsigner --release --output android-build --jdk $JAVA_HOME
    
    mkdir -p $SRC_DIR/Artifacts

    mv -v $SRC_DIR/AndroidRelease/android-build/build/outputs/bundle/release/*.aab $SRC_DIR/Artifacts/
    mv -v $SRC_DIR/AndroidRelease/android-build/build/outputs/apk/release/*.apk $SRC_DIR/Artifacts/
#     mv -v $SRC_DIR/AndroidRelease/android-build/build/outputs/bundle/debug/*.aab $SRC_DIR/Artifacts/
#     mv -v $SRC_DIR/AndroidRelease/android-build/build/outputs/apk/debug/*.apk $SRC_DIR/Artifacts/

    echo "** Rename";
    pushd $PWD

    cd $SRC_DIR/Artifacts

    for file in `find -name "*.a*"`; do mv "$file" "${file/android-build/logview_android-${Major}.${Minor}.${buildnum}}" ; done

    ls -l

    popd

    mv -v $SRC_DIR/Artifacts/*.aab $SRC_DIR/
    mv -v $SRC_DIR/Artifacts/*.apk $SRC_DIR/

    echo "";
    echo "***** End android build";
fi;


