#!/bin/bash
# Tuomo Björk
# 2026-07-27
#
# * Initialise Python environment with requirements.
# * Initialise/update the west workspace from app/west.yml.
# * Install user-level SDKs.
#
BIN_PYTHON="$(which python3)"

DIR_LOG="$PWD/.log"
DIR_VENV="$PWD/.venv"
DIR_MANIFEST="app"

LOG_SETUP="$DIR_LOG/setup.log"

ERR_BIN=1
ERR_ENV=2
ERR_PKG=3


we_good() {
# Check if RC was OK,
# print the small message and exit* on failure.
	local RC=$1
	local EXIT_CODE=$2
	local EXIT_STRAT=${EXIT_STRAT:-default}

	if [[ $RC != 0 ]]; then
		echo "FAIL"
		if [[ "$SKIP_EXIT" != "canfail" ]]; then
			# Do nothing,
			# but pass RC for flexibility
			return $RC
		else
			# Default is to exit
			exit "$EXIT_CODE"
		fi
	else
		echo "OK"
	fi
}

my_west_init() {
	echo " * initialising workspace (manifest: '$DIR_MANIFEST') ... "
	west init -l "$DIR_MANIFEST" #&>> "$LOG_SETUP"
	return $?
}


mkdir -p "$DIR_LOG" && echo -e "\n[LOG_START] $(date -u)" >> "$LOG_SETUP"

# Create or Enter Python virtual environment,
# and install all required components.
echo ">> Python env."
if ! [[ -d "$DIR_VENV" ]]; then
	echo -n " * creating ... "
	$BIN_PYTHON -m venv "$DIR_VENV"
	we_good $? $ERR_ENV
else
	echo " * exists "
fi

echo -n " * entering ... "
source "$DIR_VENV/bin/activate" &>> "$LOG_SETUP"
we_good $? $ERR_ENV

echo -n " * upgrading pip ... "
pip install --upgrade pip &>> "$LOG_SETUP"
we_good $? $ERR_PKG "canfail" # OK if fails

echo -n " * satisfying requirements ... "
pip install -r requirements.txt &>> "$LOG_SETUP"
we_good $? $ERR_PKG

# Handle Everything related to base West and Zephyr.
# Also attempt to install SDK for user.
echo ">> West & Manifest"
if ! [[ -d ".west" ]]; then
	my_west_init
	we_good $? $ERR_PKG
else
	echo " * workspace already initialised ('.west' exists)"
fi

echo " * updating manifest ... "
west update #&>> "$LOG_SETUP"
we_good $? $ERR_PKG "canfail"
if [[ $? != 0 ]]; then
	# Updating can fail if the fetched workspace state is borked somehow.
	rm -fr ".west"
	my_west_init
	we_good $? $ERR_PKG
	west update #&>> "$LOG_SETUP"
fi

echo " * configuring export ... "
west zephyr-export #&>> "$LOG_SETUP"
we_good $? $ERR_ENV

echo -n " * installing python packages ... "
west packages pip --install &>> "$LOG_SETUP"
we_good $? $ERR_PKG

echo -n " * installing SDK to user home ... "
west sdk install &>> "$LOG_SETUP"
we_good $? $ERR_PKG

# Smoke the version
echo -e "\n[FETCH & INSTALL COMPLETE]"
echo " * $(west --version)"
