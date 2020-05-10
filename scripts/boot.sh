#!/bin/bash
FILENAME=kernel/kernel.bin
FILESIZE=$(stat -c%s "$FILENAME")
echo "Size of $FILENAME = $FILESIZE bytes."
SECTORSIZE=512
ROUNDEDKSIZE=$((FILESIZE+SECTORSIZE))
FINALSIZE=$((ROUNDEDKSIZE/SECTORSIZE))
echo "$FINALSIZE" > bootloader/boot0.h