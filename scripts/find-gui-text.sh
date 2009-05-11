TEXT=$1
PACKAGE=$2
LOKALIZE_INSTANCES=`qdbus org.kde.lokalize*`
if [ -z $LOKALIZE_INSTANCES ]; then lokalize>/dev/null 2>/dev/null & fi

let COUNTER=0
while [ -z $LOKALIZE_INSTANCES ]; do \
let COUNTER=$COUNTER+1
if [ $COUNTER -gt 10 ]; then exit ; fi
sleep 1
LOKALIZE_INSTANCES=`qdbus org.kde.lokalize*` ;\
done

for LOKALIZE_INSTANCE in $LOKALIZE_INSTANCES; do \
    qdbus $LOKALIZE_INSTANCE /ThisIsWhatYouWant org.kde.Lokalize.MainWindow.showTranslationMemory; \
    qdbus $LOKALIZE_INSTANCE /ThisIsWhatYouWant/TranslationMemory/0 org.kde.Lokalize.TranslationMemory.findGuiTextPackage "$TEXT" "$PACKAGE"; \
done

