"""java_noise.py: the random sources and noise samplers of Java Edition worldgen.

The tool builds every permutation table and origin offset with these, so the C++ side
never needs a random source for the terrain shape. The evaluators here are the reference
the C++ sampler must match: ImprovedNoise (3D Perlin with the yScale/yMax trick),
PerlinNoise (octaves), NormalNoise (two samplers, the second at 1.0181268882301 times the
input), SimplexNoise (2D, for end islands) and BlendedNoise (the 1.18 base 3D noise).
"""
from __future__ import annotations

import hashlib
import math
import struct

MASK64 = (1 << 64) - 1
MASK48 = (1 << 48) - 1


def _s64(v: int) -> int:
    v &= MASK64
    return v - (1 << 64) if v >> 63 else v


def _s32(v: int) -> int:
    v &= 0xFFFFFFFF
    return v - (1 << 32) if v >> 31 else v


def _rotl(v: int, n: int) -> int:
    v &= MASK64
    return ((v << n) | (v >> (64 - n))) & MASK64


def mix_stafford13(z: int) -> int:
    z = ((z ^ (z >> 30)) * 0xBF58476D1CE4E5B9) & MASK64
    z = ((z ^ (z >> 27)) * 0x94D049BB133111EB) & MASK64
    return z ^ (z >> 31)


def upgrade_seed_to_128(seed: int) -> tuple[int, int]:
    lo = (seed ^ 0x6A09E667F3BCC909) & MASK64
    hi = (lo + 0x9E3779B97F4A7C15) & MASK64
    return mix_stafford13(lo), mix_stafford13(hi)


def seed_from_hash_of(name: str) -> tuple[int, int]:
    d = hashlib.md5(name.encode("utf-8")).digest()
    lo = struct.unpack(">Q", d[:8])[0]
    hi = struct.unpack(">Q", d[8:])[0]
    return lo, hi


def java_string_hash(s: str) -> int:
    """String.hashCode over UTF-16 code units."""
    h = 0
    for unit in _utf16_units(s):
        h = (31 * h + unit) & 0xFFFFFFFF
    return _s32(h)


def _utf16_units(s: str):
    b = s.encode("utf-16-be")
    for i in range(0, len(b), 2):
        yield (b[i] << 8) | b[i + 1]


class Xoroshiro:
    """XoroshiroRandomSource."""

    def __init__(self, lo: int, hi: int):
        lo &= MASK64
        hi &= MASK64
        if (lo | hi) == 0:
            lo, hi = 0x9E3779B97F4A7C15, 0x6A09E667F3BCC909
        self.lo, self.hi = lo, hi

    @staticmethod
    def from_seed(seed: int) -> "Xoroshiro":
        lo, hi = upgrade_seed_to_128(seed & MASK64)
        return Xoroshiro(lo, hi)

    def next_long(self) -> int:
        l, m = self.lo, self.hi
        n = (_rotl((l + m) & MASK64, 17) + l) & MASK64
        m ^= l
        self.lo = (_rotl(l, 49) ^ m ^ ((m << 21) & MASK64)) & MASK64
        self.hi = _rotl(m, 28)
        return n

    def next_bits(self, bits: int) -> int:
        return self.next_long() >> (64 - bits)

    def next_int(self, bound: int | None = None) -> int:
        if bound is None:
            return _s32(self.next_long())
        if bound <= 0:
            raise ValueError("bound must be positive")
        r = (self.next_long() & 0xFFFFFFFF) * bound
        lo = r & 0xFFFFFFFF
        if lo < bound:
            j = ((~bound + 1) & 0xFFFFFFFF) % bound
            while lo < j:
                r = (self.next_long() & 0xFFFFFFFF) * bound
                lo = r & 0xFFFFFFFF
        return r >> 32

    def next_double(self) -> float:
        return float(self.next_long() >> 11) * 1.1102230246251565e-16

    def next_float(self) -> float:
        return f32(float(self.next_long() >> 40) * 5.9604645e-8)

    def consume(self, n: int) -> None:
        for _ in range(n):
            self.next_long()

    def fork_positional(self) -> "XoroshiroPositional":
        return XoroshiroPositional(self.next_long(), self.next_long())


class XoroshiroPositional:
    def __init__(self, lo: int, hi: int):
        self.lo, self.hi = lo & MASK64, hi & MASK64

    def from_hash_of(self, name: str) -> Xoroshiro:
        lo, hi = seed_from_hash_of(name)
        return Xoroshiro(lo ^ self.lo, hi ^ self.hi)

    def at(self, x: int, y: int, z: int) -> Xoroshiro:
        return Xoroshiro(mth_get_seed(x, y, z) ^ self.lo, self.hi)


def mth_get_seed(x: int, y: int, z: int) -> int:
    l = (_s64(_s32(x * 3129871)) ^ (_s64(z) * 116129781) ^ _s64(y)) & MASK64
    l = (l * l * 42317861 + l * 11) & MASK64
    return (_s64(l) >> 16) & MASK64


class LegacyRandom:
    """LegacyRandomSource, the 48-bit LCG of java.util.Random."""

    def __init__(self, seed: int):
        self.seed = (seed ^ 0x5DEECE66D) & MASK48

    def next(self, bits: int) -> int:
        self.seed = (self.seed * 0x5DEECE66D + 0xB) & MASK48
        return _s32(self.seed >> (48 - bits))

    def next_int(self, bound: int | None = None) -> int:
        if bound is None:
            return self.next(32)
        if bound <= 0:
            raise ValueError("bound must be positive")
        if (bound & -bound) == bound:
            return _s32((bound * self.next(31)) >> 31)
        while True:
            bits = self.next(31)
            val = bits % bound
            if bits - val + (bound - 1) < (1 << 31):
                return val

    def next_long(self) -> int:
        return _s64((self.next(32) << 32) + self.next(32))

    def next_double(self) -> float:
        return float((self.next(26) << 27) + self.next(27)) * 1.1102230246251565e-16

    def next_float(self) -> float:
        return f32(self.next(24) / float(1 << 24))

    def consume(self, n: int) -> None:
        for _ in range(n):
            self.next_int()

    def fork_positional(self) -> "LegacyPositional":
        return LegacyPositional(self.next_long())


class LegacyPositional:
    def __init__(self, seed: int):
        self.seed = seed & MASK64

    def from_hash_of(self, name: str) -> LegacyRandom:
        return LegacyRandom(self.seed ^ (java_string_hash(name) & MASK64))

    def at(self, x: int, y: int, z: int) -> LegacyRandom:
        return LegacyRandom(mth_get_seed(x, y, z) ^ self.seed)


def f32(x: float) -> float:
    return struct.unpack("f", struct.pack("f", x))[0]


GRADIENT = [
    (1, 1, 0), (-1, 1, 0), (1, -1, 0), (-1, -1, 0), (1, 0, 1), (-1, 0, 1), (1, 0, -1), (-1, 0, -1),
    (0, 1, 1), (0, -1, 1), (0, 1, -1), (0, -1, -1), (1, 1, 0), (0, -1, 1), (-1, 1, 0), (0, -1, -1),
]


def _shuffled_perm(random) -> list:
    p = list(range(256))
    for i in range(256):
        j = random.next_int(256 - i)
        p[i], p[i + j] = p[i + j], p[i]
    return p


class NoiseTable:
    """One ImprovedNoise or SimplexNoise: a permutation and three offsets."""

    def __init__(self, random=None, perm=None, xo=0.0, yo=0.0, zo=0.0):
        if random is not None:
            self.xo = random.next_double() * 256.0
            self.yo = random.next_double() * 256.0
            self.zo = random.next_double() * 256.0
            self.perm = _shuffled_perm(random)
        else:
            self.perm, self.xo, self.yo, self.zo = list(perm), xo, yo, zo

    def p(self, i: int) -> int:
        return self.perm[i & 0xFF]


IDENTITY_TABLE = NoiseTable(perm=list(range(256)))


def smoothstep(x: float) -> float:
    return x * x * x * (x * (x * 6.0 - 15.0) + 10.0)


def lerp(t, a, b):
    return a + t * (b - a)


def improved_noise(t: NoiseTable, x: float, y: float, z: float, y_scale: float = 0.0, y_max: float = 0.0) -> float:
    d, e, f = x + t.xo, y + t.yo, z + t.zo
    i, j, k = math.floor(d), math.floor(e), math.floor(f)
    g, h, l = d - i, e - j, f - k
    if y_scale != 0.0:
        n = y_max if (y_max >= 0.0 and y_max < h) else h
        m = math.floor(n / y_scale + 1.0000000116860974e-07) * y_scale
    else:
        m = 0.0
    return _sample_and_lerp(t, i, j, k, g, h - m, l, h)


def _grad_dot(hash_: int, x: float, y: float, z: float) -> float:
    gx, gy, gz = GRADIENT[hash_ & 15]
    return gx * x + gy * y + gz * z


def _sample_and_lerp(t, i, j, k, x, y, z, y_frac):
    p = t.p
    l = p(i)
    m = p(i + 1)
    n = p(l + j)
    o = p(l + j + 1)
    q = p(m + j)
    r = p(m + j + 1)
    d = _grad_dot(p(n + k), x, y, z)
    e = _grad_dot(p(q + k), x - 1.0, y, z)
    f = _grad_dot(p(o + k), x, y - 1.0, z)
    g = _grad_dot(p(r + k), x - 1.0, y - 1.0, z)
    h = _grad_dot(p(n + k + 1), x, y, z - 1.0)
    s = _grad_dot(p(q + k + 1), x - 1.0, y, z - 1.0)
    tt = _grad_dot(p(o + k + 1), x, y - 1.0, z - 1.0)
    u = _grad_dot(p(r + k + 1), x - 1.0, y - 1.0, z - 1.0)
    v, w, aa = smoothstep(x), smoothstep(y_frac), smoothstep(z)
    return lerp(aa, lerp(w, lerp(v, d, e), lerp(v, f, g)), lerp(w, lerp(v, h, s), lerp(v, tt, u)))


def wrap(v: float) -> float:
    return v - math.floor(v / 3.3554432e7 + 0.5) * 3.3554432e7


class PerlinNoise:
    """PerlinNoise: octaves with amplitudes; tables[i] is None for a zero amplitude."""

    def __init__(self, first_octave: int, amplitudes: list, tables: list):
        self.first_octave = first_octave
        self.amplitudes = list(amplitudes)
        self.tables = list(tables)
        n = len(amplitudes)
        self.lowest_freq_input_factor = math.pow(2.0, first_octave)
        self.lowest_freq_value_factor = math.pow(2.0, n - 1) / (math.pow(2.0, n) - 1.0)

    @staticmethod
    def create(random, first_octave: int, amplitudes: list) -> "PerlinNoise":
        """The new random path: one positional factory, an octave per named child."""
        factory = random.fork_positional()
        tables = []
        for k, a in enumerate(amplitudes):
            tables.append(NoiseTable(factory.from_hash_of("octave_%d" % (first_octave + k))) if a != 0.0 else None)
        return PerlinNoise(first_octave, amplitudes, tables)

    @staticmethod
    def create_legacy(random, first_octave: int, amplitudes: list) -> "PerlinNoise":
        """The sequential path of blended noise and the legacy nether biome noises."""
        n = len(amplitudes)
        j = -first_octave
        tables = [None] * n
        first = NoiseTable(random)
        if 0 <= j < n and amplitudes[j] != 0.0:
            tables[j] = first
        for k in range(j - 1, -1, -1):
            if k < n:
                if amplitudes[k] != 0.0:
                    tables[k] = NoiseTable(random)
                else:
                    random.consume(262)
            else:
                random.consume(262)
        if j < n - 1:
            raise ValueError("positive octaves are not supported by the legacy path")
        return PerlinNoise(first_octave, amplitudes, tables)

    def octave(self, i: int):
        """Java's getOctaveNoise: counted from the end."""
        return self.tables[len(self.tables) - 1 - i]

    def value(self, x: float, y: float, z: float, y_scale: float = 0.0, y_max: float = 0.0, use_origin: bool = False) -> float:
        result = 0.0
        freq = self.lowest_freq_input_factor
        amp = self.lowest_freq_value_factor
        for i, t in enumerate(self.tables):
            if t is not None:
                v = improved_noise(t, wrap(x * freq), -t.yo if use_origin else wrap(y * freq), wrap(z * freq), y_scale * freq, y_max * freq)
                result += self.amplitudes[i] * v * amp
            freq *= 2.0
            amp /= 2.0
        return result


INPUT_FACTOR = 1.0181268882301


class NormalNoise:
    def __init__(self, first: PerlinNoise, second: PerlinNoise, amplitudes: list):
        self.first, self.second = first, second
        nz = [k for k, a in enumerate(amplitudes) if a != 0.0]
        span = (max(nz) - min(nz)) if nz else 1
        self.value_factor = 0.16666666666666666 / (0.1 * (1.0 + 1.0 / (span + 1)))

    @staticmethod
    def create(random, first_octave: int, amplitudes: list, legacy: bool = False) -> "NormalNoise":
        if legacy:
            a = PerlinNoise.create_legacy(random, first_octave, amplitudes)
            b = PerlinNoise.create_legacy(random, first_octave, amplitudes)
        else:
            a = PerlinNoise.create(random, first_octave, amplitudes)
            b = PerlinNoise.create(random, first_octave, amplitudes)
        return NormalNoise(a, b, amplitudes)

    def value(self, x: float, y: float, z: float) -> float:
        return (self.first.value(x, y, z) + self.second.value(x * INPUT_FACTOR, y * INPUT_FACTOR, z * INPUT_FACTOR)) * self.value_factor


F2 = 0.5 * (math.sqrt(3.0) - 1.0)
G2 = (3.0 - math.sqrt(3.0)) / 6.0


