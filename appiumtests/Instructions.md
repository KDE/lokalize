# To run all test:

```bash
kde-builder --run-tests lokalize --no-include-dependencies --no-src --cmake-options="-DBUILD_APPIUM_TESTS=ON"     
```
## If you want to run the tests manually: 
Installations required:
- selenium-webdriver-at-spi
- install using 
```bash
kde-builder selenium-webdriver-at-spi 
```
Instructions to run tests manually:
```bash
kde-builder --run selenium-webdriver-at-spi-run python3 workflowtest.py
```

You can replace the file name with test you want to run. It is possible to run multiple tests at once in a single file. 

# To write new tests

1. Create a new Python test file in this directory.
2. Start from one of the existing test classes and keep the same structure:
   - `setUpClass()` creates the driver.
   - one or more `test_` methods perform the UI actions.
   - `tearDownClass()` quits the driver and cleans temporary state.
3. Use `test_support.py` instead of duplicating setup code.
4. Add the new test case to `run_all.py` so it becomes part of the full test suite.

## Helper files

`test_support.py` already contains helper functions:

- `create_driver(...)`
  - starts Lokalize through Appium,
  - creates a temporary config directory,
  - writes a minimal `lokalizerc`,
  - optionally opens `test.po`, or accepts a custom `app_command`.
- `dismiss_project_prompt(driver, timeout=2)`
  - useful for tests that start Lokalize without a project and need to close the initial prompt.
- `TEST_FILE`
  - points to `test.po` in this directory.

## Adding the test to `run_all.py`

After the new file works on its own:

1. Import the new test class at the top of `run_all.py`.
2. Add it inside `add_test()` with `loader.loadTestsFromTestCase(...)`.

