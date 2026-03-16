#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 Vishesh <visheshkratos@gmail.com>
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import shlex
import tempfile
import time
from pathlib import Path

from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy
from selenium.webdriver.support.ui import WebDriverWait


TEST_FILE = Path(__file__).with_name("test.po").resolve()


def create_driver(*, open_test_file=False):
    config_dir = tempfile.TemporaryDirectory(prefix="lokalize-appium-")
    config_home = Path(config_dir.name)
    config_home.joinpath("lokalizerc").write_text(
        "[Identity]\n"
        "authorName=Test\n"
        "authorLocalizedName=Test\n"
        "authorEmail=test@test.com\n",
        encoding="utf-8",
    )

    options = AppiumOptions()
    app = "lokalize"
    if open_test_file:
        app = f"{app} {shlex.quote(str(TEST_FILE))}"
    options.set_capability("app", app)
    options.set_capability("appium:environ", {"XDG_CONFIG_HOME": str(config_home)})

    driver = webdriver.Remote(
        command_executor="http://127.0.0.1:4723",
        options=options,
    )
    return driver, config_dir


def dismiss_project_prompt(driver, timeout=2):
    try:
        short_wait = WebDriverWait(driver, timeout)
        short_wait.until(lambda d: d.find_element(AppiumBy.NAME, "Open Project…"))
        driver.find_element(AppiumBy.NAME, "Cancel").click()
        time.sleep(0.5)
    except Exception:
        pass
