#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Vishesh <visheshkratos@gmail.com>
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

# Full workflow test: open a file, write two translation entries,
# approve both, verify the status bar, and save.

import shlex
import time
import unittest
from pathlib import Path

from appium.webdriver.common.appiumby import AppiumBy
from selenium.webdriver.support.ui import WebDriverWait

from test_support import TEST_FILE, create_driver

PROJECT_FILE = Path(__file__).with_name("index.lokalize").resolve()

class LokalizeEditorWorkflow(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.original_test_file = TEST_FILE.read_text(encoding="utf-8")
        if not PROJECT_FILE.exists():
            raise unittest.SkipTest(f"Missing project file: {PROJECT_FILE}")
        app_command = (
            f"lokalize --project {shlex.quote(str(PROJECT_FILE))} "
            f"{shlex.quote(str(TEST_FILE))}"
        )
        cls.driver, cls.config_dir = create_driver(app_command=app_command)
        cls.wait = WebDriverWait(cls.driver, 30)

    @classmethod
    def tearDownClass(cls) -> None:
        try:
            cls.driver.quit()
        except Exception:
            pass
        TEST_FILE.write_text(cls.original_test_file, encoding="utf-8")
        cls.config_dir.cleanup()

    def find_target_field(self):
        return self.wait.until(
            lambda d: d.find_element(AppiumBy.ACCESSIBILITY_ID, "TargetTextEdit")
        )

    def type_translation(self, text):
        field = self.find_target_field()
        field.click()
        time.sleep(0.3)
        field.send_keys(text)
        return field

    def approve_and_go_next(self):
        edit_menu = self.wait.until(lambda d: d.find_element(AppiumBy.NAME, "Edit"))
        edit_menu.click()
        approve_next = self.wait.until(
            lambda d: d.find_element(AppiumBy.NAME, "Approve and Go Next")
        )
        approve_next.click()
        time.sleep(1)

    def get_status_text(self):
        labels = self.driver.find_elements(AppiumBy.XPATH, "//label")
        return " | ".join([l.text for l in labels if l.text])

    def test_workflow_two_entries(self):
        # Adding text and checking if the gui updated properly
        field = self.type_translation("Appium entry 1")
        self.assertEqual(field.text, "Appium entry 1")
        self.approve_and_go_next()

        field = self.type_translation("Appium entry 2")
        self.assertEqual(field.text, "Appium entry 2")
        self.approve_and_go_next()

        # Checking if both entries are approved now
        self.wait.until(lambda _: "Not ready: 0" in self.get_status_text())
        self.assertIn("Not ready: 0", self.get_status_text())

        file_menu = self.wait.until(lambda d: d.find_element(AppiumBy.NAME, "File"))
        file_menu.click()
        save_action = self.wait.until(
            lambda d: d.find_element(AppiumBy.ACCESSIBILITY_ID, "file_save")
        )
        save_action.click()


if __name__ == "__main__":
    unittest.main()
