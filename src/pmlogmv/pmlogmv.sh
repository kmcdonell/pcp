Note:
    Do not use this script ... it is the old version and is
    kept here simply for reference.  The C application works
    much better.
#!/bin/sh
# 
# Copyright (c) 1997,2003 Silicon Graphics, Inc.  All Rights Reserved.
# Copyright (c) 2013-2014 Red Hat.
# Copyright (c) 2014 Ken McDonell.  All Rights Reserved.
# 
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
# 
# Move (rename) a PCP archive.
#
# Operation is atomic across the multiple files of a PCP archive
#

. $PCP_DIR/etc/pcp.env

status=1
tmp=`mktemp -d "$PCP_TMPFILE_DIR/pmlogmv.XXXXXXXXX"` || exit 1
trap "rm -rf $tmp; exit \$status" 0
trap "_cleanup; rm -rf $tmp; exit \$status" 1 2 3 15
prog=`basename $0`

cat > $tmp/usage << EOF
# Usage: [options] oldname newname

Options:
  -N, --showme    perform a dry run, showing what would be done
  -V, --verbose   increase diagnostic verbosity
  --help
EOF

_usage()
{
    pmgetopt --progname=$prog --config=$tmp/usage --usage
    exit 1
}

verbose=false
very_verbose=false
showme=false
LN=ln
RM=rm
RMF="rm -f"

ARGS=`pmgetopt --progname=$prog --config=$tmp/usage -- "$@"`
[ $? != 0 ] && exit 1

eval set -- "$ARGS"
while [ $# -gt 0 ]
do
    case "$1"
    in
	-N)	# show me
	    showme=true
	    LN='echo >&2 + ln'
	    RM='echo >&2 + rm'
	    RMF='echo >&2 + rm -f'
	    ;;
	-V)	# verbose
	    if $verbose
	    then
		very_verbose=true
	    else
		verbose=true
	    fi
	    ;;
	--)
	    shift
	    break
	    ;;
	-\?)
	    _usage
	    # NOTREACHED
	    ;;
    esac
    shift
done

if [ $# -ne 2 ]
then
    _usage
    # NOTREACHED
fi

# need to handle compression suffixes at the end of a file name
#
eval `pmconfig -L -s compress_suffixes`
if [ -n "$compress_suffixes" ]
then
    pat=`echo $compress_suffixes | sed -e 's/\.//g' -e 's/ /|/g' -e 's/^/(/' -e 's/$/)/'`
else
    # fallback to a fixed list, as of PCP 4.0.1
    #
    pat='(xz|lzma|bz2|bz|gz|Z|z)'
fi

if [ -f "$1" ]
then
    # oldname is an existing file, strip any compression suffix,
    # then strip the expected PCP suffix
    #
    if echo "$1" | grep -E -q "\\.$pat\$"
    then
	base=`echo "$1" | sed -e 's/\.[a-zA-Z][a-zA-Z0-9]*$//'`
    else
	base="$1"
    fi
    case "$base"
    in
	*[0-9])
	    old=`echo "$base" | sed -e 's/\.[0-9][0-9]*$//'`
	    ;;
	*.index|*.meta)
	    old=`echo "$base" | sed -e 's/\.[a-z][a-z]*$//'`
	    ;;
	*)
	    echo >&2 "$prog: Error: oldname argument ($1) is not a PCP archive"
	    exit
	    ;;
    esac
    $verbose && echo "strip archive suffix: $1 -> $old"
else
    old="$1"
fi
new="$2"
# don't even try if there of sh(1) special characters in the destination
# name ...
#
if echo "$new" | grep '[<>;|&]' >/dev/null
then
    echo >&2 "$prog: Error: newname cannot contain '<', '>', ';', '|' or '&'"
    exit
fi
# and escape the glob special characters
#
new=`echo "$2" | sed -e 's/\([?*[]\)/\\\\\1/g'`

_get_part()
{
    fname="$1"
    xtra=''
    for suffix in `echo "$pat" | sed -e 's/^(//' -e 's/)$//' -e 's/|/ /g'`
    do
	if echo "$fname" | grep "\.$suffix\$" >/dev/null
	then
	    xtra=".$suffix"
	    fname=`echo "$fname" | sed -e "s/\.$suffix\$//"`
	    break
	fi
    done
    part=`echo "$fname" | sed -e 's/.*\.\([^.][^.]*\)$/\1/'`$xtra
    $very_verbose && echo "fname=$fname -> part=$part"
}

_cleanup()
{
    if [ -f $tmp/old ]
    then
	for f in `cat $tmp/old`
	do
	    if eval [ ! -f "$f" ]
	    then
		_get_part "$f"
		if [ ! -f "$new.$part" ]
		then
		    echo >&2 "$prog: Fatal: $f and $new.$part lost"
		    eval ls -l "$old_noglob".* "$new".*
		    rm -f $tmp/"$old".*
		    return
		fi
		$verbose && echo >&2 "cleanup: recover $f from $new.$part"
		if eval $LN "$new.$part" "$f"
		then
		    :
		else
		    echo >&2 "$prog: Fatal: ln $new.$part $f failed!"
		    eval ls -l "$old_noglob".* "$new".*
		    rm -f $tmp/"$old".*
		    return
		fi
	    fi
	done
    fi
    if [ -f $tmp/new ]
    then
	for f in `cat $tmp/new`
	do
	    $verbose && echo >&2 "cleanup: remove $f"
	    eval $RMF "$f"
	done
	rm -f $tmp/new
    fi
    exit
}

