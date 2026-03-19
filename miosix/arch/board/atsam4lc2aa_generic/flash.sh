#!/bin/bash
echo -e "soft_reset_halt\nadapter speed 1000\nflash write_image erase main.bin 0x4000\nresume" | nc localhost 4444 -q 1
