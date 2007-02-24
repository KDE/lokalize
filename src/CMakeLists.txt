PROJECT(kaider)
INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR}/../common ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} )

set(kaider_SRCS
   main.cpp
   kaider.cpp
   kaider_findreplace.cpp
   kaider_prefs.cpp
   kaiderview.cpp
   ../common/catalog.cpp
   ../common/catalogitem.cpp
   ../common/importplugin.cpp
   ../common/exportplugin.cpp
   ../common/gettextimport.cpp
   ../common/gettextexport.cpp
   ../common/cmd.cpp
 )

kde4_automoc(${kaider_SRCS})

kde4_add_ui_files(kaider_SRCS prefs_identity.ui)

kde4_add_kcfg_files(kaider_SRCS settings.kcfgc )

kde4_add_executable(kaider ${kaider_SRCS})

target_link_libraries(kaider ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} )

install(TARGETS kaider DESTINATION ${BIN_INSTALL_DIR} )


########### install files ###############

install( FILES kaider.desktop  DESTINATION  ${XDG_APPS_DIR} )
install( FILES kaider.kcfg  DESTINATION  ${KCFG_INSTALL_DIR} )
install( FILES kaiderui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kaider )


