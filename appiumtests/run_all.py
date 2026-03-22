#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 Vishesh <visheshkratos@gmail.com>
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import sys
import unittest

from file_open import LokalizeFileOpen
from project_close import CloseProjectDisablesActions
from simple_open import QuickOpenClose
from workflowtest import LokalizeEditorWorkflow

def add_test():
    loader = unittest.TestLoader()
    test_list = unittest.TestSuite()
    test_list.addTests(loader.loadTestsFromTestCase(QuickOpenClose))
    test_list.addTests(loader.loadTestsFromTestCase(LokalizeFileOpen))
    test_list.addTests(loader.loadTestsFromTestCase(LokalizeEditorWorkflow))
    test_list.addTests(loader.loadTestsFromTestCase(CloseProjectDisablesActions))
    return test_list

if __name__ == "__main__":
    result = unittest.TextTestRunner(verbosity=2).run(add_test())
    if not result.wasSuccessful():
        failed_tests = [case.id() for case, _ in result.failures + result.errors]
        if failed_tests:
            print("\nFailed tests:")
            for test_id in failed_tests:
                print(f"- {test_id}")
        sys.exit(1)
