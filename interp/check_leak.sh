#!/bin/sh

valgrind --leak-check=full ./sli_tcc test.sl
