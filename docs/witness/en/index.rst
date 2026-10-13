ESP-BIST Witness Test Report
=============================

The Witness Test chapter documents the certification process for validating the ESP-BIST v1.1.0 library on physical hardware. This process involves submitting a development board connected to JTAG to several error injection scenarios to certify that the BIST library correctly detects and responds to hardware faults as expected.

The certification process demonstrates that the BIST library can reliably detect hardware failures and that the system behaves appropriately when errors are injected, providing evidence that the library meets its safety requirements in real-world conditions.

.. toctree::
    :maxdepth: 2

    Witness Test Report <witness_test_report>
