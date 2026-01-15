# README for Developer

```bash
cd build && make

cd build/apps && ./dotlin ../../hello.lin
cd build/apps && ./dotlin ../../test_vars.lin
rm -rf build && mkdir build && cd build && cmake .. && make && ./apps/dotlin ..
/tests/files/datetime_test.lin
```