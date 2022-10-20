# charm_spectre_tests
Tests for charm++ that illustrate failures encountered when developing spectre

To reproduce the issue:
```
git clone https://github.com/UIUC-PPL/charm.git

./build LIBS multicore-linux-x86_64 --build-shared -g -O0

cd tests/charm++/

git clone https://github.com/kidder/charm_spectre_tests.git

cd charm_spectre_tests/TEST_NAME

make test
```

where TEST_NAME is one of the following:

dynamic_array_expansion:
  test will fail on develop, but pass for version 7.0.0.  Appears to be broken
  by PR #3614

dynamic_array_hang:
  test will hang on version 7.0.0.  On develop test fails because of
  dynamic_array_expansion failure prior to the point where v7.0.0 hangs

