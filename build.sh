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
if [ -z ${buildnum} ]; then buildnum=$TRAVIS_BUILD_NUMBER; fi
if [ -z ${buildnum} ]; then buildnum=$GITHUB_RUN_NUMBER; fi

buildname=$APPVEYOR_JOB_NAME
if [ -z ${buildname} ]; then buildname=$TRAVIS_JOB_NAME; fi

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
if [ -z ${debbuild+x} ];     then debbuild=false; fi
if [ -z ${androidbuild+x} ]; then androidbuild=false; fi
if [ -z ${extdbuild+x} ];    then extdbuild=false; fi

if [ -z ${compiler+x} ]; then
    if [[ "$androidbuild" == true ]]; then
        compiler=clang ;
    else
        compiler=gcc;
    fi
fi

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

export BUILD_VERSION_IDS="$Major.$Minor.$buildnum.$appBranch"

if [[ "$debbuild" == false ]] && [[ "$androidbuild" == false ]] && [[ "$extdbuild" == false ]]; then
    echo "";
    echo "***** Build flag not set - exit";
    echo "";
    exit 0;
fi

# ls -la $HOME/Qt/5.15.2
#     exit 0;

# ************************************************************************

echo "***** Test Qt";

REQUIRES_INSTALL_QT_5=false

if [[ "$androidbuild" == false ]]; then
    xQttype=gcc_64
    echo "Search Qt gcc_64..."
else
    xQttype=android
    echo "Search Qt for android..."
fi

xQtpath515=$HOME/Qt/5.15/${xQttype}/bin
xQtpath5152=$HOME/Qt/5.15.2/${xQttype}/bin

if [ -x "$(command -v $xQtpath515/qmake)" ]; then
    echo "Found Qt path [$xQtpath515]"
    export PATH="$xQtpath515/:$PATH"
elif [ -x "$(command -v $xQtpath5152/qmake)" ]; then
    echo "Found Qt path [$xQtpath5152]"
    export PATH="$xQtpath5152/:$PATH"
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
fi

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

# ANDROID_SDK_ROOT
if [ -z "${ANDROID_SDK_ROOT+x}" ]; then
     if   [ -x "$(command -v $HOME/android-sdk/cmdline-tools/tools/bin/sdkmanager)" ]; then
         echo "Use exist SDK (tools)"
         export ANDROID_SDK_ROOT=$HOME/android-sdk
     elif [ -x "$(command -v $HOME/android-sdk/cmdline-tools/latest/bin/sdkmanager)" ]; then
         echo "Use exist SDK (latest)"
         export ANDROID_SDK_ROOT=$HOME/android-sdk
     else
         if ! [ -f "$HOME/androidsdk.zip" ]; then
            echo "Download android SDK"
            curl -L -k -s -o "$HOME/androidsdk.zip" https://dl.google.com/android/repository/commandlinetools-linux-6200805_latest.zip;
         fi
         echo "Unzip SDK"
         export ANDROID_SDK_ROOT=$HOME/android-sdk

         mkdir -p "$HOME/.android"
         touch "$HOME/.android/repositories.cfg"

         mkdir -p $ANDROID_SDK_ROOT
         unzip -qq "$HOME/androidsdk.zip" -d "$ANDROID_SDK_ROOT/cmdline-tools";

         echo "Installing packages"
         cd $ANDROID_SDK_ROOT/cmdline-tools/tools/bin
         yes | ./sdkmanager --licenses 
         yes | ./sdkmanager --update
         ./sdkmanager "platform-tools" "platforms;android-30" "build-tools;30.0.2"
         cd -
     fi
fi

