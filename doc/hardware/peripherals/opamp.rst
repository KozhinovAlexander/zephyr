.. _opamp_api:

OpAmp
#####

Overview
********

An analog opamp compares the voltages of two analog signals connected to its negative and
positive inputs. If the voltage at the positive input is higher than the negative input, the
opamp's output will be high, otherwise, it will be low.

Comparators can typically set a trigger which triggers on output changes. This trigger can
either invoke a callback, or its status can be polled.

Related configuration options:

* :kconfig:option:`CONFIG_OPAMP`

Configuration
*************

Embedded opamps can typically be configured at runtime. When enabled, an initial
configuration must be provided using the devicetree. At runtime, opamps can have their
configuration updated using device driver specific APIs. The configuration will be applied
when the opamp is resumed.

Power management
****************

Comparators are enabled using power management. When resumed, the opamp will actively
compare its inputs, producing an output and detecting edges. When suspended, the opamp
will be inactive.

Comparator shell
****************

The opamp shell provides the ``comp`` command with a set of subcommands for the
:ref:`shell <shell_api>` module.

The ``comp`` shell command provides the following subcommands:

* ``get_output`` See :c:func:`opamp_get_output`
* ``set_trigger`` See :c:func:`opamp_set_trigger`
* ``await_trigger`` Awaits trigger using the following flow:

  * Set trigger callback using :c:func:`opamp_set_trigger_callback`
  * Await callback or time out after default or optionally provided timeout
  * Clear trigger callback using :c:func:`opamp_set_trigger_callback`
* ``trigger_is_pending`` See :c:func:`opamp_trigger_is_pending`

Related configuration options:

* :kconfig:option:`CONFIG_SHELL`
* :kconfig:option:`CONFIG_OPAMP_SHELL`
* :kconfig:option:`CONFIG_OPAMP_SHELL_AWAIT_TRIGGER_DEFAULT_TIMEOUT`
* :kconfig:option:`CONFIG_OPAMP_SHELL_AWAIT_TRIGGER_MAX_TIMEOUT`

.. note::
   The power management shell can optionally be enabled alongside the opamp shell.

   Related configuration options:

   * :kconfig:option:`CONFIG_PM_DEVICE`
   * :kconfig:option:`CONFIG_PM_DEVICE_SHELL`

API Reference
*************

.. doxygengroup:: opamp_interface