def simplex_2d(t: NoiseTable, x: float, z: float) -> float:
    d = (x + z) * F2
    i, j = math.floor(x + d), math.floor(z + d)
    e = (i + j) * G2
    f, g = i - e, j - e
    h, k = x - f, z - g
    if h > k:
        l, m = 1, 0
    else:
        l, m = 0, 1
    n, o = h - l + G2, k - m + G2
    p_, q = h - 1.0 + 2.0 * G2, k - 1.0 + 2.0 * G2
    r, s = i & 0xFF, j & 0xFF
    tt = t.p(r + t.p(s)) % 12
    u = t.p(r + l + t.p(s + m)) % 12
    v = t.p(r + 1 + t.p(s + 1)) % 12
    return 70.0 * (_corner(tt, h, k) + _corner(u, n, o) + _corner(v, p_, q))


def _corner(grad: int, x: float, y: float) -> float:
    e = 0.5 - x * x - y * y
    if e < 0.0:
        return 0.0
    e *= e
    gx, gy, _ = GRADIENT[grad]
    return e * e * (gx * x + gy * y)


def end_island_height(t: NoiseTable, i: int, j: int) -> float:
    """TheEndIslandDensityFunction.getHeightValue; i, j already divided by 8 as ints."""
    k, l = int(i / 2), int(j / 2)
    m, n = int(math.fmod(i, 2)), int(math.fmod(j, 2))
    f = f32(100.0 - f32(math.sqrt(f32(i * i + j * j))) * 8.0)
    f = max(-100.0, min(80.0, f))
    for o in range(-12, 13):
        for p in range(-12, 13):
            q, r = k + o, l + p
            if q * q + r * r > 4096 and simplex_2d(t, float(q), float(r)) < -0.8999999761581421:
                g = f32(math.fmod(f32(abs(q) * 3439.0 + abs(r) * 147.0), 13.0) + 9.0)
                h, s = float(m - o * 2), float(n - p * 2)
                tt = f32(100.0 - f32(math.sqrt(f32(h * h + s * s))) * g)
                tt = max(-100.0, min(80.0, tt))
                f = max(f, tt)
    return f


class BlendedNoise:
    """The 1.18 base 3D noise: two limit samplers of 16 octaves and a main of 8."""

    def __init__(self, min_limit: PerlinNoise, max_limit: PerlinNoise, main: PerlinNoise,
                 xz_scale: float, y_scale: float, xz_factor: float, y_factor: float, smear: float):
        self.min_limit, self.max_limit, self.main = min_limit, max_limit, main
        self.xz_multiplier = 684.412 * xz_scale
        self.y_multiplier = 684.412 * y_scale
        self.xz_factor, self.y_factor, self.smear = xz_factor, y_factor, smear

    @staticmethod
    def create(random, xz_scale, y_scale, xz_factor, y_factor, smear) -> "BlendedNoise":
        lim = [1.0] * 16
        main = [1.0] * 8
        a = PerlinNoise.create_legacy(random, -15, lim)
        b = PerlinNoise.create_legacy(random, -15, lim)
        c = PerlinNoise.create_legacy(random, -7, main)
        return BlendedNoise(a, b, c, xz_scale, y_scale, xz_factor, y_factor, smear)

    def value(self, bx: int, by: int, bz: int) -> float:
        d = bx * self.xz_multiplier
        e = by * self.y_multiplier
        f = bz * self.xz_multiplier
        g, h, i = d / self.xz_factor, e / self.y_factor, f / self.xz_factor
        j = self.y_multiplier * self.smear
        k = j / self.y_factor
        l = m = n = 0.0
        o = 1.0
        for p in range(8):
            t = self.main.octave(p)
            if t is not None:
                n += improved_noise(t, wrap(g * o), wrap(h * o), wrap(i * o), k * o, h * o) / o
            o /= 2.0
        q = (n / 10.0 + 1.0) / 2.0
        bl2, bl3 = q >= 1.0, q <= 0.0
        o = 1.0
        for r in range(16):
            s, tt, u = wrap(d * o), wrap(e * o), wrap(f * o)
            v = j * o
            if not bl2:
                t2 = self.min_limit.octave(r)
                if t2 is not None:
                    l += improved_noise(t2, s, tt, u, v, e * o) / o
            if not bl3:
                t3 = self.max_limit.octave(r)
                if t3 is not None:
                    m += improved_noise(t3, s, tt, u, v, e * o) / o
            o /= 2.0
        return clamped_lerp(l / 512.0, m / 512.0, q) / 128.0


def clamped_lerp(a: float, b: float, t: float) -> float:
    if t < 0.0:
        return a
    if t > 1.0:
        return b
    return lerp(t, a, b)
