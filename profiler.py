from elftools.elf.elffile import ELFFile
from elftools.elf.sections import Symbol
from typing import List

def get_elf_symbols(elf_file_path) -> dict[int, Symbol]:
    """
    Parses the symbol table of an ELF file.

    Args:
        elf_file_path: Path to the ELF file.
    """
    with open(elf_file_path, 'rb') as f:
        elf = ELFFile(f)
        
        symbol_map = {}
        symbol_table = elf.get_section_by_name(".symtab")
        if symbol_table:
            for symbol in symbol_table.iter_symbols():
                symbol_map[symbol.entry.st_value] = symbol
        return symbol_map

import re
from collections import defaultdict

def parse_log(log_file, symbol_map: dict[int, Symbol]):
    events: List[list] = []
    events.append([])
    events.append([])
    events.append([])
    events.append([])
    with open(log_file, 'rb') as f:
        while True:
            bytes = f.read(32)
            if not bytes:
                break
            function = int.from_bytes(bytes[0:8], byteorder='little', signed=False)
            caller = int.from_bytes(bytes[8:16], byteorder='little', signed=False)
            timestamp = int.from_bytes(bytes[16:24], byteorder='little', signed=False)
            flags = int.from_bytes(bytes[24:32], byteorder='little', signed=False)

            core_id = (flags >> 8) & 0xFF
            direction = flags & 0xFF

            events[core_id].append({
                'timestamp': str(timestamp),
                'type': 'enter' if direction == 1 else 'exit',
                'function': symbol_map[function].name if function in symbol_map else str(function),
                'caller': symbol_map[caller].name if caller in symbol_map else str(caller)
            })
    return events

def reconstruct_stacks(events):
    # This is a simplified and conceptual approach.
    # Real stack reconstruction based on enter/exit events can be tricky
    # and might require more sophisticated logic to handle recursion and asynchronicity.
    active_calls = []
    stack_counts = defaultdict(int)

    for event in events:
        if event['type'] == 'enter':
            active_calls.append(event['function'])
        elif event['type'] == 'exit' and active_calls:
            if active_calls[-1] == event['function']:
                current_stack = ";".join(reversed(active_calls)) # Bottom of stack first
                stack_counts[current_stack] += 1
                active_calls.pop()
            else:
                print(f"Warning: Mismatched enter/exit for {event['function']}")
                if active_calls:
                    active_calls.pop() # Try to recover

    return stack_counts

def format_for_flamegraph(stack_counts, output_file):
    with open(output_file, 'w') as f:
        for stack, count in stack_counts.items():
            f.write(f"{stack} {count}\n")

# if __name__ == "__main__":
#     log_file = "your_function_log.txt"  # Replace with your log file
#     events = parse_log(log_file)
#     stack_counts = reconstruct_stacks(events)
#     output_file = "flamegraph_input.txt"
#     format_for_flamegraph(stack_counts, output_file)
#     print(f"Formatted data for flamegraph saved to {output_file}")
#     print("Now run './flamegraph.pl flamegraph_input.txt > flamegraph.svg' in your terminal.")

def main():

    symbol_map: dict[int, Symbol] = get_elf_symbols('./build/kernel.elf')
    events = parse_log("uart1.log", symbol_map)
    i =0
    for core_stack in events:
        stack_counts = reconstruct_stacks(core_stack)
        output_file = f"flamegraph_input_{i}.txt"
        format_for_flamegraph(stack_counts, output_file)
        print(f"Formatted data for flamegraph saved to {output_file}")
        print("Now run './flamegraph.pl flamegraph_input.txt > flamegraph.svg' in your terminal.")
        i += 1


if __name__ == "__main__":
    main()