#!/bin/bash

install -v -D -m 0644 zlib.h ${ZRT_ROOT}/include/zlib.h
install -v -D -m 0644 zconf.h ${ZRT_ROOT}/include/zconf.h
install -v -D -m 0644 zlibdefs.h ${ZRT_ROOT}/include/zlibdefs.h
install -v -D -m 0644 libz.a ${ZRT_ROOT}/lib/libz.a

