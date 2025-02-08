#include "uart.h"
#include "utils.h"
#include "printf.h"
#include "percpu.h"
#include "stdint.h"
#include "entry.h"
#include "core.h"
#include "mm.h"

struct Stack {
    static constexpr int BYTES = 4096;
    uint64_t bytes[BYTES] __attribute__ ((aligned(16)));
};

PerCPU<Stack> stacks;

static bool smpInitDone = false;

uint64_t PGD[512] __attribute__((aligned(4096), section(".paging")));
uint64_t PUD[512] __attribute__((aligned(4096), section(".paging")));
uint64_t PMD[512] __attribute__((aligned(4096), section(".paging")));
uint64_t PTE[512] __attribute__((aligned(4096), section(".paging")));


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


void patch_page_tables() {

    uint64_t lower_attributes = 0x1 | (0x0 << 2) | (0x1 << 10) | (0x0 << 6) | (0x0 << 8);

    for (int i = 504; i < 512; i++) {
        PMD[i] = PMD[i] & (0xFFFFFFFFFFFFF000);
        PMD[i] = PMD[i] | lower_attributes;
    }

}

void test_atomic_operations(void) {
    int val = 0;
    int result;
    int new_val = 10;

    printf("Initial value of val: %d\n", val);

    result = __atomic_exchange_n(&val, new_val, __ATOMIC_SEQ_CST);

    printf("Value after atomic exchange: %d, old value: %d\n", val, result);

    if (val == new_val) {
        printf("Atomic exchange successful, value updated to: %d\n", val);
    } else {
        printf("Atomic exchange failed, value not updated. Current val: %d\n", val);
    }

    printf("Final value of val: %d\n", val);
}

void breakpoint(){
    return;
}

extern "C" void kernel_init() {
    if(getCoreID() == 0){
        create_page_tables();
        init_mmu();
        patch_page_tables();
        uart_init();
        init_printf(nullptr, uart_putc_wrapper);
        printf("printf initialized!!!\n");
        breakpoint();
        print_ascii_art();
        smpInitDone = true;
        wake_up_cores();
    } else {
        init_mmu();
    }

    // test_atomic_operations();           
    // Enable MMU (all cores must set the enable MMU bit to 1)

    // this line here will cause race conditions between cores
    if(getCoreID() == 1){
    printf("Hi, I'm core %d\n", getCoreID());
    }

    if(getCoreID() == 0){
        while (1) {
            uart_putc(uart_getc()); // will allow you to type letters through UART
        }
    }
}

