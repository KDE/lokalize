install( FILES
        find-gui-text.sh
        new-project-wizard.py
        new-project-wizard.rc
        DESTINATION  ${DATA_INSTALL_DIR}/lokalize/scripts )
install( FILES
        widget-text-capture.py
        widget-text-capture.rc
        widget-text-capture.ui
        msgmerge.py
        msgmerge.rc
        DESTINATION  ${DATA_INSTALL_DIR}/lokalize/scripts/kde )
install( FILES
        xliff2odf.py
        xliff2odf.rc
        xliff2odf-standalone.py
        DESTINATION  ${DATA_INSTALL_DIR}/lokalize/scripts/odf )
