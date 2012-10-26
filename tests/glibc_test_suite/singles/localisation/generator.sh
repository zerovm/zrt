#!/bin/bash

#it's parses alias file and based on it's data
#it prints commands to invoke in terminal to compile
#example of single locale of output:
#localedef -i hr_HR -f ISO-8859-2 ./hr_HR.ISO-8859-2
awk '{ \
  #skip comments #... \
  if ( $0 !~ /#/ && NF==2) {
    split($2, params, ".");\
    print "localedef -i ",params[1], "-f", params[2], "./localedata/"$2 \
  }\
  
}' share/locale.alias

