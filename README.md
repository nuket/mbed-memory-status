# mbed-memory-status
Print thread stack, ISR stack, and global heap locations, sizes, and utilization at runtime when using mbed OS. Useful for tracking down total runtime memory usage and stack overflows.

Does *not* use printf(). It *will* automatically initialize the default serial port to 115200 8N1 using the low-level mbed `serial_api.h` if no other instantiation has occurred.

## Example
```
int main()
{
    print_all_thread_info();
    print_heap_and_isr_stack_info();
}
```

## Output

Using ARM RTX RTOS on mbed, this will print something like:

```
    stack ( start: 20005100 end: 20005420 size: 00000320 used: 00000070 ) thread ( id: 2000542C entry: 00020D91 )
    stack ( start: 20003D1C end: 2000451C size: 00000800 used: 00000254 ) thread ( id: 2000546C entry: 000201E1 )
    stack ( start: 20004E58 end: 20005058 size: 00000200 used: 00000050 ) thread ( id: 20005644 entry: 0002022D )
     heap ( start: 200056E8 end: 20007800 size: 00002118 used: 00000398 )  alloc ( ok: 00000006  fail: 00000000 )
isr_stack ( start: 20007800 end: 20008000 size: 00000800 used: 000002B0 )
```

## Use

Add to your program:

```
mbed add https://github.com/nuket/mbed-memory-status.git mbed-memory-status
```

To enable the ISR stack usage tracking, the following assembly code needs to be added to your chip's specific `startup_[your chip].S` file:

```asm
LDR     R0, =fill_isr_stack_with_canary
BLX     R0
```

See the `startup_example.S.txt` file for what this looks like (the file is a modified copy of `startup_NRF51822.S`).

Then define this in `mbed_memory_status.c`, or via the `mbed_app.json` macros, or via the command line:

```c
#define CAN_DEBUG_ISR_STACK_USAGE
```

## Why

This code exists because of a stupid amount of bug-hunting:
> https://vilimpoc.org/blog/2017/02/01/stack-heap-and-thread-crash-hunting-in-mbed-os/
> https://vilimpoc.org/blog/2017/02/04/isr-stack-usage-on-mbed/
