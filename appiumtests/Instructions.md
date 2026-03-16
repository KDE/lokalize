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
Apps like dolphin and kcalc run it using selenium-webdriver-at-spi-run command, however I preferred to use kde-builder to run the tests.

The workflowtest.py now takes an untranslated po file and translates it, checks if the translation updates correctly and then after finishing checks if the number of translated entries in the ui update correctly.
