#!/bin/bash

# Module configuration
MODULE_NAME="acpi_ec"
MODULE_PATH="/lib/modules/$(uname -r)/kernel/drivers/acpi"
MODULES_LOAD_DIR="/etc/modules-load.d"

# Build configuration
BUILD_DIR="build"
SRC_DIR="src"

# Export variables for Makefile
export MODULE_NAME
export MODULE_PATH
export MODULES_LOAD_DIR
export BUILD_DIR
export SRC_DIR 