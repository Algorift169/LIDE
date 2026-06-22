#!/bin/bash
# bmr.sh - Run the BlackLine Linux ISO in QEMU for testing

WORKSPACE="/home/israfil/Desktop/LIDE"
ISO_PATH="$WORKSPACE/iso/blackline.iso"

if [ ! -f "$ISO_PATH" ]; then
    echo "Error: ISO not found at $ISO_PATH"
    echo "Please build the ISO first by running ./iso/build_iso.sh"
    exit 1
fi

echo "Starting BlackLine OS in QEMU..."
# Common QEMU options:
#   -usb -device usb-tablet: Absolute mouse positioning (fixes cursor alignment)
#   -vga virtio: GPU acceleration for better graphics performance
QEMU_COMMON="-m 2048 -cdrom $ISO_PATH -vga virtio -usb -device usb-tablet -serial stdio"

# Try kvm if available, fallback to pure emulation if not
if [ -e /dev/kvm ] && groups | grep -q -E "kvm|root"; then
    qemu-system-x86_64 -enable-kvm $QEMU_COMMON
else
    qemu-system-x86_64 $QEMU_COMMON
fi