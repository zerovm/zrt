#!/bin/sh
# Generate debian/changelog.upstream from debian/changelog and
# the git revision log.  Inspired by Gerrit Pape’s
# debian/changelog.upstream.sh, from the git-core Debian package.

set -e

# If argument matches /^Version: /, output remaining text.
# Result is true if and only if argument matches.
version_line() {
	local line result
	line=$1
	result=${line#Version: }

	if test "$result" = "$line"
	then
		return 1
	else
		printf "%s\n" "$result"
		return 0
	fi
}

# If argument matches /^\* New.*snapshot.*commit /,
# output remaining text.
# Result is true if and only if argument matches.
commit_id_line() {
	local line result
	line=$1
	result=${line#\* New*snapshot*commit }

	if test "$result" = "$line"
	then
		return 1
	else
		printf "%s\n" "$result"
		return 0
	fi
}

# Read standard input, scanning for a changelog entry of the
# form “New snapshot, taken from upstream commit <blah>.”
# Output is <blah>.
# Fails and writes a message to standard error if no such entry is
# found before the next Version: line with a different upstream
# version (or EOF).
# $1 is the upstream version sought.
read_commit_id() {
	local upstream_version line version cid
	upstream_version=$1

	while read line
	do
		if
			version=$(version_line "$line") &&
			test "${version%-*}" != "$upstream_version"
		then
			break
		fi

		if cid=$(commit_id_line "$line")
		then
			printf "%s\n" "${cid%.}"
			return 0
		fi
	done

	echo >&2 "No commit id for $upstream_version"
	return 1
}

last=none
last_cid=none
# Add a list of all revisions up to $last to debian/changelog.upstream
# and set last=$2.
# $1 is a user-readable name for the commit $2
add_version() {
	local new new_cid limiter
	new=$1
	new_cid=$2

	if test "$last" = none
	then
		: > debian/changelog.upstream
		last=$new
		last_cid=$new_cid
		return 0
	fi

	exec >> debian/changelog.upstream
	if test "$new" = none
	then
		echo "Version $last:"
		echo "Version $last:" | tr "[:print:]" -
		limiter=
	elif test "$new" = "$last"
	then
		return 0
	else
		echo "Version $last; changes since $new:"
		echo "Version $last; changes since $new:" | tr "[:print:]" -
		limiter="$new_cid.."
	fi
	echo
	git log --date=iso --stat --no-merges --format=medium "$limiter$last_cid"
	test "$new" = none || echo

	last=$new
	last_cid=$new_cid
}

dpkg-parsechangelog --format rfc822 --all | {
while read line
do
	if version=$(version_line "$line")
	then
		# strip Debian revision
		upstream_version=${version%-*}

		if git rev-parse --verify -q "v$upstream_version" > /dev/null
		then
			# upstream release
			add_version "$upstream_version" "v$upstream_version"
		else
			# snapshot
			cid=$(read_commit_id "$upstream_version") || exit 1
			add_version "$upstream_version" "$cid"
		fi
	fi
done
add_version none none
}
