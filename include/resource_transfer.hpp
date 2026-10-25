#pragma once
#include "core.hpp"
#include "inplace_reset.hpp"
#include "reference_map.hpp"

namespace spire {
constexpr unsigned ResourceTransferImageBytes=320u*240u*8u+2u*240u;
constexpr unsigned ResourceTransferCapacity=ResourceTransferImageBytes+ResourceTransferImageBytes/255u+32u;
struct ResourceTransferBytes {uint8_t data[ResourceTransferCapacity];};
struct ResourceTransferSerial {
    SaveData save;
    alignas(State) unsigned char state[sizeof(State)];
};
union ResourceTransferWorkspace {ResourceTransferBytes encoded;ResourceTransferSerial serial;};
union ForegroundWorkspace {ResourceTransferWorkspace transfer;reference::MapWorkspace map;};
extern ForegroundWorkspace foreground_workspace;
extern uint8_t resource_transfer_uses;
static_assert(sizeof(ResourceTransferSerial)<=ResourceTransferCapacity,
              "Save bytes and codec staging must fit the existing image-transfer workspace");
static_assert(sizeof(ResourceTransferWorkspace)<=ResourceTransferCapacity+alignof(State)-1,
              "Sharing serialization may add alignment padding, never another large buffer");
static_assert(sizeof(ForegroundWorkspace)<=sizeof(reference::MapWorkspace)+alignof(State)-1,
              "The shared map workspace must replace, not duplicate, transfer storage");

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
// Map generation is also a complete foreground operation. It has no resource
// callbacks and publishes only the resulting graph, never a scratch pointer.
// Its full path/group capacities therefore share these same transient bytes.
enum class ResourceTransferUse:uint8_t {None=0,Bytes=1,Save=2,Codec=4,Map=8};
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
            if(use==ResourceTransferUse::Map)
                ::new(detail::InPlaceAddress{&foreground_workspace.map}) reference::MapWorkspace;
            else{
                auto*transfer=::new(detail::InPlaceAddress{&foreground_workspace.transfer}) ResourceTransferWorkspace;
                if(use==ResourceTransferUse::Bytes)
                    ::new(detail::InPlaceAddress{&transfer->encoded}) ResourceTransferBytes;
                else ::new(detail::InPlaceAddress{&transfer->serial}) ResourceTransferSerial;
            }
        }
        resource_transfer_uses=uint8_t(occupied|unsigned(use));use_=use;
    }
    ~ResourceTransferLease(){resource_transfer_uses&=uint8_t(~unsigned(use_));}
    explicit operator bool()const noexcept{return use_!=ResourceTransferUse::None;}
    uint8_t* bytes()const noexcept{return use_==ResourceTransferUse::Bytes?foreground_workspace.transfer.encoded.data:nullptr;}
    SaveData* save()const noexcept{return use_==ResourceTransferUse::Save?&foreground_workspace.transfer.serial.save:nullptr;}
    void* state_storage()const noexcept{return use_==ResourceTransferUse::Codec?foreground_workspace.transfer.serial.state:nullptr;}
    reference::MapWorkspace* map_workspace()const noexcept{return use_==ResourceTransferUse::Map?&foreground_workspace.map:nullptr;}
    ResourceTransferLease(const ResourceTransferLease&)=delete;
    ResourceTransferLease&operator=(const ResourceTransferLease&)=delete;
};
}