# count hard links to a file
#
_count_links()
{
    case "$PCP_PLATFORM"
    in
	# for *BSD-base systems with a -f option to stat(1)
	#
	darwin|freebsd|netbsd|openbsd)
	    stat -f '%l' "$1"
	    ;;
	# for Linux systems with a -c option to stat(1)
	#
	*)
	    stat -c '%h' "$1"
	    ;;
    esac
}

# Get oldnames inventory and check required files are present
# ... be very careful here, if the ls expands $old.* into a filename
# that contains _any_ egrep(1) regexp special characters we have to
# escape 'em ... that's what $old_noregex is for.
#
# Similarty we have to be careful of sh(1) glob characters in
# $old ... that's what $old_noglob is for, and we use this in
# lots of places with eval(1) to ensure they are handled correctly.
#
old_noregex=`echo "$old" | sed -e 's/\([?+*{\.^$|([]\)/\\\\\1/g'`
$very_verbose && echo "old_noregex=$old_noregex"
old_noglob=`echo "$old" | sed -e 's/\([?*[]\)/\\\\\1/g'`
$very_verbose && echo "old_noglob=$old_noglob"

eval ls "$old_noglob".* 2>&1 \
| grep -E "$old_noregex"'\.((index|meta|[0-9][0-9]*)|((index|meta|[0-9][0-9]*)\.'"$pat"'))$' \
| sed -e 's/\([?*$[]\)/\\\1/g' >$tmp/old
if [ -s $tmp/old ]
then
    # $old may be an ambiguous suffix, e.g. 20140417.00 (with more than
    # one .HH archives) ... pick the suffixes and make sure there are
    # no duplicates
    #
    touch $tmp/ok
    cat $tmp/old \
    | while read fname
    do
	if echo "$fname" | grep -E -q "\\.$pat\$"
	then
	    echo "$fname" | sed -e 's/\.[a-zA-Z0-9]$//'
	else
	    echo "$fname"
	fi
    done \
    | sed \
	-e 's/.*\.index$/index/' \
	-e 's/.*\.meta$/meta/' \
	-e 's/.*\.\([0-9][0-9]*\)$/\1/' \
    | sort \
    | uniq -c \
    | while read c x
    do
	case $c
	in
	    1)
	    	;;
	    *)
	    	echo >&2 "$prog: Error: oldname argument ($old) is a prefix for multiple PCP archive files:"
		grep "\\.$x\$" $tmp/old | sed -e 's/^/    /' >&2
		rm -f $tmp/ok
		;;
	esac
    done
    [ -f $tmp/ok ] || exit
else
    echo >&2 "$prog: Error: cannot find any files for the input archive ($old)"
    exit
fi
if grep -q '\.[0-9][0-9]*$' $tmp/old
then
    :
elif grep -E -q '\.[0-9][0-9]*\.'"$pat"'$' $tmp/old
then
    :
else
    echo >&2 "$prog: Error: cannot find any data files for the input archive ($old)"
    eval ls -l "$old_noglob".* >&2
    echo "Inspecting files in this list ..." >&2
    cat $tmp/old >&2
    exit
fi

if grep -q '\.meta$' $tmp/old
then
    :
elif grep -E -q '\.meta\.'"$pat"'$' $tmp/old
then
    :
else
    echo >&2 "$prog: Error: cannot find .meta file for the input archive ($old)"
    eval ls -l "$old_noglob".* >&2
    echo "Inspecting files in this list ..." >&2
    cat $tmp/old >&2
    exit
fi

if $very_verbose
then
    echo "source files: `tr '[\012]' ' ' <$tmp/old`"
fi

# (hard) link oldnames and newnames
#
for f in `cat $tmp/old`
do
    if eval [ ! -f "$f" ]
    then
	echo >&2 "$prog: Error: ln-pass: input file vanished: $f"
	eval ls -l "$old_noglob".* "$new"*
	_cleanup
	# NOTREACHED
    fi
    _get_part "$f"
    if eval [ -f "$new.$part" ]
    then
	echo >&2 "$prog: Error: ln-pass: output file already exists: $new.$part"
	eval ls -l "$old_noglob".* "$new"*
	_cleanup
	# NOTREACHED
    fi
    $verbose && ! $showme && echo >&2 "link $f -> $new.$part"
    echo "$new.$part" >>$tmp/new
    if eval $LN "$f" "$new.$part"
    then
	:
    else
	echo >&2 "$prog: Error: ln $f $new.$part failed!"
	eval ls -l "$old_noglob".* "$new"*
	_cleanup
	# NOTREACHED
    fi
done

# unlink oldnames provided link count is 2
#
for f in `cat $tmp/old`
do
    if eval [ ! -f "$f" ]
    then
	echo >&2 "$prog: Error: rm-pass: input file vanished: $f"
	eval ls -l "$old_noglob".* "$new"*
	_cleanup
	# NOTREACHED
    fi
    links=`eval _count_links $f`
    xpect=2
    $showme && xpect=1
    if [ -z "$links" -o "$links" != $xpect ]
    then
	echo >&2 "$prog: Error: rm-pass: link count "$links" (not $xpect): $f"
	eval ls -l "$old_noglob".* "$new"*
	_cleanup
    fi
    $verbose && ! $showme && echo >&2 "remove $f"
    if eval $RM "$f"
    then
	:
    else
	echo >&2 "$prog: Warning: rm $f failed!"
    fi
done

status=0
