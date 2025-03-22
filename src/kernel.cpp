#include "uart.h"
#include "utils.h"
#include "printf.h"
#include "percpu.h"
#include "stdint.h"
#include "entry.h"
#include "core.h"
#include "mm.h"
#include "framebuffer.h"
#include "atomic.h"
#include "dcache.h"
#include "icache.h"
#include "vm.h"

void mergeCores();

struct Stack {
    static constexpr int BYTES = 4096;
    uint64_t bytes[BYTES] __attribute__ ((aligned(16)));
};

SpinLock lock;
PerCPU<Stack> stacks;
static bool smpInitDone = false;

extern "C" uint64_t pickKernelStack(void) {
    return (uint64_t) &stacks.forCPU(smpInitDone ? getCoreID() : 0).bytes[Stack::BYTES];
}

void print_ascii_art() {
    printf("\n");
    printf("                                                                                     \n");
    printf("      # ###          #######                                                /       \n");
    printf("    /  /###        /       ###                           #                #/        \n");
    printf("   /  /  ###      /         ##      #                   ###               ##        \n");
    printf("  /  ##   ###     ##        #      ##                    #                ##        \n");
    printf(" /  ###    ###     ###             ##                                     ##        \n");
    printf("##   ##     ##    ## ###         ######## ###  /###    ###        /###    ##  /##   \n");
    printf("##   ##     ##     ### ###      ########   ###/ #### /  ###      / ###  / ## / ###  \n");
    printf("##   ##     ##       ### ###       ##       ##   ###/    ##     /   ###/  ##/   ### \n");
    printf("##   ##     ##         ### /##     ##       ##           ##    ##         ##     ## \n");
    printf("##   ##     ##           #/ /##    ##       ##           ##    ##         ##     ## \n");
    printf(" ##  ##     ##            #/ ##    ##       ##           ##    ##         ##     ## \n");
    printf("  ## #      /              # /     ##       ##           ##    ##         ##     ## \n");
    printf("   ###     /     /##        /      ##       ##           ##    ###     /  ##     ## \n");
    printf("    ######/     /  ########/       ##       ###          ### /  ######/   ##     ## \n");
    printf("      ###      /     #####          ##       ###          ##/    #####     ##    ## \n");
    printf("               |                                                                 /  \n");
    printf("                \\)                                                              /   \n");
    printf("                                                                               /    \n");
    printf("                                                                              /     \n");
}

extern "C" void secondary_kernel_init() {
    init_mmu();
    mergeCores();
}

extern "C" void primary_kernel_init() {
        create_page_tables();
        patch_page_tables();
        init_mmu();
        uart_init();
        init_printf(nullptr, uart_putc_wrapper);
        printf("printf initialized!!!\n");
        print_ascii_art();
        if (fb_init()) {
            printf("Framebuffer initialization successful!\n");
        } else {
            printf("Framebuffer initialization failed!\n");
        }
        smpInitDone = true;
        // with data cache on, we must write the boolean back to memory to allow other cores to see it.
        clean_dcache_line(&smpInitDone);
        wake_up_cores();
        mergeCores();
}

void mergeCores(){
    uint64_t sp_val;
    asm volatile("mov %0, sp" : "=r" (sp_val));
    printf("Core #%d stack pointer = 0x%llx\n", getCoreID(), sp_val);

    // uncomment this code to do the animation
    // init_animation(); 
    // while (1) {
    //     update_animation();
    // }  

    if(getCoreID() == 0){
        while (1) {
            char ch = uart_getc();
            // commneted out code below will allow you to update letters on screen based on uart input.
            // char temp[1];
            // temp[0] = ch;
            // const char* str = temp;
            // fb_print(str, WHITE);
            uart_putc(ch); // will allow you to type letters through UART
        }
    }
}

