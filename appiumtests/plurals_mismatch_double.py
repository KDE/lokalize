#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 Finley Watson <fin-w@tutanota.com>
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import unittest
from pathlib import Path
from time import sleep

from appium.webdriver.common.appiumby import AppiumBy
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver import ActionChains
from selenium.webdriver.common.keys import Keys
from selenium.webdriver.remote.webelement import WebElement
from selenium.webdriver.support import expected_conditions as EC

from test_support import create_driver

PROJECT_FILE = Path(__file__).resolve().parent / "data" / "plurals-mismatch" / "double" / "index.lokalize"

class OpenDoublePluralWithTripleInBranch(unittest.TestCase):
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

    def test_double_plural_with_single_on_branch(self) -> None:
        # project_widget: WebElement = self.wait.until(
        #     lambda d: d.find_element(AppiumBy.ACCESSIBILITY_ID, "ProjectWidget")
        # )
        # project_widget.click()
        # sleep(2)

        file_menu = self.wait.until( EC.presence_of_element_located((AppiumBy.NAME, "File")))
        file_menu.click()
        open_item = self.wait.until( EC.presence_of_element_located((AppiumBy.NAME, "Open…")))
        open_item.click()


        sleep(1)
        open_file: WebElement = self.wait.until(lambda d: d.find_element(AppiumBy.NAME, "Open"))
        open_file.click()

        self.driver.find_element(
            AppiumBy.ACCESSIBILITY_ID,
            "ProjectWidget"
        ).send_keys("\n\n\n")
        sleep(100)


if __name__ == "__main__":
    unittest.main()






