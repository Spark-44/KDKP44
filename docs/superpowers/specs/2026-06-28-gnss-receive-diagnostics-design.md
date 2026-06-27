# GNSS Receive Diagnostics Design

## Goal

Distinguish valid RMC/GGA reception from arbitrary newline-terminated UART data so a changing sequence counter cannot hide a GNSS parse failure.

## Design

- Count received bytes, complete NMEA lines, recognized RMC/GGA/THS lines, unknown lines, successful parses, and checksum or format failures.
- Safely reject short, oversized, unterminated, or checksum-less lines before parsing.
- Set the existing `gnss_flag` only for recognized RMC/GGA/THS sentences.
- Advance the portion-2 GPS sequence and position update only when `gnss_data_parse()` reports a successfully parsed RMC or GGA sentence.
- Emit one `[GNSS-DIAG]` line per second through the existing debug UART with counters, latest sentence type, fix, satellite count, coordinates, and parse result.

## Compatibility

UART3, 9600 baud, TAU1201 initialization commands, GNSS pin assignments, route recording thresholds, and GPS fusion behavior remain unchanged.

## Verification

Static regression checks require the counters, guarded parsing, success-only update, and rate-limited diagnostic output. The existing portion-2 verifier and C preprocessor checks must still pass.
