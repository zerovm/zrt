#!/bin/sh
# $FreeBSD: src/tools/regression/fstest/tests/open/21.t,v 1.1 2007/01/17 01:42:10 pjd Exp $

desc="open returns EFAULT if the path argument points outside the process's allocated address space"

dir=`dirname $0`
. ${dir}/../misc.sh

if [ "${fs}" != "zrtfs" ]
then
    echo "1..2"
else
    echo "1..1"
fi    

if [ "${fs}" != "zrtfs" ]
then
    expect EFAULT open NULL O_RDONLY
    expect EFAULT open DEADCODE O_RDONLY
else
    #zrtfs returning errno=EFAULT overwriten in further to EPERM in unexpected way
    expect EPERM open NULL O_RDONLY
fi    

