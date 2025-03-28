#!/bin/bash
# This script was partly inspired by https://github.com/atar-axis/xpadneo/blob/master/install.sh

# Source variables
source _variables.sh

set -e

if [[ "$EUID" != 0 ]]; then
  echo "The script needs to be run as root."
  exit 1
fi

echo "Starting installation of $MODULE_NAME module..."

# Build the module
echo "Building module..."
make clean && make

# Install the module
echo "Installing module..."
make install

# Update module dependencies
echo "Updating module dependencies..."
depmod -a

# Load the module
echo "Loading module..."
modprobe $MODULE_NAME

# Configure module auto-loading
echo "Configuring module auto-loading..."
echo "$MODULE_NAME" > "$MODULES_LOAD_DIR/$MODULE_NAME.conf"

echo "Installation completed successfully!"
echo "Module will be loaded automatically on next boot."
