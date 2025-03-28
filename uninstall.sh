#!/bin/bash
# This script was partly inspired by https://github.com/atar-axis/xpadneo/blob/master/uninstall.sh

# Source variables
source _variables.sh

set -e

if [[ "$EUID" != 0 ]]; then
    echo "The script needs to be run as root."
    exit 1
fi

echo "Starting uninstallation of $MODULE_NAME module..."

# Unload the module if it's loaded
echo "Checking if module is loaded..."
if lsmod | grep -q "^$MODULE_NAME "; then
    echo "Module is loaded. Unloading..."
    modprobe -rq $MODULE_NAME
    echo "Module unloaded successfully."
else
    echo "Module is not loaded."
fi

# Remove the module auto-loading
echo "Removing module from auto-load configuration..."
rm -f "$MODULES_LOAD_DIR/$MODULE_NAME.conf"

# Remove the module
echo "Removing module..."
make uninstall

# Update module dependencies
echo "Updating module dependencies..."
depmod -a

echo "Uninstallation completed successfully!"
