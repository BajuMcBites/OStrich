#!/bin/bash
lldb build/kernel.elf --batch -o "gdb-remote localhost:1234"
