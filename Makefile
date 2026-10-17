.PHONY: test-python test-cpp test run-all clean

test-python:
	python3 tests/python/test_config.py

test-cpp:
	g++ -std=c++11 -Wall -Wextra -o tests/cpp/test_protocol tests/cpp/test_protocol.cpp
	./tests/cpp/test_protocol

clean:
	rm -f tests/cpp/test_protocol

run-all: test-cpp test-python
	@echo "All tests passed successfully!"

test: run-all

