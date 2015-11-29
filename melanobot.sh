#!/bin/bash
#
# Copyright 2015 Mattia Basaglia
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#
#
# Bot launcher script.
#
# In order for this to work properly, the POSIX module must have be enabled and
# the bot configuration needs to have the following code inside "bot":
#
#     Pipe
#     {
#         type            Template
#         template        Pipe
#     }
#

# Directory of this script file
SELFDIR=$(dirname $(readlink -se "${BASH_SOURCE[0]}"))

# Directory to run the bot from
declare MELANOBOT_RUN_DIR="$PWD"
# Directory with the bot sources
declare MELANOBOT_SRC_DIR="$SELFDIR"
# Directory to compile in
declare MELANOBOT_BUILD_DIR="$MELANOBOT_SRC_DIR/build"
# Directory containing the executable
declare MELANOBOT_BIN_DIR="$MELANOBOT_BUILD_DIR/bin"
# Executable name, relative to BIN_DIR
declare MELANOBOT_EXECUTABLE="melanobot"
# Directory for temporary files
declare MELANOBOT_TMP_DIR="/tmp/.melanobot"
# Bot identifier
declare MELANOBOT_BOT_ID="$USER"
# Tmux session
declare MELANOBOT_TMUX_SESSION="bot"


# Returns the name of the temporary directory for this specific bot instance
# If it doesn't exists, it gets created
tmp_dir()
{
    local tdir="$MELANOBOT_TMP_DIR/$MELANOBOT_BOT_ID"
    if [ ! -d "$tdir" ]
    then
        mkdir -p "$tdir"
    fi
    echo "$tdir"
}

# Prints an error message
error_log()
{
    echo -e "\x1b[31mError\x1b[0m: $@" 1>&2
}

# Prints an error message and exits with status 1
error()
{
    error_log $@
    exit 1
}

# Checks whether the bot appears to be running
is_running()
{
    local pidfile="$(tmp_dir)/pid"
    [ -f "$pidfile" ] && kill -0 "$(cat "$pidfile")" 2>/dev/null
}

# Runs the bot (blocking)
# Accepts arguments passed to the executable
melanobot_run()
{
    # Go to the selected work directory
    cd "$MELANOBOT_RUN_DIR"

    # Pipe file for input
    local input="$(tmp_dir)/input"
    # Action file for looping
    local action="$(tmp_dir)/action"
    # Pid file to recognize the process running the server
    local pid="$(tmp_dir)/pid"

    is_running && error "$MELANOBOT_EXECUTABLE is already running"

    # Create a named pipe
    mkfifo "$input"

    # Create file with the process id
    echo $$ >"$pid"

    # Loop forever
    while :
    do
        # Run the bot
        LC_ALL=C "$MELANOBOT_BIN_DIR/$MELANOBOT_EXECUTABLE" $@ \
            --settings.melanobot_sh=1 \
            --settings.tmp_dir="$(tmp_dir)" \
            --bot.Pipe.file="$input"

        # Check for an action file
        if [ -f "$action" ]
        then
            case $(cat "$action") in
                restart)
                    # Restart, we just remove the file to restart this once
                    rm "$action"
                    ;;
                quit)
                    # Exit, we remove the file and exit
                    rm "$action"
                    rm "$input"
                    rm "$pid"
                    exit 0
                    ;;
                loop)
                    # Keep looping, we the file remains untouched
                    ;;
                *)
                    # Error, unknown action
                    rm "$input"
                    rm "$pid"
                    error "Unknown loop action"
                    ;;
            esac
        else
            # No special action, therefore we exit
            rm "$input"
            rm "$pid"
            exit 0
        fi
    done
}

# Starts the bot (non-blocking)
# Accepts arguments passed to the executable
# If the first argument is "loop", it will call melanobot_loop
melanobot_start()
{
    # Pipe file for input
    local input="$(tmp_dir)/input"
    is_running && error "$MELANOBOT_EXECUTABLE is already running"

    if [ "$1" = loop ]
    then
        shift
        melanobot_loop
    fi

    # TODO pass on the MELANOBOT_* variables
    if tmux new -d -s "$MELANOBOT_TMUX_SESSION" "\"$0\" run $@"
    then
        echo "Bot started"
    else
        error "Bot starting failed"
    fi
}

