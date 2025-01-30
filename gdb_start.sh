#!/bin/bash
gdb-multiarch -ex "file build/kernel.elf" -ex "target remote localhost:1234"
