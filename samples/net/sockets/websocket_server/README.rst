.. _sockets-websocket-server-sample:

Socket Websocket Server
#######################

Overview
********

This sample application implements a Websocket server that will wait for an HTTP
or HTTPS handshake request from HTTP client, then start to send data and wait for
the responses from the Websocket client.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/sockets/websocket_server`.

Requirements
************

- :ref:`networking_with_host`

Building and Running
********************

You can use this application on a supported board, including
running it inside QEMU as described in :ref:`networking_with_qemu`.

Build websocket-client sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/websocket_server
   :board: <board to use>
   :conf: <config file to use>
   :goals: build
   :compact:

Enabling TLS support
====================

Enable TLS support in the sample by building the project with the
``overlay-tls.conf`` overlay file enabled using these commands:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/websocket_server
   :board: qemu_x86
   :conf: "prj.conf overlay-tls.conf"
   :goals: build
   :compact:

An alternative way is to specify ``-DOVERLAY_CONFIG=overlay-tls.conf`` when
running ``west build`` or ``cmake``.

The certificate and private key used by the sample can be found in the sample's
:zephyr_file:`samples/net/sockets/websocket_server/src/` directory.


Running websocket-client in Linux Host
======================================

You can run this ``websocket-server`` sample application in QEMU
and run the ***ToDo**...

To use QEMU for testing, follow the :ref:`networking_with_qemu` guide.

---> ToDo

Run ``websocket-server`` application in QEMU:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/websocket_server
   :host-os: unix
   :board: qemu_x86
   :conf: prj.conf
   :goals: run
   :compact:
