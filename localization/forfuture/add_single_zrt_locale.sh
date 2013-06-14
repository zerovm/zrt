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
ADD_LOCALES_FOLDER=localedef
LOCALE_PATH=$ADD_LOCALES_FOLDER/$LOCALE_NAME


#create dir for tar locales
mkdir -p $MOUNTS_LOCALE_FOLDER

cd $ADD_LOCALES_FOLDER
#compile and add specific locale into archive & create LC_TYPE* files
LANG_TERRITORY=$1 CHARMAP=$2 sh locale_create.sh

#move localisation single archive into locales folder
if [ -f $LOCALE_PATH.tar ]
then
    echo mv $LOCALE_PATH.tar $MOUNTS_LOCALE_FOLDER
    mv $LOCALE_PATH.tar $MOUNTS_LOCALE_FOLDER
    echo locale $LOCALE_PATH compiled.
fi




