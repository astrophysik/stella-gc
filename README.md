# stella-gc

## Build and Tests

Building the project requires
```
- compiler C11
- GNU Make
- Python 3
- docker
```


If the conditions are met, the following commands are available.

```sh
make test          # build and run all tests
make test-c        # tests on C
make test-stella   # Stella -> C -> exe -> verification of the result
make all          # build tests files in build
make clean        # clean build/
```
