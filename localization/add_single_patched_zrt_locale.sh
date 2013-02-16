#!/bin/bash

# it's script run compiler for single locale
# and prepare locale to be used inside of ZRT
# applying a patch to LC_CTYPE file

if [ $# -lt 2 ]
then
	echo "Args expected 1: language_territory 2:codeset, exam: de_DE UTF-8 "
	exit
fi

#LOCALES_SOURCE=~/nacl/nativeclient/native_client/tools/SRC/glibc/localedata/locales
#CHARMAPS_SOURCE=~/nacl/nativeclient/native_client/tools/SRC/glibc/localedata/charmaps
MOUNTS_LOCALE_FOLDER=../mounts/locales
LOCALE_NAME=$1.$2
ADD_LOCALES_FOLDER=lib/locale
LOCALE_PATH=$ADD_LOCALES_FOLDER/$LOCALE_NAME
LOCALE_PATCHER=locale_patcher/locale_patcher

#build patcher if that not yet exist
if [ ! -f $LOCALE_PATCHER ]
then
    echo error: you need build a patcher: $LOCALE_PATCHER 
    exit
fi

#create dir for tar locales
mkdir -p $MOUNTS_LOCALE_FOLDER
mkdir -p $ADD_LOCALES_FOLDER

#compile locale

if [ ! -f $ADD_LOCALES_FOLDER/$1.$2/LC_CTYPE ]
then
#    localedef -c -i $LOCALES_SOURCE/$1 -f $CHARMAPS_SOURCE/$2  $ADD_LOCALES_FOLDER/$1.$2
    localedef -c -i $1 -f $2  $ADD_LOCALES_FOLDER/$1.$2
#apply patch
    $LOCALE_PATCHER $ADD_LOCALES_FOLDER/$1.$2/LC_CTYPE
fi

#add localisation files related to single archive into archive
tar -cf $MOUNTS_LOCALE_FOLDER/$LOCALE_NAME.tar $LOCALE_PATH



