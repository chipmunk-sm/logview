#/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */


TARGET = logview
TEMPLATE = app

QT += core gui widgets
#CONFIG += c++11
#CONFIG += c++14

win32:!winrt:!wince {
    CONFIG += c++latest
} else {
    equals(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 12) {
        #message("Qt 5.11 and earlier")
        QMAKE_CXXFLAGS += -std=c++17
        CONFIG += c++1z
    } else {
       #message("Qt 5.12 and later")
       CONFIG += c++17
    }
}

linux-clang++{
    message("**** clang++")
}

linux-g++{
    message("**** g++")
}

win32:!winrt:!wince {
    QT += winextras
    LIBS += -lshell32 -luser32
}

android {

    QT += androidextras
    QT += svg

    ANDROID_API=21
    __ANDROID_API__=android-21
    DEFAULT_ANDROID_PLATFORM=21
    ANDROID_NDK_PLATFORM=android-21
    DEFAULT_ANDROID_PLATFORM=android-21
    API_VERSION = android-21

    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
}


DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    clogdatasource.cpp \
    main.cpp \
    logviewmodel.cpp \
    logviewtable.cpp \
    logviewwidget.cpp \
    logviewselectionmodel.cpp \
    fileselector.cpp \
    iconhelper.cpp \
    fileselectorstandardmodel.cpp \
    fileselectorlocations.cpp \
    fileselectorlocationsenum.cpp \
    fileselectorlineedit.cpp \
    fileselectormodelitem.cpp \
    permissionhelper.cpp \
    stylehelper.cpp

HEADERS += \
    clogdatasource.h \
    logviewmodel.h \
    logviewtable.h \
    logviewwidget.h \
    logviewselectionmodel.h \
    fileselector.h \
    iconhelper.h \
    fileselectorstandardmodel.h \
    fileselectorlocations.h \
    fileselectorlocationsenum.h \
    fileselectorlineedit.h \
    fileselectormodelitem.h \
    permissionhelper.h \
    stylehelper.h \
    ver.h \
    versionhelper.h
    
RESOURCES += \
    resources.qrc

DISTFILES += \
    .travis.yml \
    android/AndroidManifest.xml \
    android/gradle.properties \
    android/gradle.properties \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew \
    android/gradlew \
    android/gradlew.bat \
    android/gradlew.bat \
    android/res/values/libs.xml \
    android/build.gradle \
    android/res/mipmap-hdpi/ic_launcher_round.png \
    android/res/mipmap-hdpi/ic_launcher.png \
    android/res/mipmap-mdpi/ic_launcher_round.png \
    android/res/mipmap-mdpi/ic_launcher.png \
    android/res/mipmap-xhdpi/ic_launcher_round.png \
    android/res/mipmap-xhdpi/ic_launcher.png \
    android/res/mipmap-xxhdpi/ic_launcher_round.png \
    android/res/mipmap-xxhdpi/ic_launcher.png \
    android/res/mipmap-xxxhdpi/ic_launcher_round.png \
    android/res/mipmap-xxxhdpi/ic_launcher.png \
    android/ic_launcher-web.png \
    README.md \
    build.sh \
    debian/compat \
    debian/control \
    debian/copyright \
    debian/changelog \
    debian/rules \
    debian/source/format \
    debian/logview.install \
    data/logview.desktop \
    ISSUE_TEMPLATE.md \
    LICENSE \
    appveyor.yml \
    version.ps1 \
    installer.ps1 \
    build.cmd
    
win32 {
  RC_FILE     += logview.rc
  OTHER_FILES += logview.rc
}


# lupdate -no-obsolete -verbose -pro *.pro
# cd translations
# linguist *.ts
# cd ..
# lrelease -removeidentical -compress *.pro


#prepare for spell checker
#
# xmlstarlet sel -t -v "//translation[not(@type)]"  language_en.ts > language_en.txt
# xmlstarlet sel -t -v "//translation[not(@type)]"  language_cs.ts > language_cs.txt
# xmlstarlet sel -t -v "//translation[not(@type)]"  language_de.ts > language_de.txt
# xmlstarlet sel -t -v "//translation[not(@type)]"  language_fr.ts > language_fr.txt
# xmlstarlet sel -t -v "//translation[not(@type)]"  language_ja.ts > language_ja.txt
# xmlstarlet sel -t -v "//translation[not(@type)]"  language_pl.ts > language_pl.txt
# xmlstarlet sel -t -v "//translation[not(@type)]"  language_ru.ts > language_ru.txt
# xmlstarlet sel -t -v "//translation[not(@type)]"  language_sl.ts > language_sl.txt
# xmlstarlet sel -t -v "//translation[not(@type)]"  language_zh_CN.ts > language_zh_CN.txt
# xmlstarlet sel -t -v "//translation[not(@type)]"  language_zh_TW.ts > language_zh_TW.txt

TRANSLATIONS += \
    translations/language_en.ts \
    translations/language_cs.ts \
    translations/language_de.ts \
    translations/language_fr.ts \
    translations/language_ja.ts \
    translations/language_pl.ts \
    translations/language_ru.ts \
    translations/language_sl.ts \
    translations/language_zh_CN.ts \
    translations/language_zh_TW.ts \
    translations/language_es.ts

qtPrepareTool(LRELEASE, lrelease)
for(tsfile, TRANSLATIONS) {
    qmfile = $$shadowed($$tsfile)
    qmfile ~= s,.ts$,.qm,
    qmdir = $$dirname(qmfile)

    !exists($$qmdir) {
        mkpath($$qmdir)|error("Aborting.")
    }

    command = $$LRELEASE -compress -nounfinished -removeidentical $$tsfile -qm $$qmfile
    system($$command)|error("Failed to run: $$command")
}

unix:!macx {
    isEmpty(PREFIX) {
        PREFIX=/usr
    }

    target.path = $${PREFIX}/bin/

    hicolor.path = $${PREFIX}/share/icons/hicolor/
    hicolor.files = data/hicolor/*

    desktopentry.path = $${PREFIX}/share/applications/
    desktopentry.files = data/$${TARGET}.desktop

    INSTALLS += target hicolor desktopentry
}

#af		Afrikaans
#sq		Albanian
#ar		Arabic
#eu		Basque
#be		Belarusian
#bs		Bosnian
#bg		Bulgarian
#ca		Catalan
#hr		Croatian
#zh_cn		Chinese (Simplified)
#zh_tw		Chinese (Traditional)
#cs		Czech
#da		Danish
#nl		Dutch
#en		English
#en_us		English (US)
#et		Estonian
#fa		Farsi
#fil		Filipino
#fi		Finnish
#fr		French
#fr_ca		French (Canada)
#ga		Gaelic
#gl		Gallego
#ka		Georgian
#de		German
#de_du		German (Personal)
#el		Greek
#gu		Gujarati
#he		Hebrew
#hi		Hindi
#hu		Hungarian
#is		Icelandic
#id		Indonesian
#it		Italian
#ja		Japanese
#kn		Kannada
#km		Khmer
#ko		Korean
#lo		Lao
#lt		Lithuanian
#lv		Latvian
#ml		Malayalam
#ms		Malaysian
#mi_tn		Maori (Ngai Tahu)
#mi_wwow	Maori (Waikoto Uni)
#mn		Mongolian
#no		Norwegian
#no_gr		Norwegian (Primary)
#nn		Nynorsk
#pl		Polish
#pt		Portuguese
#pt_br		Portuguese (Brazil)
#ro		Romanian
#ru		Russian
#sm		Samoan
#sr		Serbian
#sk		Slovak
#sl		Slovenian
#so		Somali
#es		Spanish (International)
#sv		Swedish
#tl		Tagalog
#ta		Tamil
#th		Thai
#to		Tongan
#tr		Turkish
#uk		Ukrainian
#vi		Vietnamese
