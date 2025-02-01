.. zephyr:code-sample:: comparator_sample
   :name: Comparator sample
   :relevant-api: comparator, gpio_interface

   Use comparator functionality and print state to the console

Overview
********

This sample is a minimum application to demonstrate basic behavior of a basic
:ref:`COMP driver API <comparator_api>` set up.

The comparator has following configuration:

- positive input is connected to one of the MCU prints
- negative input is connected to 1/4 V_ref
- the output is non-inverting
- raising and falling edge trigger event is enabled

The main thread reads periodically the value of comparator output and prints it out.
Additionally the main thread toggles led0 GPIO each 3rd run.
It is very helpful to connect comparator's positive input directly to GPIO driving
led0. In this case one can see comparator state change every 3rd main loop run
directly in the console.

.. _comparator-sample-requirements:

Requirements
************

The board should support COMP and GPIO.

Building and Running
********************

Build and flash as follows, changing ``nucleo_h745zi_q`` for your board:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/comparator
   :board: nucleo_h745zi_q/stm32h745xx/m7
   :goals: build flash
   :compact:

Sample output
=============

You should get a similar output as below, repeated every second:

.. code-block:: console

   - comparator@5800380c output: 1
