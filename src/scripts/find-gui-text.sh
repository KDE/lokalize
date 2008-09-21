TEXT=$1
LOKALIZE_INSTANCES=`qdbus org.kde.lokalize*`
for LOKALIZE_INSTANCE in $LOKALIZE_INSTANCES; do \
qdbus $LOKALIZE_INSTANCE /ThisIsWhatYouWant org.kde.Lokalize.MainWindow.showTranslationMemory; \
qdbus $LOKALIZE_INSTANCE /ThisIsWhatYouWant/TranslationMemory/0 org.kde.Lokalize.TranslationMemory.findGuiText "$TEXT"; \
done
