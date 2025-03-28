#!/bin/bash

# Cooler Boost control script for MSI laptops

COOLER_BOOST_REG=152  # 0x98 in decimal

function get_status() {
    value=$(sudo dd if=/dev/ec0 bs=1 count=1 skip=$COOLER_BOOST_REG 2>/dev/null | hexdump -e '1/1 "%02x"')
    case "$value" in
        "00") echo "Cooler Boost is OFF" ;;
        "80") echo "Cooler Boost is ON" ;;  # 128 in hex is 0x80
        *) echo "Cooler Boost status: unknown (0x$value)" ;;
    esac
}

function set_status() {
    if [ "$1" == "on" ]; then
        echo -ne "\x80" | sudo dd of=/dev/ec0 bs=1 count=1 seek=$COOLER_BOOST_REG 2>/dev/null
        echo "Enabled Cooler Boost"
    elif [ "$1" == "off" ]; then
        echo -ne "\x00" | sudo dd of=/dev/ec0 bs=1 count=1 seek=$COOLER_BOOST_REG 2>/dev/null
        echo "Disabled Cooler Boost"
    fi
}

case "$1" in
    "on")
        set_status on
        sleep 1
        get_status
        ;;
    "off")
        set_status off
        sleep 1
        get_status
        ;;
    "status")
        get_status
        ;;
    *)
        echo "Usage: $0 {on|off|status}"
        echo "Controls MSI laptop Cooler Boost feature"
        exit 1
        ;;
esac

exit 0 