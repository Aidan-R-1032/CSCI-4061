# CSCI 4061 Lab Makefile
AN=lab03

CFLAGS = -Wall -Werror -g
CC = gcc $(CFLAGS)
SHELL = /bin/bash
CWD = $(shell pwd | sed 's/.*\///g')

.PHONY: all clean test test-quiz test-code test-setup

all: fork_wait fork_exec

fork_wait: fork_wait.c
	$(CC) -o $@ $^

fork_exec: fork_exec.c
	$(CC) -o $@ $^

clean:
	rm -f fork_wait fork_exec

clean-tests:
	rm -rf test_results

help:
	@echo 'Typical usage is:'
	@echo '  > make                          # build all programs'
	@echo '  > make clean                    # remove all compiled items'
	@echo '  > make zip                      # create a zip file for submission'
	@echo '  > make test                     # run all tests'
	@echo '  > make test-quiz                # run tests that verify quiz answers'
	@echo '  > make test-code                # run tests that verify code is correct'
	@echo '  > make test-code testnum=3      # run only test #3'

# 'make zip' to create zip file for submission
zip: clean clean-tests
	rm -f $(AN)-code.zip
	cd .. && zip "$(CWD)/$(AN)-code.zip" -r "$(CWD)" -x "$(CWD)/test_cases/*" "$(CWD)/testius"
	@echo Zip created in $(AN)-code.zip
	@if (( $$(stat -c '%s' $(AN)-code.zip) > 10*(2**20) )); then echo "WARNING: $(AN)-code.zip seems REALLY big, check there are no abnormally large test files"; du -h $(AN)-code.zip; fi
	@if (( $$(unzip -t $(AN)-code.zip | wc -l) > 256 )); then echo "WARNING: $(AN)-code.zip has 256 or more files in it which may cause submission problems"; fi

ifdef testnum
test: test-setup all
	./testius test_cases/tests.json -v -n "$(testnum)"
else
test: test-setup all
	./testius test_cases/tests.json
endif

test-quiz: test-setup QUESTIONS.txt
	./testius test_cases/tests.json -v -n 1

test-code: test-setup all
	./testius test_cases/tests.json -n "2,3"

test-setup:
	@chmod u+x testius
	@chmod u+x test_cases/resources/socrates
	@chmod u+x test_cases/resources/test_order_pids
