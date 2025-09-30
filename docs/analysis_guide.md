# Cube96 Analysis Tooling Guide

This guide walks through the Python helpers shipped under [`analysis/`](../analysis/) for
performing lightweight differential and linear analysis on Cube96. All commands
assume you are in the repository root with Python 3 available.

## Difference Distribution & Linear Approximation Tables

`analysis/ddt_lat.py` recomputes the AES S-box difference distribution table
(DDT) and linear approximation table (LAT) that Cube96 inherits. The script
writes CSV files next to the script (or to a custom prefix) and prints a quick
summary of the extremal entries.

```sh
python3 analysis/ddt_lat.py
```

Expected output:

```
DDT written to aes_sbox_ddt.csv (max entry 256/256).
LAT written to aes_sbox_lat.csv (max bias 256/256).
```

To choose a different destination directory, pass `--output-prefix` (the script
will append `_ddt.csv`/`_lat.csv`).

## Differential Trail Sampling

`analysis/diff_trails.py` performs a branch-and-bound search for high-probability
differential characteristics. You control the number of rounds explored and the
branching factor for each expansion step.

```sh
python3 analysis/diff_trails.py --rounds 3 --branch 4 --layout zslice
```

Example output:

```
Analysed 3 rounds with branch factor 4.
Trail weight 20.00 (prob≈9.537e-07):
  Δ_in: 000000000000000000000001
  Δ_after_round_1: 000000000000000000001000
  Δ_after_round_2: 000000000000000000000080
  Δ_after_round_3: 888000000000000000004040
```

Additional trails follow with similar formatting.

The script lists the lightest trails it discovered in weight order (lighter
means higher probability). Increase `--rounds` to go deeper, `--branch` to keep
more candidates per layer, and `--layout rowmajor` to match an alternate build
configuration. Runs grow exponentially with larger parameters, so start small
and scale gradually.

## Linear Bias Monte Carlo Estimator

`analysis/linear_bias.py` estimates the bias of user-specified linear masks by
Monte Carlo sampling. Control the number of rounds, random trials, and RNG seed
for reproducibility.

```sh
python3 analysis/linear_bias.py --rounds 3 --samples 5000 --layout zslice --seed 1
```

Sample output:

```
Estimated bias +0.0204 (≈2^{-5.62}) over 5000 samples.
```

Increase `--samples` to tighten confidence intervals (runtime grows linearly).
The default masks examine a single input/output bit; pass `--mask-in`/`--mask-out`
with 24-hex-digit values to analyse custom linear approximations. As with the
differential tool, use `--layout rowmajor` if you built the cipher with the
alternate state mapping.

