#include "uart.h"
#include "utils.h"
#include "printf.h"
#include "percpu.h"
#include "stdint.h"
#include "entry.h"
#include "core.h"
#include "vm.h"
#include "mm.h"
#include "heap.h"
#include "libk.h"
#include "kernel_tests.h"
#include "threads.h"

struct Stack
{
    static constexpr int BYTES = 4096;
    uint64_t bytes[BYTES] __attribute__((aligned(16)));
};

PerCPU<Stack> stacks;

static bool smpInitDone = false;

extern "C" uint64_t pickKernelStack(void)
{
    return (uint64_t)&stacks.forCPU(smpInitDone ? getCoreID() : 0).bytes[Stack::BYTES];
}

void print_ascii_art()
{
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

void breakpoint()
{
    return;
}

extern "C" void kernel_main()
{
    heapTests();
}

extern char __heap_start[];
extern char __heap_end[];

#define HEAP_START ((uintptr_t)__heap_start)
#define HEAP_END ((uintptr_t)__heap_end)
#define HEAP_SIZE (HEAP_END - HEAP_START)

static Atomic<int> coresAwake(0);

extern "C" void kernel_init()
{
    if (getCoreID() == 0)
    {
        create_page_tables();
        init_mmu();
        patch_page_tables();
        uart_init();
        init_printf(nullptr, uart_putc_wrapper);
        printf("printf initialized!!!\n");
        breakpoint();
        print_ascii_art();
        uinit((void *)HEAP_START, HEAP_SIZE);
        smpInitDone = true;
        threadsInit();
        wake_up_cores();
    }
    else
    {
        init_mmu();
    }

    printf("Hi, I'm core %d\n", getCoreID());

    auto number_awake = coresAwake.add_fetch(1);
    printf("There are %d cores awake\n", number_awake);

    if (number_awake == CORE_COUNT)
    {
        printf("creating kernel thread\n", number_awake);
        thread([]
               { kernel_main(); });
        thread([]
               { heapTests();
                thread([]
                    { int x = 1;
                        printf("x = %d\n", x);
                        printf("i do nothing\n");
                    yield();
                    x++;
                    printf("x = %d\n", x);
             }); });

        thread([]
               { printf("i do nothing\n"); });
        thread([]
               { kernel_main(); });
    }
    stop();

    if (getCoreID() == 0)
    {
        while (1)
        {
            uart_putc(uart_getc()); // will allow you to type letters through UART
        }
    }
}