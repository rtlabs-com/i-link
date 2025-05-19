I-Link IO-Link master stack
===========================

The RT-Labs IO-Link stack I-Link is used for IO-Link master implementations.
It is easy to use and provides a small footprint. It is especially well suited
for embedded systems where resources are limited and efficiency is crucial.
It is written in C and can be run on bare-metal hardware, an RTOS such as
rt-kernel, or on Linux. The I-Link stack is supplied with full sources
including a porting layer.

Also, C++ (any version) is supported.

RT-Labs I-Link is developed according to specification 1.1.3.

Web resources
-------------

* Source repository: [https://github.com/rtlabs-com/i-link](https://github.com/rtlabs-com/i-link)
* Documentation: [https://rt-labs.com/docs/i-link](https://rt-labs.com/docs/i-link)
* RT-Labs (stack integration, certification services and training): [https://rt-labs.com](https://rt-labs.com)

Features
--------

* Porting layer provided
* MAX14819 master transceiver supported
* The sample application currently supports two different devices from IFM:
  * An RFID reader (IFM part number: DTI515)
  * A display device (IFM part number: E30430)

Limitations
-----------

* For support of other IO-Link devices, code have to be added to the application

License
-------

This software is dual-licensed, with GPL version 3 and a commercial license. See
LICENSE.md for more details.

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
It runs rt-kernel, and we have tested it with 2 MAX14819 chips, each with 2
IO-Link ports.

Contributions
-------------

Contributions are welcome. If you want to contribute you will need to sign a
Contributor License Agreement and send it to us either by e-mail or by physical
mail. More information is available
on [https://rt-labs.com/contribution](https://rt-labs.com/contribution).
