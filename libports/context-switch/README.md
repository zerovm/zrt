# Context switching implementation library for ZeroVM

This is library for context switching under ZeroVM. It's based on *makecontext/getcontext* family functions (see [man makecontext](http://linux.die.net/man/3/makecontext))

It is the first implementation, barely tested. At the moment only integer registers are saved. No floating-point context is saved, sigprocmask also not saved. Just general purpose registers, SP, PC, etc.

## Installation

    https://github.com/rampage644/context-switch
    cd context-switch
    make

This will compile library and run small test suite.

You will get `libcontext.a` library which should be linked with your app. Current glibc [implementation][glibc] doesn't support context switching via _makecontext_ family funcions, only _setjmp/longjmp_ functions available. So make sure you linking this library before glibc.

## TODO

* Floating-point context switch OR clear
* Massive testing (hope gnu-pth will help)
* sigprocmask workaround (not supported under ZeroVM)
* **Insert into [glibc]** 

[glibc]:	https://github.com/zerovm/glibc