# Stops the bot
melanobot_stop()
{
    if is_running
    then
        echo quit >"$(tmp_dir)/action"
        echo quit >"$(tmp_dir)/input"
        sleep 1
        if is_running
        then
            error_log "Didn't close cleanly"
            kill -9 "$(cat "$(tmp_dir)/pid")"
            rm -f "$(tmp_dir)/pid" "$(tmp_dir)/input"
        else
            echo "$MELANOBOT_EXECUTABLE exited cleanly"
        fi
    else
        error "$MELANOBOT_EXECUTABLE isn't running" 1>&2
    fi
}

# Restarts the bot
melanobot_restart()
{
    # Pipe file for input
    local input="$(tmp_dir)/input"
    is_running && melanobot_stop
    rm -f "$input"
    melanobot_start
}

# Pulls, compiles and restarts
melanobot_update()
{
    cd "$MELANOBOT_SRC_DIR"
    git pull
    melanobot_configure
    melanobot_build $@
    melanobot_restart
}

# Cleanup residual temporary files
melanobot_cleanup()
{
    # Clear old files
    rm -f "$(tmp_dir)"/*
}

# Forces to restart
melanobot_loop()
{
    # Action file for looping
    local action="$(tmp_dir)/action"
    echo loop >$action
}

# Removes forced restart
melanobot_noloop()
{
    # Action file for looping
    local action="$(tmp_dir)/action"
    if [ "$(cat "$action")" = "loop" ]
    then
        rm $action
    fi
}

# Compiles the bot
# Accepts parameters which are sent to make
melanobot_build()
{
    if [ ! -d "$MELANOBOT_BUILD_DIR" ]
    then
        echo "Error: Build directory isn't configured" 1>&2
        exit 1
    fi
    cd "$MELANOBOT_BUILD_DIR"
    make $@
}

# Configures the bot for compilation
# Accepts parameters which are sent to cmake
melanobot_configure()
{
    if [ ! -d "$MELANOBOT_BUILD_DIR" ]
    then
        mkdir -p "$MELANOBOT_BUILD_DIR"
    fi
    cd "$MELANOBOT_BUILD_DIR"
    cmake $@ "$MELANOBOT_SRC_DIR"
}

# Attaches to the tmux session
melanobot_attach()
{
    is_running || error "Bot isn't running"

    if tmux has -t "$MELANOBOT_TMUX_SESSION"
    then
        tmux attach -t "$MELANOBOT_TMUX_SESSION"
    else
        error "Bot is running but not in the tmux session \"$MELANOBOT_TMUX_SESSION\""
    fi
}

# Shows help
# TODO: Better formatting, more descriptive help
melanobot_help()
{
    echo "Usage:"
    echo "    $0 [variable=value...] command [option...]"
    echo "Commands:"
    declare -F | grep -oE "melanobot_[a-z_]+" | sed 's/melanobot_/    /'
    echo "Variables:"
    declare -p | grep -oE "MELANOBOT_[a-zA-Z_]+=.*" | sed -r -e 's/MELANOBOT_/    /' -e 's/=(.*)/ [\1]/'
}

# Process command line arguments
while [ "$1" ]
do
    arg="$1"
    shift
    case "$arg" in
        *=*)
            varname="${arg%%=*}"
            value="${arg#*=}"
            if declare -p "MELANOBOT_$varname" &>/dev/null
            then
                eval "MELANOBOT_$varname=\"\$value\""
            else
                error_log "Unknown variable name: $varname"
            fi
        ;;
        *)
            if [ "$(type -t "melanobot_$arg")" = "function" ]
            then
                # Run the selected funtion, passing the rest of the arguments to it
                "melanobot_$arg" $@
                exit 0
            else
                error_log "Unknown command line option: $arg"
            fi
        ;;
    esac
done

error_log "No command specified"
melanobot_help
exit 1
