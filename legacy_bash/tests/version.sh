#!/bin/bash


update_version()
{
	local vtype=$2
	local newv=$4

	echo "Updating $TARGET"
	sed -e "s/BASHRC_VERSIONINFO\[$vtype\]=[0-9]./BASHRC_VERSIONINFO\[$vtype\]=$newv/" $TARGET


}

TARGET=$1

if [ -a $TARGET ]; then
	update_version 1 3
fi

#BASHRC_VERSIONINFO[0]=1         #Major version
#BASHRC_VERSIONINFO[1]=2         #Minor version
#BASHRC_VERSIONINFO[2]=4         #Micro version
#BASHRC_VERSIONINFO[3]=1         #Patch ID
#BASHRC_VERSIONINFO[4]="beta"    #Release type
