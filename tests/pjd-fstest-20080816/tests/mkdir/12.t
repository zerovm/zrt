#!/bin/sh
# $FreeBSD: src/tools/regression/fstest/tests/mkdir/12.t,v 1.1 2007/01/17 01:42:09 pjd Exp $

desc="mkdir returns EFAULT if the path argument points outside the process's allocated address space"

dir=`dirname $0`
. ${dir}/../misc.sh

if [ "$fs" != "zrtfs" ]
then
    echo "1..2"
else
    echo "1..1"
fi    

expect EFAULT mkdir NULL 0755
if [ "$fs" != "zrtfs" ] #zrt has no API to check invalid pointer; 1 test excluded
then
    
    expect EFAULT mkdir DEADCODE 0755
fi
    

