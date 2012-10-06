#!/bin/sh
# $FreeBSD: src/tools/regression/fstest/tests/mkdir/07.t,v 1.1 2007/01/17 01:42:09 pjd Exp $

desc="mkdir returns ELOOP if too many symbolic links were encountered in translating the pathname"

dir=`dirname $0`
. ${dir}/../misc.sh

if [ "$fs" != "zrtfs" ]
then
    echo "1..6"
else
    quick_exit
fi

n0=`namegen`
n1=`namegen`

if [ "$fs" != "zrtfs" ] #symlink does not supported by zrtfs; excluded all 6 tests
then
    expect 0 symlink ${n0} ${n1}
    expect 0 symlink ${n1} ${n0}
    expect ELOOP mkdir ${n0}/test 0755
    expect ELOOP mkdir ${n1}/test 0755
    expect 0 unlink ${n0}
    expect 0 unlink ${n1}
fi    
