#!/bin/bash

#The main of prerequisites that must be pre-installed is a Google Native Client SDK. 
#You must setup path of nacl glibc that part of NACL SDK. This script would be used by ZRT project Makefiles.
#If version of SDK has newer version of pepper, set it accordingly.
echo ${HOME}/nacl_sdk/pepper_19/toolchain/linux_x86_glibc

