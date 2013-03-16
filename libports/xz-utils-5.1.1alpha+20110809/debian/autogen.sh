#!/bin/sh
# Generate debian/changelog.upstream, debian/generated-m4.list,
# and debian/generated-po.list.
#
# Uses debian/changelog, the git revision log, and .gitignore
# files from the current checkout.

set -e

changelog_needs_update() {
	test -e debian/changelog.upstream &&
	read line < debian/changelog.upstream ||
	return 0

	ver=${line#Version } &&
	ver=${ver%;*} &&
	test "$ver" != "" ||
	return 0

	read line < debian/changelog &&
	rhs=${line#*(} &&
	deb_ver=${rhs%)*} &&
	new_ver=${deb_ver%-*} ||
	return 0

	test "$ver" != "$new_ver"
}

cp -f m4/.gitignore debian/generated-m4.list
cp -f po/.gitignore debian/generated-po.list
sed -n 's,^build-aux/,, p' .gitignore > debian/generated-build-aux.list
! changelog_needs_update || exec sh debian/changelog.upstream.sh
