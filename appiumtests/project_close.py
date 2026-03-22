#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 Vishesh <visheshkratos@gmail.com>
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import unittest
from pathlib import Path

from appium.webdriver.common.appiumby import AppiumBy
from selenium.webdriver.support.ui import WebDriverWait

from test_support import create_driver

PROJECT_FILE = Path(__file__).with_name("index.lokalize").resolve()

class CloseProjectDisablesActions(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        if not PROJECT_FILE.exists():
            raise unittest.SkipTest(f"Missing project file: {PROJECT_FILE}")
        app_command = f"lokalize {str(PROJECT_FILE)}"
        cls.driver, cls.config_dir = create_driver(app_command=app_command)
        cls.wait = WebDriverWait(cls.driver, 20)

    @classmethod
    def tearDownClass(cls) -> None:
        try:
            cls.driver.quit()
        except Exception:
            pass
        cls.config_dir.cleanup()

    def test_close_project_disables_editor_menus(self) -> None:
        project_menu = self.wait.until(
            lambda d: d.find_element(AppiumBy.NAME, "Project")
        )
        project_menu.click()

        close_project = self.wait.until(
            lambda d: d.find_element(AppiumBy.NAME, "Close Project")
        )
        close_project.click()

        edit_menu = self.wait.until(lambda d: d.find_element(AppiumBy.NAME, "Edit"))
        go_menu = self.wait.until(lambda d: d.find_element(AppiumBy.NAME, "Go"))
        sync_menu = self.wait.until(lambda d: d.find_element(AppiumBy.NAME, "Sync"))

        self.assertFalse(edit_menu.is_enabled())
        self.assertFalse(go_menu.is_enabled())
        self.assertFalse(sync_menu.is_enabled())


if __name__ == "__main__":
    unittest.main()
