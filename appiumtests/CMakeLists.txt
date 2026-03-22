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

    add_test(
        NAME appium_run_all
        COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/run_all.py
    )
    set_tests_properties(appium_run_all PROPERTIES TIMEOUT 300)
endif()
