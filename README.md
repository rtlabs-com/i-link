i-link IO-Link master stack
===========================

Web resources
-------------

* Source repository: [https://github.com/rtlabs-com/i-link](https://github.com/rtlabs-com/i-link)
* Documentation: [https://rt-labs.com/docs/i-link](https://rt-labs.com/docs/i-link)
* Continuous integration: [https://github.com/rtlabs-com/i-link/actions](https://github.com/rtlabs-com/i-link/actions)
* RT-Labs (stack integration, certification services and training): [https://rt-labs.com](https://rt-labs.com)

[![Build Status](https://travis-ci.org/rtlabs-com/i-link.svg?branch=master)](https://travis-ci.org/rtlabs-com/i-link)

i-link
-----
The rt-labs IO-Link stack i-link is used for IO-Link master implementations. 
It is easy to use and provides a small footprint. 
It is especially well suited for embedded systems where resources are limited and efficiency is crucial.

It is written in C and can be run on bare-metal hardware, an RTOS such as rt-kernel, or on Linux. 
The i-link stack is supplied with full sources including a porting layer.

Also C++ (any version) is supported.

rt-labs i-link is developed according to specification 2.3:

 * Conformance Class A (Class B upon request)
 * Real Time Class 1

Features:

 * Porting layer provided
 * MAX14819 master transceiver supported

Limitations:

 * None

This software is dual-licensed, with GPL version 3 and a commercial license.
See LICENSE.md for more details.

Requirements
------------
cmake

* cmake 3.14 or later

For Linux:

* gcc 4.6 or later

For rt-kernel:

* Workbench 2018.1 or later

As an example of a microcontroller we have been using the Infineon XMC4800,
which has an ARM Cortex-M4 running at 144 MHz, with 2 MB Flash and 352 kB RAM.
It runs rt-kernel, and we have tested it with 2 MAX14819 chips, each with 2 IO-Link ports. 

Contributions
--------------
Contributions are welcome. If you want to contribute you will need to
sign a Contributor License Agreement and send it to us either by
e-mail or by physical mail. More information is available
on [https://rt-labs.com/contribution](https://rt-labs.com/contribution).
