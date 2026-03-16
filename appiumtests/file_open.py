#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Vishesh <visheshkratos@gmail.com>
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

# This test aims to open the file choosing dialog of the app, waits for it to render and then quits

import unittest
import time

from appium.webdriver.common.appiumby import AppiumBy
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC

from test_support import create_driver, dismiss_project_prompt


class LokalizeFileOpen(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.driver, cls.config_dir = create_driver()
        cls.wait = WebDriverWait(cls.driver, 10)

    def test_file_open(self) -> None:
        dismiss_project_prompt(self.driver)
        file_menu = self.wait.until(
            EC.presence_of_element_located((AppiumBy.NAME, "File"))
        )
        file_menu.click()
        time.sleep(1)

        open_item = self.wait.until(
            EC.presence_of_element_located((AppiumBy.NAME, "Open…"))
        )
        open_item.click()

        time.sleep(3)
        print("opening file dialog done")

    @classmethod
    def tearDownClass(cls) -> None:
        try:
            cls.driver.quit()
        except Exception:
            pass
        cls.config_dir.cleanup()


if __name__ == "__main__":
    unittest.main()
