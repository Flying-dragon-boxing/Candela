# Velocity matrix component selection

Set `readvmatrix 1` and use `vmatrix_components` in INPUT:

```text
wf_in_type ABACUS2
readvmatrix 1
vmatrix_components momentum,nonlocal
```

- `total` (default): read legacy `vmatrixN.dat` unchanged.
- `all`: sum all complex amplitudes from `vmatrix_componentsN.dat`, then square.
- Any comma-separated subset of `momentum`, `nonlocal`, `mgga` (no spaces).
  A correction by itself is allowed, for example `vmatrix_components mgga`.

Selection preserves interference between chosen operators. The component format
is supplied by the accompanying ABACUS PW Kubo-Greenwood changes. ABACUS writes
zero for corrections disabled or inapplicable during that calculation; Candela
cannot recover disabled corrections. EXX velocity is not currently implemented.

Component selection requires ABACUS/ABACUS2 input and `cond_intra 0` (files contain
interband pairs only). Invalid selections and missing, truncated, mismatched or
unsupported files fail with an error. All conductivity consumers use the common
WfRead reader, including both cond_method values and elecond_contribute.

Binary records use native endianness with 32-bit byte markers. The first record
contains three 32-bit integers `(version=1, nbands, ncomponents=3)`. Each of the
three following records (momentum, nonlocal, mgga) contains binary64 values in
pair order i<j, storing Re/Im of x,y,z for each pair.

Run the focused standalone C++11 reader test from the repository root:

```bash
g++ -std=c++11 -Wall -Wextra -pedantic -Isrc test/test_vmatrix_components.cpp -o /tmp/test_vmatrix_components
/tmp/test_vmatrix_components
```

The reader is header-only and included by input.cpp and wfRead.cpp. The test is
standalone; it does not require adding a production source to the Makefile.
