#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Vishesh <visheshkratos@gmail.com>
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
# A basic test of opening the ui and closing the app.
import unittest
import time

from test_support import create_driver, dismiss_project_prompt


class QuickOpenClose(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.driver, cls.config_dir = create_driver()

    def test_open(self) -> None:
        dismiss_project_prompt(self.driver)
        print("Opened app")
        time.sleep(2)

    @classmethod
    def tearDownClass(cls) -> None:
        try:
            cls.driver.quit()
        except Exception:
            pass
        cls.config_dir.cleanup()


if __name__ == "__main__":
    unittest.main()
