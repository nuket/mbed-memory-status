# mbed-memory-status
Print thread and ISR stack locations and sizes, and heap location and size at runtime when using mbed OS. Useful for tracking down total runtime memory usage.

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

Using RTX RTOS, this will print something like:

```
    stack ( start: 20005100 end: 20005420 size: 00000320 used: 00000070 ) thread ( id: 2000542C entry: 00020A6D )
    stack ( start: 20003D1C end: 2000451C size: 00000800 used: 00000254 ) thread ( id: 2000546C entry: 0001FEBD )
    stack ( start: 20004E58 end: 20005058 size: 00000200 used: 00000050 ) thread ( id: 20005644 entry: 0001FF09 )
     heap ( start: 200056E8 end: 20007800 size: 00002118 )
isr_stack ( start: 20007800 end: 20008000 size: 00000800 )
```

## Use

```
mbed add https://github.com/nuket/mbed-memory-status.git mbed-memory-status
```

## Why

This code exists because of a stupid amount of bug-hunting:
> https://vilimpoc.org/blog/2017/02/01/stack-heap-and-thread-crash-hunting-in-mbed-os/
