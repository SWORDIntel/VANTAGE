#!/usr/bin/env bash
# VANTAGE Emergency Minimal Bashrc
# Use this file when the terminal crashes due to issues with the regular bashrc
# To use: cp emergency.bashrc ~/.bashrc

# Basic prompt
PS1='\[\033[01;32m\]\u@\h\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]\$ '

# Essential path
export PATH="/usr/local/bin:/usr/bin:/bin:/usr/local/sbin:/usr/sbin:/sbin:$HOME/bin:$HOME/.local/bin"

# Basic settings
HISTCONTROL=ignoreboth
HISTSIZE=1000
HISTFILESIZE=2000
shopt -s checkwinsize
shopt -s histappend

# Enable color support
if [ -x /usr/bin/dircolors ]; then
    test -r ~/.dircolors && eval "$(dircolors -b ~/.dircolors)" || eval "$(dircolors -b)"
    alias ls='ls --color=auto'
    alias grep='grep --color=auto'
    alias fgrep='fgrep --color=auto'
    alias egrep='egrep --color=auto'
fi

# Basic aliases
alias ll='ls -alF'
alias la='ls -A'
alias l='ls -CF'

# Basic functions
cd() { builtin cd "$@" && ls; }

# Instructions for recovery
echo "*** VANTAGE EMERGENCY MODE ***"
echo "This is a minimal .bashrc file for recovery."
echo "To reinstall VANTAGE properly, run:"
echo "  cd ~/Documents/GitHub/VANTAGE && ./install.sh"
echo "------------------------------"
