columns = ["Register",	"Offset",	"Width",	"Access",	"Reset Value",	"Description"]

hardcoded = {
    "64": "uint64_t",
    "32": "uint32_t",
    "16": "uint16_t",
    "8": "uint8_t"
}   

print(f"#define USB_BASE 0x3F980000")

with open("offsets.txt", "r") as file:
    
    while True:
        header = file.readline().strip()
        description = file.readline().strip()
        if(header == '' or description == ''):
            break
        register, offset, width, access, reset_value = header.split()
        print(f"// {description}")
        print(f"#define USB_{register} ((volatile {hardcoded[width] if width in hardcoded else "uint32_t"} *)(USB_BASE + {offset}))")
        print(f"#define USB_{register}_RESET() *USB_{register} = {reset_value};")


        