/** Sha256.cpp: SHA-256 as in FIPS 180-4, processing 64-byte blocks. */
#include "pier/dimensions/pack/sha256.h"

#include <cstring>

namespace pier::dimensions::pack
{
    namespace
    {
        constexpr std::uint32_t kK[64] = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
        };

        constexpr std::uint32_t rotr(std::uint32_t v, int n) { return (v >> n) | (v << (32 - n)); }

        void block(std::uint32_t h[8], std::uint8_t const* p)
        {
            std::uint32_t w[64];
            for (int i = 0; i < 16; ++i)
            {
                w[i] = (std::uint32_t(p[i * 4]) << 24) | (std::uint32_t(p[i * 4 + 1]) << 16)
                    | (std::uint32_t(p[i * 4 + 2]) << 8) | std::uint32_t(p[i * 4 + 3]);
            }
            for (int i = 16; i < 64; ++i)
            {
                std::uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
                std::uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
                w[i] = w[i - 16] + s0 + w[i - 7] + s1;
            }
            std::uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
            for (int i = 0; i < 64; ++i)
            {
                std::uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
                std::uint32_t ch = (e & f) ^ (~e & g);
                std::uint32_t t1 = hh + S1 + ch + kK[i] + w[i];
                std::uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
                std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
                std::uint32_t t2 = S0 + maj;
                hh = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
            }
            h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
        }
    } // namespace

    Sha256 sha256(std::uint8_t const* data, std::size_t len)
    {
        std::uint32_t h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                              0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
        std::size_t full = len / 64;
        for (std::size_t i = 0; i < full; ++i) block(h, data + i * 64);
        std::uint8_t tail[128];
        std::size_t rest = len - full * 64;
        std::memcpy(tail, data + full * 64, rest);
        tail[rest] = 0x80;
        std::size_t padded = rest + 1 <= 56 ? 64 : 128;
        std::memset(tail + rest + 1, 0, padded - rest - 1);
        std::uint64_t bits = std::uint64_t(len) * 8;
        for (int i = 0; i < 8; ++i) tail[padded - 1 - i] = std::uint8_t(bits >> (8 * i));
        block(h, tail);
        if (padded == 128) block(h, tail + 64);
        Sha256 out{};
        for (int i = 0; i < 8; ++i)
        {
            out[i * 4] = std::uint8_t(h[i] >> 24);
            out[i * 4 + 1] = std::uint8_t(h[i] >> 16);
            out[i * 4 + 2] = std::uint8_t(h[i] >> 8);
            out[i * 4 + 3] = std::uint8_t(h[i]);
        }
        return out;
    }

    std::string hex(Sha256 const& h)
    {
        static constexpr char digits[] = "0123456789abcdef";
        std::string s;
        s.reserve(64);
        for (auto b : h)
        {
            s.push_back(digits[b >> 4]);
            s.push_back(digits[b & 15]);
        }
        return s;
    }

    bool parseHex(std::string const& text, Sha256& out)
    {
        if (text.size() != 64) return false;
        auto nib = [](char c) -> int
        {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        };
        for (std::size_t i = 0; i < 32; ++i)
        {
            int hi = nib(text[i * 2]), lo = nib(text[i * 2 + 1]);
            if (hi < 0 || lo < 0) return false;
            out[i] = std::uint8_t(hi * 16 + lo);
        }
        return true;
    }
} // namespace pier::dimensions::pack
