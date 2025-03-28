# ACPI EC Driver with Multi-EC Support

A Linux kernel module providing direct access to ACPI Embedded Controller (EC) with multi-EC support, particularly useful for MSI laptops.

## Features

- Direct ACPI EC register access
- Multi-EC device support
- Independent read/write operations
- Cooler Boost control for MSI laptops
- User-space interface via device files
- Automatic module loading on boot

## Prerequisites

Before installing, ensure you have the following packages installed:

```bash
sudo apt update
sudo apt install build-essential linux-headers-$(uname -r)
```

## Project Structure

```
.
├── src/                    # Source code directory
│   ├── acpi_ec.c          # Main module source
│   └── acpi_ec.h          # Module header
├── install.sh             # Installation script
├── uninstall.sh          # Uninstallation script
├── cooler_boost.sh       # MSI Cooler Boost control utility
├── Makefile              # Build system
└── _variables.sh         # Configuration variables
```

## Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/agnath18K/acpi_ec.git
   cd acpi_ec
   ```

2. Install the module:
   ```bash
   sudo ./install.sh
   ```

   This will:
   - Build the kernel module
   - Install it to the appropriate kernel directory
   - Load the module
   - Configure automatic loading on boot

## Usage

### Device Files
After installation, two device files will be created:
- `/dev/ec0` - Primary EC
- `/dev/ec1` - Secondary EC (if present)

By default, these files are only accessible by root. To allow user access:
```bash
sudo chmod 666 /dev/ec*
# Or add your user to the ec group (recommended)
sudo usermod -a -G ec $USER
```

### Reading EC Registers
```bash
# Read from offset 0x98 (152 decimal)
sudo dd if=/dev/ec0 bs=1 count=1 skip=152 2>/dev/null | hexdump -C
```

### Writing EC Registers
```bash
# Write 0x80 to offset 0x98 (152 decimal)
echo -ne "\x80" | sudo dd of=/dev/ec0 bs=1 count=1 seek=152 2>/dev/null
```

### Cooler Boost Control (MSI Laptops)
The `cooler_boost.sh` script provides easy control of the Cooler Boost feature:
```bash
./cooler_boost.sh status  # Check current status
./cooler_boost.sh on     # Enable Cooler Boost
./cooler_boost.sh off    # Disable Cooler Boost
```

## Uninstallation

To remove the module:
```bash
sudo ./uninstall.sh
```

This will:
- Unload the module
- Remove it from the kernel directory
- Remove auto-loading configuration

## Troubleshooting

1. Check module status:
   ```bash
   lsmod | grep acpi_ec
   dmesg | grep acpi_ec
   ```

2. Check kernel logs:
   ```bash
   sudo journalctl -k | grep acpi_ec
   ```

3. Permission issues:
   ```bash
   # Check device file permissions
   ls -l /dev/ec*
   # Check your group membership
   groups
   ```

## Contributing

1. Fork the repository
2. Create your feature branch
3. Make your changes
4. Submit a pull request

## License

This project is licensed under the GPL v2 License - see the [LICENSE](LICENSE) file for details.

## Thanks to  

- Thomas Renninger (ec_sys.c)  
- Sayafdine Said (Out-of-tree port)  