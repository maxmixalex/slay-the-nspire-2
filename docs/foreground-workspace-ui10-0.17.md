# Shared foreground map and transfer workspace

The existing 617,328-byte transfer workspace and 794,788-byte map workspace
had disjoint production lifetimes. They now occupy one 794,792-byte aligned
union. The static storage difference is **617,324 bytes**, without reducing
image pages, decoded groups, text caches, map paths, save capacity or history
capacity. The final linked ARM allocation still needs to include code,
relocations and loader overhead; this object-size difference is not that gate.

## Lifetime contract

Map construction is a synchronous foreground operation. The default
`reference::generate_act_map` overload borrows the shared workspace, computes
the entire graph and returns. It performs no resource reads, decoder calls or
host callbacks. `ReferenceMap` stores its own points, edges, grid and RNG state;
it retains no pointer into `MapWorkspace`. Callers with an independently owned
workspace keep the existing explicit-workspace overload.

Resource loads already finish their encoded-data lease before publishing a
decoded image or text view. Save/load already use a separate State suffix
while a host Save lease retains the file prefix. Neither canonical State
validation nor serialization generates a map. History/file callbacks retain
their existing complete foreground lifetime.

The same one-byte lease mask now includes Map. Map is exclusive with Bytes,
Save and Codec. The existing Save→Codec nesting remains the only permitted
overlap, because those ranges are independent. A refused lease does not begin
a different union member's lifetime or touch any existing bytes. A blocked
default map request returns an explicit capacity-error map without replacing
the active workspace. This preserves the existing serial event-loop contract;
it does not add cross-thread synchronization.

## No new large clear or cache eviction

`MapWorkspace` is now trivial scratch storage. Its full capacities remain
32,768 groups and 32,768 segments, with the existing 14-byte packed segments.
The generator resets every bucket, live counter and point-id mapping before
use, and still value-initializes each new `Segment{}` and `Group{}`. Activating
the union with placement construction therefore performs no 794 KiB clear.
Unused previous map, encoded-image or file bytes are never treated as map
entries. The transfer members are likewise activated without clearing their
entire capacity. No decoded pixel or text cache lives in the shared union.

## Focused verification

The short `foreground_workspace_regression` passes **788,050 checks**. It
compares all graph/RNG/quota fields against an independent dirty workspace
across 48 combinations of regions, standard/golden/spoils variants and map
modifiers. Each case alternates map construction, a full 512 KiB LZMA2 transfer,
canonical save/load/save and reconstruction of the same map. Both the earlier
returned graph and exact save bytes survive those overwrites.

The test also verifies unchanged bytes on every rejected nested lease,
preservation of the Save prefix during its allowed Codec operation, untouched
State/output buffers on refusal, and release on every scope exit. It proves
that activating each member leaves a prefilled entire union unchanged, so a
hidden aggregate constructor cannot reintroduce a large clear. The existing
retained-image regression now also asks for a map from the real streamed
resource callback and requires refusal without corrupting the current decode.

Full native suites, the matched ARM link/allocation, original map transcripts
and ordinary save/continue remain the next frozen-source acceptance gates.