# ANDROID_NDK_ROOT
if ! [ -x "$(command -v $ANDROID_NDK_ROOT/prebuilt/linux-x86_64/bin/make)" ]; then

     if [ -d "$ANDROID_SDK_ROOT/ndk" ]; then
         echo "Local NDK folder exist [$ANDROID_SDK_ROOT/ndk]"
         testFolder=$ANDROID_SDK_ROOT/ndk/*/
         listNdk=$(ls -d $testFolder | sort)
         echo -e "Local NDK list:\n$listNdk"
         export ANDROID_NDK_ROOT=$(echo "$listNdk" | tail -n1)
         echo -e "NDK path for test [$ANDROID_NDK_ROOT]"
     fi

     if [ -x "$(command -v $ANDROID_NDK_ROOT/prebuilt/linux-x86_64/bin/make)" ]; then
         echo "Use ANDROID_NDK_ROOT=[$ANDROID_NDK_ROOT]"
     else
#        sdkUrl="https://dl.google.com/android/repository/android-ndk-r21b-linux-x86_64.zip"
#        SHA1Checksum="50250fcba479de477b45801e2699cca47f7e1267  $HOME/androidndk.zip"
        sdkUrl="https://dl.google.com/android/repository/android-ndk-r21d-linux-x86_64.zip"
        SHA1Checksum="bcf4023eb8cb6976a4c7cff0a8a8f145f162bf4d  $HOME/androidndk.zip"
#        sdkUrl="https://dl.google.com/android/repository/android-ndk-r21e-linux-x86_64.zip"
#        SHA1Checksum="c3ebc83c96a4d7f539bd72c241b2be9dcd29bda9  $HOME/androidndk.zip"
         if ! [ -f "$HOME/androidndk.zip" ]; then
            echo -e "Valid NDK not found.\nDownload NDK from [$sdkUrl] to [$HOME/androidndk.zip]"
            curl -L -k -s -o "$HOME/androidndk.zip" $sdkUrl;

         fi

         SHA1ChecksumDownload=$(sha1sum $HOME/androidndk.zip)
         if [[ $SHA1Checksum != $SHA1ChecksumDownload ]]; then
            echo "Checksum failed  [$SHA1Checksum] != [$SHA1ChecksumDownload]"
            exit 1
         else
            echo "Checksum OK  [$SHA1Checksum] == [$SHA1ChecksumDownload]"
         fi

         export ANDROID_NDK_ROOT=$ANDROID_SDK_ROOT/ndk

         echo "Unzip NDK [$HOME/androidndk.zip] > [$(pwd)/tmp]"
         unzip -qq "$HOME/androidndk.zip" -d "$(pwd)/tmp";

         listNdkTmpDir=$(ls -d tmp/*/ | sort)
         echo "List Ndk in 'tmp' [$listNdkTmpDir]"

         verfile=$(echo "$listNdkTmpDir" | tail -n1)
         echo "Path to NDK [$verfile]"

         verfile=${verfile}/source.properties
         echo "Path to NDK version file [$verfile]"

         while IFS='=' read -r key value
         do
           key=$(echo $key | tr '.' '_')
           value="$(echo $value | sed -e 's/^[[:space:]]*//')"
           value="$(echo $value | sed -e 's/[[:space:]]*$//')"
            eval ${key}=\${value}
         done < "$verfile"

         echo "NDK revision [$Pkg_Revision]"

         export ANDROID_NDK_ROOT="${ANDROID_NDK_ROOT}/${Pkg_Revision}"
         echo "Set ANDROID_NDK_ROOT=[$ANDROID_NDK_ROOT]"

         mv -v $(pwd)/tmp/android-ndk-r* "$ANDROID_NDK_ROOT"
     fi
fi

#   $HOME/Android/Sdk/android_openssl
    echo "";
    echo "********************************************";
    echo "";

    echo "** JAVA_HOME        = [$JAVA_HOME]";
    echo "** ANDROID_SDK_ROOT = [$ANDROID_SDK_ROOT]";
    echo "** ANDROID_NDK_ROOT = [$ANDROID_NDK_ROOT]";

    echo "";
    echo "********************************************";
    echo "";

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

#     mv -v $SRC_DIR/Artifacts/*.aab $SRC_DIR/
#     mv -v $SRC_DIR/Artifacts/*.apk $SRC_DIR/

    echo "";
    echo "***** End android build";
fi


