#!/bin/bash
#It's dangerous to your files to make changes here
echo removing files listed below

find -name "*.nexe"
find -name "*.nexe" | xargs rm -f

find -name "*.a"
find -name "*.a" | xargs rm -f

find -name "*.o"
find -name "*.o" | xargs rm -f

find -name "*~"
find -name "*~" | xargs rm -f

find -name "*.log"
find -name "*.log" | xargs rm -f

find -name "*.std"
find -name "*.std" | xargs rm -f

find -name "*.log"
find -name "*.log" | xargs rm -f

echo cleanup completed