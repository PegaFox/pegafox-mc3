
# ABI

## Calling convention

When calling a function, space for the return value will be pushed to the stack, followed by the return address, and then the function arguments at the top of the stack. The stack pointer is moved to point to the return address

When returning, the return value is written to the position just below the bottom of the stack frame, the program jumps to the return address stored at the bottom of the stack frame, and the stack pointer position is reverted to the old position
