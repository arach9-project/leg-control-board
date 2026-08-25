Public data types
=================

The C-compatible types live in ``as5048a/types.h``.

``AS5048A_Angle_t``
   ``raw`` is the 14-bit count (0–16,383). ``degrees`` covers approximately
   0–359.978 degrees and ``radians`` approximately 0–2π, with zero inclusive
   and one full revolution exclusive.

``AS5048A_Magnitude_t``
   ``raw`` holds the 14-bit CORDIC magnitude value. Interpret expected ranges
   using the datasheet and measurements from the final magnetic assembly.

``AS5048A_Diagnostics_t``
   ``raw`` preserves the diagnostics register; ``agc`` is its 8-bit automatic
   gain value. The booleans expose offset-compensation completion, CORDIC
   overflow, magnet-too-strong, and magnet-too-weak state.

``AS5048A_Sample_t``
   Groups angle, magnitude, and diagnostics. ``valid`` is true only when
   ``sample()`` completed and accepted all diagnostic conditions.

