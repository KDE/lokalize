
IF (WIN32)
#    install( FILES
#            #find-gui-text.sh TODO
#            DESTINATION  ${KDE_INSTALL_DATADIR}/lokalize/scripts )
ELSE (WIN32)
    install( PROGRAMS find-gui-text.sh
             DESTINATION  ${KDE_INSTALL_DATADIR}/lokalize/scripts )

ENDIF (WIN32)

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
