/*
    mbed Memory Status Helper
    Copyright (c) 2017 Max Vilimpoc
    
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
*/

/**
 * Purpose: Print out thread info and other useful details using 
 *          only raw serial access.
 * 
 * Based on mbed_board.c's error printing functionality, minus 
 * pulling in all the printf() code.
 */

#include "mbed_stats.h"

#include "platform/critical.h"
#include "hal/serial_api.h"

// #define CAN_DEBUG_ISR_STACK_USAGE

#ifdef CAN_DEBUG_ISR_STACK_USAGE

#include "compiler_abstraction.h"

// Value is sprayed into all of the ISR stack at boot time.
static const uint32_t ISR_STACK_CANARY = 0xAFFEC7ED; // AFFECTED

// Refers to linker script defined symbol, may not be available
// on all platforms.
extern uint32_t __StackLimit;
extern uint32_t __StackTop;

void fill_isr_stack_with_canary(void)
{
    uint32_t * bottom = &__StackLimit;
    uint32_t * top    = (uint32_t *) GET_SP();

    for (; bottom < top; bottom++)
    {
        *bottom = ISR_STACK_CANARY;
    }
}

#endif // CAN_DEBUG_ISR_STACK_USAGE

#if DEVICE_SERIAL

extern int      stdio_uart_inited;
extern serial_t stdio_uart;

static void check_serial_and_init(void)
{
    if (!stdio_uart_inited) 
    {
        serial_init(&stdio_uart, STDIO_UART_TX, STDIO_UART_RX);
        serial_baud(&stdio_uart, 115200); // This is hard coded.
    }
}

static void debug_print_label(const char * const buffer, uint8_t size)
{
    core_util_critical_section_enter();
    check_serial_and_init();

    for (uint8_t i = 0; i < size; i++) 
    {
        serial_putc(&stdio_uart, buffer[i]);
    }
        
    core_util_critical_section_exit();
}

static const char HEX[] = "0123456789ABCDEF";

static void debug_print_u32(uint32_t u32)
{
    core_util_critical_section_enter();
    check_serial_and_init();
    
    // Always printed as bigendian.
    serial_putc(&stdio_uart, HEX[(((uint32_t) u32 & 0xf0000000) >> 28)]);
    serial_putc(&stdio_uart, HEX[(((uint32_t) u32 & 0x0f000000) >> 24)]);
    serial_putc(&stdio_uart, HEX[(((uint32_t) u32 & 0x00f00000) >> 20)]);
    serial_putc(&stdio_uart, HEX[(((uint32_t) u32 & 0x000f0000) >> 16)]);
    serial_putc(&stdio_uart, HEX[(((uint32_t) u32 & 0x0000f000) >> 12)]);
    serial_putc(&stdio_uart, HEX[(((uint32_t) u32 & 0x00000f00) >>  8)]);
    serial_putc(&stdio_uart, HEX[(((uint32_t) u32 & 0x000000f0) >>  4)]);
    serial_putc(&stdio_uart, HEX[(((uint32_t) u32 & 0x0000000f) >>  0)]);

    core_util_critical_section_exit();
}

static void debug_print_pointer(const void * pointer)
{
    debug_print_u32((uint32_t) pointer);
}

#define DPL(X) debug_print_label((X), sizeof(X) - 1)

#if (defined (MBED_CONF_RTOS_PRESENT) && (MBED_CONF_RTOS_PRESENT != 0))

#include "cmsis_os.h"

// Temporarily #undef NULL or the compiler complains about previous def.
#undef NULL
#include "rt_TypeDef.h"
 
// No public forward declaration for this.
extern P_TCB rt_tid2ptcb (osThreadId thread_id);

static void print_thread_info(osThreadId threadId)
{
    if (!threadId) return;

    osEvent event;
    
    P_TCB tcb = rt_tid2ptcb(threadId);
    
    DPL("    stack ( start: ");
    debug_print_pointer(tcb->stack);
    
    event = _osThreadGetInfo(threadId, osThreadInfoStackSize);
    
    DPL(" end: ");
    debug_print_pointer(((uint8_t *) tcb->stack + event.value.v)); // (tcb->priv_stack)));

    DPL(" size: ");
    debug_print_u32(event.value.v);
    
    event = _osThreadGetInfo(threadId, osThreadInfoStackMax);
    DPL(" used: ");
    debug_print_u32(event.value.v);

    
    DPL(" ) ");

    DPL("thread ( id: ");
    debug_print_pointer(threadId);
    
    event = _osThreadGetInfo(threadId, osThreadInfoEntry);
    DPL(" entry: ");
    debug_print_pointer(event.value.p);

    DPL(" )\r\n");
}

void print_all_thread_info(void)
{
    osThreadEnumId enumId   = _osThreadsEnumStart();
    osThreadId     threadId = NULL;

    while ((threadId = _osThreadEnumNext(enumId)))
    {
        print_thread_info(threadId);
    }
 
    _osThreadEnumFree(enumId);
}

#endif // MBED_CONF_RTOS_PRESENT

static void print_memory_contents(const uint32_t * start, const uint32_t * end)
{
    uint8_t line = 0;
    
    for (; start < end; start++)
    {
        if (0 == line)
        {
            debug_print_pointer(start);
            DPL(": ");
        }
        
        debug_print_u32(*start);
        
        line++;
        
        if (16 == line)
        {
            DPL("\r\n");
            line = 0;
        }
    }
}

extern uint32_t mbed_stack_isr_size;

#ifdef CAN_DEBUG_ISR_STACK_USAGE

uint32_t calculate_isr_stack_usage(void)
{
    for (const uint32_t * stack = &__StackLimit; stack < &__StackTop; stack++)
    {
        if (*stack != ISR_STACK_CANARY)
        {
            return (uint32_t) &__StackTop - (uint32_t) stack;
        }
    }
    
    return mbed_stack_isr_size;
}

#endif

void print_heap_and_isr_stack_info(void)
{
    extern unsigned char * mbed_heap_start;
    extern uint32_t        mbed_heap_size;

    extern unsigned char * mbed_stack_isr_start;
    
    mbed_stats_heap_t      heap_stats;
    
    mbed_stats_heap_get(&heap_stats);

    DPL("     heap ( start: ");
    debug_print_pointer(mbed_heap_start);
    
    DPL(" end: ");
    debug_print_pointer(mbed_heap_start + mbed_heap_size);
    
    DPL(" size: ");
    debug_print_u32(mbed_heap_size);
    
    DPL(" used: ");
    debug_print_u32(heap_stats.max_size);
    
    DPL(" )  alloc ( ok: ");
    debug_print_u32(heap_stats.alloc_cnt);
    
    DPL("  fail: ");
    debug_print_u32(heap_stats.alloc_fail_cnt);
    
    DPL(" )\r\n");
    
    DPL("isr_stack ( start: ");
    debug_print_pointer(mbed_stack_isr_start);
    
    DPL(" end: ");
    debug_print_pointer(mbed_stack_isr_start + mbed_stack_isr_size);
    
    DPL(" size: ");
    debug_print_u32(mbed_stack_isr_size);
    
#ifdef CAN_DEBUG_ISR_STACK_USAGE
    
    DPL(" used: ");
    debug_print_u32(calculate_isr_stack_usage());
    
#endif

    DPL(" )\r\n");

    // Print ISR stack contents.
    // print_memory_contents(&__StackLimit, &__StackTop);
}

#endif // DEVICE_SERIAL