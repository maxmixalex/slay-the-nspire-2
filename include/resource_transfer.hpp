#pragma once
#include "core.hpp"
#include "inplace_reset.hpp"

namespace spire {
constexpr unsigned ResourceTransferImageBytes=320u*240u*8u+2u*240u;
constexpr unsigned ResourceTransferCapacity=ResourceTransferImageBytes+ResourceTransferImageBytes/255u+32u;
struct ResourceTransferBytes {uint8_t data[ResourceTransferCapacity];};
struct ResourceTransferSerial {
    SaveData save;
    alignas(State) unsigned char state[sizeof(State)];
};
union ResourceTransferWorkspace {ResourceTransferBytes encoded;ResourceTransferSerial serial;};
extern ResourceTransferWorkspace resource_transfer_workspace;
extern uint8_t resource_transfer_uses;
static_assert(sizeof(ResourceTransferSerial)<=ResourceTransferCapacity,
              "Save bytes and codec staging must fit the existing image-transfer workspace");
static_assert(sizeof(ResourceTransferWorkspace)<=ResourceTransferCapacity+alignof(State)-1,
              "Sharing serialization may add alignment padding, never another large buffer");

// Foreground-only leases, consumed completely before returning to the game.
// Image views never point here: streamed payloads are copied/decoded into the
// normal resident image pool. Ndless file reads/writes likewise finish inside
// their host callback. Browser progress staging deliberately remains separate:
// its lifetime extends across WebAssembly calls until JavaScript acknowledges.
//
// Image/history transfers are exclusive. A host Save lease reserves the save
// prefix throughout file I/O, and may contain one codec operation in the
// separate State suffix. A reentrant reader/observer cannot overwrite either
// active range. This follows the existing single-thread event-loop contract;
// it is deliberately not a cross-thread lock.
enum class ResourceTransferUse:uint8_t {None=0,Bytes=1,Save=2,Codec=4};
class ResourceTransferLease {
    ResourceTransferUse use_=ResourceTransferUse::None;
public:
    explicit ResourceTransferLease(ResourceTransferUse use) noexcept {
        if(use==ResourceTransferUse::None)return;
        const unsigned occupied=resource_transfer_uses;
        if(use==ResourceTransferUse::Codec?bool(occupied&~unsigned(ResourceTransferUse::Save)):bool(occupied))return;
        // Default initialization begins a trivial member lifetime without
        // clearing 600 KiB, running constructor tables or making a stack copy.
        // An enclosing Save lease already owns serial.save: leave it intact.
        if(!occupied){
            if(use==ResourceTransferUse::Bytes)
                ::new(detail::InPlaceAddress{&resource_transfer_workspace.encoded}) ResourceTransferBytes;
            else ::new(detail::InPlaceAddress{&resource_transfer_workspace.serial}) ResourceTransferSerial;
        }
        resource_transfer_uses=uint8_t(occupied|unsigned(use));use_=use;
    }
    ~ResourceTransferLease(){resource_transfer_uses&=uint8_t(~unsigned(use_));}
    explicit operator bool()const noexcept{return use_!=ResourceTransferUse::None;}
    uint8_t* bytes()const noexcept{return use_==ResourceTransferUse::Bytes?resource_transfer_workspace.encoded.data:nullptr;}
    SaveData* save()const noexcept{return use_==ResourceTransferUse::Save?&resource_transfer_workspace.serial.save:nullptr;}
    void* state_storage()const noexcept{return use_==ResourceTransferUse::Codec?resource_transfer_workspace.serial.state:nullptr;}
    ResourceTransferLease(const ResourceTransferLease&)=delete;
    ResourceTransferLease&operator=(const ResourceTransferLease&)=delete;
};
}
