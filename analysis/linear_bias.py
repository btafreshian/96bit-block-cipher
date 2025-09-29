#!/usr/bin/env python3
"""Monte Carlo estimator for linear biases of cube96."""

import argparse
import hashlib
import hmac
import math
import random
from dataclasses import dataclass
from typing import List, Sequence

AES_SBOX = [
    0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B,
    0xFE, 0xD7, 0xAB, 0x76, 0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0,
    0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0, 0xB7, 0xFD, 0x93, 0x26,
    0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
    0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2,
    0xEB, 0x27, 0xB2, 0x75, 0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0,
    0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84, 0x53, 0xD1, 0x00, 0xED,
    0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
    0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F,
    0x50, 0x3C, 0x9F, 0xA8, 0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5,
    0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2, 0xCD, 0x0C, 0x13, 0xEC,
    0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
    0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14,
    0xDE, 0x5E, 0x0B, 0xDB, 0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C,
    0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79, 0xE7, 0xC8, 0x37, 0x6D,
    0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
    0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F,
    0x4B, 0xBD, 0x8B, 0x8A, 0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E,
    0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E, 0xE1, 0xF8, 0x98, 0x11,
    0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
    0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F,
    0xB0, 0x54, 0xBB, 0x16,
]


@dataclass(frozen=True)
class Layout:
    name: str

    def idx_of(self, x: int, y: int, z: int) -> int:
        if self.name == "rowmajor":
            return 24 * y + 6 * x + z
        return 16 * z + 4 * y + x

    def byte_index_of_bit(self, bit_index: int) -> int:
        if self.name == "rowmajor":
            row = bit_index // 24
            offset = bit_index % 24
            return 3 * row + offset // 8
        z = bit_index // 16
        offset = bit_index % 16
        return 2 * z + offset // 8

    def bit_offset_in_byte(self, bit_index: int) -> int:
        if self.name == "rowmajor":
            return 7 - (bit_index % 8)
        offset = bit_index % 16
        return 7 - (offset % 8)


class SplitMix64:
    def __init__(self, seed: int) -> None:
        self.state = seed & 0xFFFFFFFFFFFFFFFF

    def next(self) -> int:
        self.state = (self.state + 0x9E3779B97F4A7C15) & 0xFFFFFFFFFFFFFFFF
        z = self.state
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9 & 0xFFFFFFFFFFFFFFFF
        z = (z ^ (z >> 27)) * 0x94D049BB133111EB & 0xFFFFFFFFFFFFFFFF
        return (z ^ (z >> 31)) & 0xFFFFFFFFFFFFFFFF


def identity_perm() -> List[int]:
    return list(range(96))


def compose_perm(a: Sequence[int], b: Sequence[int]) -> List[int]:
    return [b[idx] for idx in a]


def primitive_set(layout: Layout) -> List[List[int]]:
    prims: List[List[int]] = []

    def face_rotation(z: int, variant: int) -> List[int]:
        perm = identity_perm()
        for y in range(4):
            for x in range(4):
                if variant == 0:
                    nx, ny = 3 - y, x
                elif variant == 1:
                    nx, ny = y, 3 - x
                else:
                    nx, ny = 3 - x, 3 - y
                src = layout.idx_of(x, y, z)
                dst = layout.idx_of(nx, ny, z)
                perm[src] = dst
        return perm

    def row_cycle(z: int, up: bool) -> List[int]:
        perm = identity_perm()
        for y in range(4):
            ny = (y + (1 if up else 3)) & 3
            for x in range(4):
                src = layout.idx_of(x, y, z)
                dst = layout.idx_of(x, ny, z)
                perm[src] = dst
        return perm

    def col_cycle(z: int) -> List[int]:
        perm = identity_perm()
        for x in range(4):
            nx = (x + 1) & 3
            for y in range(4):
                src = layout.idx_of(x, y, z)
                dst = layout.idx_of(nx, y, z)
                perm[src] = dst
        return perm

    def x_slice_shift(x: int) -> List[int]:
        perm = identity_perm()
        for y in range(4):
            for z in range(6):
                nz = (z + 1) % 6
                src = layout.idx_of(x, y, z)
                dst = layout.idx_of(x, y, nz)
                perm[src] = dst
        return perm

    def y_slice_shift(y: int) -> List[int]:
        perm = identity_perm()
        for x in range(4):
            for z in range(6):
                nz = (z + 1) % 6
                src = layout.idx_of(x, y, z)
                dst = layout.idx_of(x, y, nz)
                perm[src] = dst
        return perm

    for z in range(6):
        prims.append(face_rotation(z, 0))
        prims.append(face_rotation(z, 1))
        prims.append(face_rotation(z, 2))
    for z in range(4):
        prims.append(row_cycle(z, True))
        prims.append(row_cycle(z, False))
        prims.append(col_cycle(z))
    for x in range(3):
        prims.append(x_slice_shift(x))
    for y in range(3):
        prims.append(y_slice_shift(y))

    return prims


def hkdf_expand(key: bytes, info: bytes, length: int) -> bytes:
    out = b""
    block = b""
    counter = 1
    while len(out) < length:
        block = hmac.new(key, block + info + bytes([counter]), hashlib.sha256).digest()
        out += block
        counter += 1
    return out[:length]


def derive_permutations(key: bytes, layout: Layout) -> List[List[int]]:
    salt = b"StagedCube's-96-HKDF-V1".ljust(32, b"\x00")
    info = b"Cube96-RK-PS-Post-v1"
    prk = hmac.new(salt, key, hashlib.sha256).digest()
    okm = hkdf_expand(prk, info, 172)
    seeds = [int.from_bytes(okm[96 + 8 * r : 96 + 8 * (r + 1)], "big") for r in range(8)]

    prims = primitive_set(layout)
    per_round: List[List[int]] = []
    for seed in seeds:
        prng = SplitMix64(seed)
        perm = identity_perm()
        for _ in range(12):
            pick = prng.next() % len(prims)
            perm = compose_perm(perm, prims[pick])
        per_round.append(perm)
    return per_round


def apply_permutation(state: Sequence[int], perm: Sequence[int], layout: Layout) -> List[int]:
    out = [0] * len(state)
    for src_bit in range(96):
        src_byte = layout.byte_index_of_bit(src_bit)
        bit_pos = layout.bit_offset_in_byte(src_bit)
        bit = (state[src_byte] >> bit_pos) & 1
        if bit:
            dst_bit = perm[src_bit]
            dst_byte = layout.byte_index_of_bit(dst_bit)
            dst_pos = layout.bit_offset_in_byte(dst_bit)
            out[dst_byte] |= 1 << dst_pos
    return out


def encrypt_rounds(block: Sequence[int], perms: Sequence[Sequence[int]], rounds: int, layout: Layout) -> List[int]:
    state = list(block)
    for r in range(rounds):
        state = [AES_SBOX[b] for b in state]
        state = apply_permutation(state, perms[r], layout)
    return state


def bit_parity(value: int) -> int:
    return bin(value).count("1") & 1


def mask_parity(state: Sequence[int], mask: Sequence[int]) -> int:
    total = 0
    for s, m in zip(state, mask):
        total ^= bit_parity(s & m)
    return total


def bytes_from_hex(s: str) -> bytes:
    s = s.replace(" ", "")
    if len(s) % 2:
        s = "0" + s
    return bytes.fromhex(s)


def estimate_bias(perms: Sequence[Sequence[int]],
                  rounds: int,
                  samples: int,
                  mask_in: Sequence[int],
                  mask_out: Sequence[int],
                  layout: Layout,
                  rng: random.Random) -> float:
    balanced = 0
    for _ in range(samples):
        block = [rng.getrandbits(8) for _ in range(12)]
        parity_in = mask_parity(block, mask_in)
        out_state = encrypt_rounds(block, perms, rounds, layout)
        parity_out = mask_parity(out_state, mask_out)
        balanced += 1 if parity_in == parity_out else -1
    return balanced / samples


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--key", default="000000000000000000000000", help="96-bit key as hex.")
    parser.add_argument("--rounds", type=int, default=4, help="Number of rounds to evaluate (≤8).")
    parser.add_argument("--samples", type=int, default=200000, help="Monte Carlo sample size.")
    parser.add_argument("--layout", choices=["zslice", "rowmajor"], default="zslice")
    parser.add_argument("--mask-in", default="000000000000000000000001", help="Input mask (24 hex chars).")
    parser.add_argument("--mask-out", default="000000000000000000000001", help="Output mask (24 hex chars).")
    parser.add_argument("--seed", type=int, default=0x12345678, help="RNG seed for reproducibility.")
    args = parser.parse_args()

    key_bytes = bytes_from_hex(args.key)
    if len(key_bytes) != 12:
        raise ValueError("Key must be exactly 96 bits.")
    layout = Layout(args.layout)
    perms = derive_permutations(key_bytes, layout)
    rounds = min(max(args.rounds, 1), len(perms))

    mask_in = list(bytes_from_hex(args.mask_in).rjust(12, b"\x00"))
    mask_out = list(bytes_from_hex(args.mask_out).rjust(12, b"\x00"))

    rng = random.Random(args.seed)
    bias = estimate_bias(perms, rounds, args.samples, mask_in, mask_out, layout, rng)
    abs_bias = abs(bias)
    if abs_bias > 0:
        log_bias = -math.log(abs_bias, 2)
        print(f"Estimated bias {bias:+.4f} (≈2^{{-{log_bias:.2f}}}) over {args.samples} samples.")
    else:
        print(f"Estimated bias {bias:+.4f} over {args.samples} samples (no deviation observed).")


if __name__ == "__main__":
    main()
