#!/bin/sh
# $FreeBSD: src/tools/regression/fstest/tests/chmod/10.t,v 1.1 2007/01/17 01:42:08 pjd Exp $

desc="chmod returns EFAULT if the path argument points outside the process's allocated address space"

dir=`dirname $0`
. ${dir}/../misc.sh

if [ "${fs}" != "zrtfs" ]
then
    echo "1..2"
else
    echo "1..1"
fi    

expect EFAULT chmod NULL 0644
if [ "${fs}" != "zrtfs" ]
then
    expect EFAULT chmod DEADCODE 0644
fi    
