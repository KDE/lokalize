#! /usr/bin/env bash
$EXTRACTRC `find . -name \*.ui -o -name \*.rc -o -name \*.kcfg` >> rc.cpp
$EXTRACTATTR --attr=collection,text --attr=collection,comment --attr=script,text --attr=script,comment scripts/*.rc >> rc.cpp || exit 11
$XGETTEXT rc.cpp src/*.cpp \
	    src/catalog/*.cpp \
	    src/cataloglistview/*.cpp \
	    src/common/*.cpp \
	    src/glossary/*.cpp \
	    src/mergemode/*.cpp \
	    src/prefs/*.cpp \
	    src/project/*.cpp \
	    src/tm/*.cpp \
	    src/webquery/*.cpp \
	    -o $podir/lokalize.pot
rm -f rc.cpp
$XGETTEXT --language=Python --join-existing scripts/*.py -o $podir/lokalize.pot
