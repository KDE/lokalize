#! /usr/bin/env bash
$EXTRACTRC `find . -name \*.ui -o -name \*.rc -o -name \*.kcfg` >> rc.cpp
$EXTRACTATTR --attr=collection,text --attr=collection,comment --attr=script,text --attr=script,comment scripts/*.rc >> rc.cpp || exit 11
$XGETTEXT *.cpp \
	    catalog/*.cpp \
	    cataloglistview/*.cpp \
	    common/*.cpp \
	    glossary/*.cpp \
	    mergemode/*.cpp \
	    prefs/*.cpp \
	    project/*.cpp \
	    tm/*.cpp \
	    webquery/*.cpp \
	    -o $podir/lokalize.pot
rm -f rc.cpp
$XGETTEXT --language=Python --join-existing scripts/*.py -o $podir/lokalize.pot
