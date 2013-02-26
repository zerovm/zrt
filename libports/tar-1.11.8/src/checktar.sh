# Check if tar appears to works correctly.
# Copyright (C) 1995 Free Software Foundation, Inc.
# François Pinard <pinard@iro.umontreal.ca>, 1995.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

# Exit on all errors.
set -e

## ----------------- ##
## Initial cleanup.  ##
## ----------------- ##

rm -rf checkdir
mkdir checkdir
cd checkdir

echo "Checking `tar --version`"

## --------------- ##
## No checks yet.  ##
## --------------- ##

echo "Useless try: no checking implemented yet"

## --------------- ##
## Final cleanup.  ##
## --------------- ##

echo "Check successful"

cd ..
rm -rf checkdir
exit 0
