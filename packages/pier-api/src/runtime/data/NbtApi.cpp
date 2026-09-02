/** runtime/data/NbtApi.cpp: binary NBT encoding and decoding.
 *
 * The whole SNBT object model lives on the SDK side. Only the binary format needs the
 * engine codec, so these two calls are all that cross the boundary. */
#include <string>
#include <string_view>

#include "mc/deps/nbt/CompoundTag.h"

#include "sdk/abi.h"

#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/str.h"

namespace pier::api_impl
{
    namespace
    {
        bool api_nbt_snbt_to_binary(PierStr snbt, int32_t fmt, void* ctx, PierBytesSink sink)
        {
            PIER_API_GUARD_BEGIN
                if (!sink) return false;
                auto tag = CompoundTag::fromSnbt(sv(snbt));
                if (!tag) return false;
                std::string bytes;
                if (fmt == 1)
                {
                    bytes = tag->toNetworkNbt();
                }
                else
                {
                    bytes = tag->toBinaryNbt(/*isLittleEndian*/ true);
                }
                sink(ctx, reinterpret_cast<uint8_t const*>(bytes.data()), bytes.size());
                return true;
            PIER_API_GUARD_END
        }

        bool api_nbt_binary_to_snbt(
            uint8_t const* data, size_t len, int32_t fmt, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                if (!sink || !data) return false;
                std::string_view view{reinterpret_cast<char const*>(data), len};
                if (fmt == 1)
                {
                    auto tag = CompoundTag::fromNetworkNbt(std::string{view});
                    if (!tag) return false;
                    sink(ctx, ps(tag->toSnbt(SnbtFormat::Minimize)));
                    return true;
                }
                auto tag = CompoundTag::fromBinaryNbt(view, /*isLittleEndian*/ true);
                if (!tag) return false;
                sink(ctx, ps(tag->toSnbt(SnbtFormat::Minimize)));
                return true;
            PIER_API_GUARD_END
        }

        void fill(PierApi& api)
        {
            api.nbt_snbt_to_binary = &api_nbt_snbt_to_binary;
            api.nbt_binary_to_snbt = &api_nbt_binary_to_snbt;
        }

        spi::SlotPackReg reg{{"nbt", &fill}};
    } // namespace
} // namespace pier::api_impl
