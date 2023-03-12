#!/bin/bash
echo -e "soft_reset_halt\n" | nc localhost 4444 -q 1
