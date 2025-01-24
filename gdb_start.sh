#!/bin/bash
gdb -ex "file build/kernel.elf" -ex "target remote localhost:1234"
