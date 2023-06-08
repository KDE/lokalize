
if(WIN32)
#    install( FILES
#            #find-gui-text.sh TODO
#            DESTINATION  ${KDE_INSTALL_DATADIR}/lokalize/scripts )
else()
    install( PROGRAMS find-gui-text.sh
             DESTINATION  ${KDE_INSTALL_DATADIR}/lokalize/scripts )
endif()

install( FILES
        msgmerge.py
        msgmerge.rc
        DESTINATION  ${KDE_INSTALL_DATADIR}/lokalize/scripts )
install( FILES
        xliff2odf.py
        xliff2odf.rc
        xliff2odf-standalone.py
        xliffmerge.py
        DESTINATION  ${KDE_INSTALL_DATADIR}/lokalize/scripts/odf )
