#pragma once
#include "core.hpp"
#include "inplace_reset.hpp"

namespace spire {
constexpr unsigned ResourceTransferImageBytes=320u*240u*8u+2u*240u;
constexpr unsigned ResourceTransferCapacity=ResourceTransferImageBytes+ResourceTransferImageBytes/255u+32u;
struct ResourceTransferBytes {uint8_t data[ResourceTransferCapacity];};
union ResourceTransferWorkspace {ResourceTransferBytes encoded;SaveData save;};
extern ResourceTransferWorkspace resource_transfer_workspace;
static_assert(sizeof(SaveData)<=ResourceTransferCapacity,"Synchronous saves must fit the resource transfer workspace");

// A foreground-only lease, consumed completely before returning to the game.
// Image views never point here: streamed payloads are copied/decoded into the
// normal resident image pool. Ndless file reads/writes likewise finish inside
// their host callback. Browser progress staging deliberately remains separate:
// its lifetime extends across WebAssembly calls until JavaScript acknowledges.
// Default initialization starts a trivial member lifetime without clearing a
// 600 KiB array, allocation, constructor tables, or a second large stack value.
inline uint8_t* resource_transfer_bytes()noexcept{
    return (::new(detail::InPlaceAddress{&resource_transfer_workspace.encoded}) ResourceTransferBytes)->data;
}
inline SaveData& resource_transfer_save()noexcept{
    return *::new(detail::InPlaceAddress{&resource_transfer_workspace.save}) SaveData;
}
}
