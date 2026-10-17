.PHONY: test-python test-cpp test run-all

test-python:
	pytest tests/python/test_config.py

test-cpp:
	g++ -o tests/cpp/test_protocol tests/cpp/test_protocol.cpp
	./tests/cpp/test_protocol

run-all: test-cpp test-python
	@echo "All tests passed!"

test: run-all
