# SPDX-FileCopyrightText: 2026 Vishesh <visheshkratos@gmail.com>
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    find_package(SeleniumWebDriverATSPI)
    set_package_properties(SeleniumWebDriverATSPI PROPERTIES
        DESCRIPTION "Selenium webdriver"
        PURPOSE "Needed for appium test"
        URL "https://invent.kde.org/sdk/selenium-webdriver-at-spi"
        TYPE OPTIONAL
    )

    file(GLOB APPIUM_TESTS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.py)
    list(REMOVE_ITEM APPIUM_TESTS test_support.py)

    foreach(appium_test ${APPIUM_TESTS})
        get_filename_component(test_name ${appium_test} NAME_WE)
        add_test(
            NAME appium_${test_name}
            COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/${appium_test}
        )
        set_tests_properties(appium_${test_name} PROPERTIES TIMEOUT 300)
    endforeach()
endif()
