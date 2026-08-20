#!/bin/bash
# 
# Tuomo Björk
# 2026-08-20
#
# This convenience script will:
# - Create Python VENV
# - Install PIP packages
# - Prepare West & Zephyr
# - Install SDKs
#

D_VENV="./.venv"
D_SPACE="space-stable"

ZEPHYR_V="v4.4.1"

CMD_PYTHON="python3"

RC_OK=0
RC_ERR_GEN=1
RC_ERR_BIN=2
RC_ERR_ENV=4
RC_ERR_PKG=8

#LOG_REDIR="/dev/null"
LOG_REDIR="$PWD/setup.log"


function is_exec() {
	local target=${1:-}

	# Guard
	if [[ -z "$target" ]]; then
		return $RC_ERR_GEN
	fi

	# Find
	local bin="$(command -v "$target" 2>> "$LOG_REDIR")"

	echo -n "* $target... "
	if [[ -z "$bin" || ! -x "$bin" ]]; then
		echo "FAIL"
		exit $RC_ERR_BIN
	fi
	echo "OK"
	return $RC_OK
}

function run() {
	local msg="$1"
	local code_err=${2:-$RC_ERR_GEN}

	# Flush
	shift 2

	echo -n "* $msg... "
	if "$@" &>> "$LOG_REDIR"; then
		echo "OK"
	else
		echo "FAIL ($?)"
		exit $code_err
	fi
}

function do_runtimes() {
	echo ">> Locating Runtimes"

	# Runtime(s)
	is_exec "$CMD_PYTHON"
	#

	echo -e "DONE\n"
}

function do_python() {
echo ">> Python Environment"

	# Activate Python virtual environment
	if ! [[ -d "$D_VENV" ]]; then
	run	"Creating"					$RC_ERR_ENV		$CMD_PYTHON -m venv "$D_VENV"
	fi
	run	"Activating"				$RC_ERR_ENV		source "$D_VENV/bin/activate"

	# Install stuffs
	run "Updating pip"				$RC_ERR_PKG		pip install --upgrade pip
	run "Installing packages"		$RC_ERR_PKG		pip install -r requirements.txt

	echo -e "DONE\n"
}

function do_west() {
echo ">> West"

	# Create Space
	if ! [[ -d "$D_SPACE" ]]; then
	run "Creating '$D_SPACE'"	$RC_ERR_GEN		mkdir -p "$D_SPACE"
	fi
	run "Entering '$D_SPACE'"	$RC_ERR_GEN		cd "$D_SPACE"

	# West Setup
	if ! [[ -d "./zephyr" ]]; then
	run	"Initialise '$ZEPHYR_V'"	$RC_ERR_PKG		west init --mr "$ZEPHYR_V"
	fi
	run	"Enter Zephyr"				$RC_ERR_PKG		cd zephyr
	run "Update West"				$RC_ERR_PKG		west update
	run	"Enable Zephyr export"		$RC_ERR_PKG		west zephyr-export
	run "Install packages"			$RC_ERR_PKG		west packages pip --install
	run "Install SDK"				$RC_ERR_PKG		west sdk install
}


echo -e "\n[SETUP] BEGIN $(date -u)" >> "$LOG_REDIR"
do_runtimes && \
do_python && \
do_west
echo -e "[SETUP] END" >> "$LOG_REDIR"
