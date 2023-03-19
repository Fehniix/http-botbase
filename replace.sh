#!/bin/bash
# Replace current sys-module on Switch SD FS with build output

[ ! -d "/Volumes/SWITCH SD" ] && echo "Switch SD not mounted." && exit -1

function unmount () {
	SWITCH_VOLUME=$(diskutil list | grep "SWITCH SD" | grep -ow "disk.*$" | perl -pe "chomp")

	if [ -z $SWITCH_VOLUME ]
	then
		echo "Switch SD volume could not be automatically fetched, need to unmount manually. Exiting."
		exit 0
	else
		diskutil eject "/dev/$SWITCH_VOLUME"
		echo "Switch SD volume ejected."
	fi
}

if [ ! -d "/Volumes/SWITCH SD/atmosphere/contents/410000000000FF15/" ]
then
	echo "Previous build on Switch FS not found. Copying over."
	cp -r ./out/410000000000FF15 "/Volumes/SWITCH SD/atmosphere/contents/"
	unmount
	exit
fi

make

rm -r "/Volumes/SWITCH SD/atmosphere/contents/410000000000FF15/"
echo "Previous build removed from Switch FS"
cp -r ./out/410000000000FF15 "/Volumes/SWITCH SD/atmosphere/contents/"
echo "New build copied over to Switch FS"

unmount