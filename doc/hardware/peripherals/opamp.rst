.. _opamp_api:

OpAmp
#####

Overview
********

An operational amplifier (OpAmp) is an analog device that amplifies the voltage
difference between its negative and positive inputs.
It is typically used in analog signal processing applications, such as filtering,
signal conditioning, and analog computation.
In Zephyr, the OpAmp API provides a way to configure and control embedded opamps.
It allows users to set up the OpAmp's configuration, read its output, and manage
power states.

Related configuration options:

* :kconfig:option:`CONFIG_OPAMP`

Configuration
*************

Embedded opamps can typically be configured at runtime. When enabled, an initial
configuration must be provided using the devicetree. At runtime, opamps can have their
configuration updated using device driver specific APIs. The configuration will be applied
when the OpAmp is resumed.

Typical configuration parameters include:
* Gain settings
* Input selection (positive and negative terminals)
* Output routing

Power management
****************

OpAmps are controlled using power management. When resumed, the OpAmp amplifies
the applied voltage based on its configuration. When suspended, the OpAmp enters
an inactive state to conserve power.

OpAmp shell
****************

The OpAmp shell provides the ``opamp`` command with a set of subcommands for the
:ref:`shell <shell_api>` module.

The ``opamp`` shell command provides the following subcommands:

* ``enable`` See :c:func:`opamp_enable`
* ``disable`` See :c:func:`opamp_disable`
* ``configure`` See :c:func:`opamp_configure`

Related configuration options:

* :kconfig:option:`CONFIG_SHELL`
* :kconfig:option:`CONFIG_OPAMP_SHELL`

.. note::
   The power management shell can optionally be enabled alongside the OpAmp shell.

   Related configuration options:

   * :kconfig:option:`CONFIG_PM_DEVICE`
   * :kconfig:option:`CONFIG_PM_DEVICE_SHELL`

API Reference
*************

.. doxygengroup:: opamp_interface

Example usage:
.. code-block:: shell

   opamp enable
   opamp disable
   opamp configure gain=10

Example devicetree configuration:
.. code-block:: dts

   opamp0: opamp@40010000 {
      compatible = "vendor,opamp";
      status = "okay";
      gain = <10>;
   };
