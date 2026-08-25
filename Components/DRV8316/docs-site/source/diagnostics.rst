Faults and diagnostics
======================

Poll the active-low pin without an SPI transaction, then refresh the cached
register snapshot::

   if (driver.isFaultPinAsserted() &&
       driver.refreshDiagnostics() == HAL_OK) {
       const DRV8316_Diagnostics_t &d = driver.diagnostics();
       const bool anyFault =
           (d.ic_status & DRV8316_IC_STATUS_FAULT_MASK) != 0U;
   }

``refreshDiagnostics()`` always reads ``IC_STATUS``. It reads ``STATUS_1`` and
``STATUS_2`` only when the summary indicates relevant IC, thermal, voltage,
overcurrent, SPI, or buck detail; otherwise it clears the cached detail bytes.
An invalid/unconfigured fault pin intentionally reads as not asserted.

Use ``clearFaults()`` only after the underlying condition is understood and
safe. Retry, latch, and report-only behavior also depends on configured device
protection settings.

