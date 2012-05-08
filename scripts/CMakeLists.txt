
IF (WIN32)
    install( FILES
            #find-gui-text.sh TODO
            newprojectwizard_win.rc
            newprojectwizard_win.py
            newprojectwizard.py
            DESTINATION  ${DATA_INSTALL_DIR}/lokalize/scripts )
ELSE (WIN32)
    install( FILES
            newprojectwizard.rc
            newprojectwizard.py
            DESTINATION  ${DATA_INSTALL_DIR}/lokalize/scripts )

    install( PROGRAMS find-gui-text.sh
             DESTINATION  ${DATA_INSTALL_DIR}/lokalize/scripts )

ENDIF (WIN32)

install( FILES
        msgmerge.py
        msgmerge.rc
        DESTINATION  ${DATA_INSTALL_DIR}/lokalize/scripts )
install( FILES
        xliff2odf.py
        xliff2odf.rc
        xliff2odf-standalone.py
        xliffmerge.py
        DESTINATION  ${DATA_INSTALL_DIR}/lokalize/scripts/odf )
