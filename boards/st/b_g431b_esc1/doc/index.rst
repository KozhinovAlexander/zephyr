.. zephyr:board:: b_g431b_esc1

Overview
********

The B-G431B-ESC1 board features an Arm Cortex-M4 based STM32G431C8 MCU
with CAN, USB and external debugger connections. Here are some highlights:

- STM32 microcontroller in LQFP48 package
- USB Type-C connector (J1)
- CAN-Bus connector (J2)
- ST-LINK/V3E debugger/programmer header (J4)
- USB VBUS power supply (5 V)
- Three LEDs: red/power_led (D1), blue/stat_led (D2), green/word_led (D3)
- One push-button for RESET
- Development support: serial wire debug (SWD), JTAG, Embedded Trace Macrocell.

The LED red/power_led (D1) is connected directly to on-board 3.3 V and not controllable by the MCU.

More information about the board can be found at the `B-G431B-ESC1 website`_.
It is very advisable to take a look in on user manual `B-G431B-ESC1 User Manual`_ and
schematic `B-G431B-ESC1 schematic`_ before start.

More information about STM32G431C8 can be found here:

- `STM32G431C8 on www.st.com`_
- `STM32G4 reference manual`_

Supported Features
==================

The Zephyr ``b_g431b_esc1`` board target supports the following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| USB       | on-chip    | universal-serial-bus                |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| FDCAN     | on-chip    | can                                 |
+-----------+------------+-------------------------------------+

Other hardware features are not yet supported on this Zephyr port.

The default configuration can be found in the defconfig file:
:zephyr_file:`boards/st/b_g431b_esc1/b_g431b_esc1_defconfig`


Connections and IOs
===================

Default Zephyr Peripheral Mapping:
----------------------------------

.. rst-class:: rst-columns

- CAN_RX/BOOT0 : PB8
- CAN_TX : PB9
- D2 : PA15
- D3 : PA0
- USB_DN : PA11
- USB_DP : PA12
- SWDIO : PA13
- SWCLK : PA14
- NRST : PG10

For more details please refer to `B-G431B-ESC1 schematic`_.

System Clock
------------

The system clock is configured from the external 8 MHz crystal (HSE) using
the PLL to reach a 160 MHz system clock. FDCAN1 is clocked from PLLQ at
80 MHz.

Programming and Debugging
*************************

B-G431B-ESC1 board includes an SWDIO debug connector header J4.

.. note::

   The debugger is not the part of the board!

Applications for the ``b_g431b_esc1`` board target can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

The board could be flashed using west.

Flashing an application to B-G431B-ESC1
---------------------------------------

The debugger shall be wired to B-G431B-ESC1 board's J4 connector
according `B-G431B-ESC1 schematic`_.

Build and flash an application. Here is an example for
:zephyr:code-sample:`hello_world`.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: b_g431b_esc1
   :goals: build flash
   :west-args: -S rtt-console
   :compact:

The argument ``-S rtt-console`` is needed for debug purposes with SEGGER RTT protocol.
This option is optional and may be omitted. Omitting it frees up RAM space but prevents RTT usage.

If option ``-S rtt-console`` is selected, the connection to the target can be established as follows:

.. code-block:: console

   $ telnet localhost 9090

You should see the following message on the console:

.. code-block:: console

   $ Hello World! b_g431b_esc1/stm32g431xx

.. note::

   RTT availability may depend on OpenOCD version.

Debugging
=========

You can debug an application in the usual way. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: b_g431b_esc1
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _B-G431B-ESC1 website:
   https://www.st.com/en/evaluation-tools/b-g431b-esc1.html

.. _B-G431B-ESC1 User Manual:
   https://www.st.com/resource/en/user_manual/um2516-electronic-speed-controller-discovery-kit-for-drones-with-stm32g431cb-stmicroelectronics.pdf

.. _B-G431B-ESC1 schematic:
   https://www.st.com/resource/en/schematic_pack/mb1419-g431cbu6-c01_schematic.pdf

.. _STM32G431C8 on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32g431c8.html

.. _STM32G4 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
