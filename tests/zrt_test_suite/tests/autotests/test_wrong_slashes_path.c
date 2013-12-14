/*
 * normalizing wrong path testing
 *
 * Copyright (c) 2013, LiteStack, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#define SLASHES_PATH "//wrng"
#define OK_PATH "/wrng"

int main(int argc, char **argv)
{
    struct stat st;
    int res;
    int fd = open(SLASHES_PATH, O_CREAT|O_RDWR);
    close(fd);
    res = stat(OK_PATH, &st);
    return res;
}
