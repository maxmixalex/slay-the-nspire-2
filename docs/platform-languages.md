# Shared resources and platform language selection

The runtime accepts the old `nSpire2.nsp.tns` file and the split installation:

| File | Purpose |
| --- | --- |
| `nSpire2.common.tns` | One copy of every encoded image and animation payload |
| `nSpire2.eng.tns` | English directories, glyphs and text |
| `nSpire2.zhs.tns` | Chinese directories, glyphs and text |

The split adapter reconstructs the original NSP2 stream through bounded range
reads. It does not assemble a resident pack. `Content::open` retains its full
integrity and structural validation. The 212-byte adapter check alone is not
payload validation. A corrupt or mismatched split installation is rejected;
it does not silently fall back to a different complete pack.

## Calculator

Install one `nSpire2.tns` launcher beside the common file and either or both
language files. With one language installed, it starts automatically. With
both, the first launch displays a bilingual selector: **4** selects Chinese,
**6** selects English, **Enter / 5** confirms and **Esc** returns. A successful
load records the choice in the small `nSpire2.lang.tns` preference file.

Later launches show the selected language for one second before starting.
Press **4** or **6** during that screen to change language; automatic start is
then cancelled until Enter. Startup work timing excludes the selector and its
input wait. Gameplay saves keep the same file and format in both languages.
The old full-pack filename is used when no common file is installed.

All eight startup stages, missing/mismatched-resource errors and battle-entry
preparation have independent English and Chinese messages. Before a language
is known, boot instructions and errors display both languages. No private
resource is needed to draw these messages. The 46 Chinese boot glyphs occupy
1,380 bytes of bitmap/index data and are generated from the open Noto Sans CJK
SC font. They are not derived from the game font. Source font SHA-256 is in
`include/locale_boot_glyphs.hpp`; redistribution includes
`licenses/Noto-Boot-OFL.txt`.

## Browser

Build one offline HTML using `tools/build_web.py --common ... --eng ... --zhs
... --output ...`. The common file is embedded once. Both small language files
are embedded, but only the selected one is decoded. The browser chooses a saved
preference, then its system language, then an installed language. The language
selector can restart the game in the other installed language; its confirmation
asks the player to save first. Saves use the existing localStorage key.

The host keeps physical resource buffers outside the fixed 16 MiB WASM memory.
`start_split(common_bytes, language_bytes, language)` takes language `1` for
English or `2` for Chinese. The existing `resource_read` import uses disjoint
256 MiB windows: common offsets start at `0x10000000`, locale offsets at
`0x20000000`. `start(bytes)` and offsets starting at zero retain legacy behavior.
This address convention is only the paired Web host protocol; files retain
ordinary zero-based offsets. It adds no copy of the common artwork.

The HTML shell, controls, help, status messages and errors use matching keyed
language tables. Packaging checks key coverage and validates each split pair's
full reconstructed checksum before embedding it. Startup and copy metrics do
not claim physical calculator FPS or original UI parity.

## Linux X11 and headless host

Use `--resources DIR --language eng` or `--resources DIR --language zhs` for
the normal split filenames. `--common FILE --locale FILE --language eng|zhs`
accepts explicit paths. A language omitted from the command line uses the
saved preference, the single available file, or the system language. Successful
interactive runs remember the choice; headless checks do not write preferences.
`--pack FILE` continues to open a complete old or new NSP2 pack.

The host still performs the full same `App::init`/`Content::open` path. Comparing
headless captures of a split pair and its corresponding complete NSP2 file
therefore checks actual platform loading and drawing, not only file splitting.

## Executed acceptance and artifact identities

The final ARM execution uses
`61970600a8d3f7fb2833af13f82f09c86105ebec`. The executed WASM and offline HTML
use `14a20900035b5e554e076263fc22fe7611d2e410`. A closed read-only gate compared
all **305** conservative Web compilation and packaging inputs at those two
checkpoints, including file modes; they were identical. Decoding the executed
HTML also reproduced the exact WASM, common resource and both language files.
The later attribution-only HTML repack has a separate packaging identity and
must preserve every byte outside its EOF license footer. It does not create a
new Web execution result.

| Executed gate | Result | Actual runtime |
| --- | --- | --- |
| WASM split versus full v18 | 10 exact framebuffer pairs, 5 in each language | `14a20900` |
| WASM legacy v17 | Both complete language packs opened | `14a20900` |
| WASM invalid inputs | Wrong requested language, actual common-payload corruption and actual locale-payload corruption rejected | `14a20900` |
| Offline Chromium | English launch, Chinese switch, remembered Chinese after reload, compendium input and cancelled restart; zero page errors | `14a20900` |
| X11 with real FILE callbacks | 6 exact split/full frame pairs, both legacy v17 packs, single-language auto-selection and wrong-language rejection | `573cb816`, historical |
| Firebird CX II CAS | Bilingual first selector, Chinese and English loading/title, direct Continue/save/exit and normal USB exports | `61970600` |

The shared v18 artwork/animation file is **86,870,047 bytes**, English is
**1,794,081 bytes**, and Chinese is **4,569,759 bytes**. Together they occupy
**93,233,887 bytes**, saving 86,869,727 bytes (**48.23%**) against the two complete
packs. Both reconstructed streams match their full packs byte for byte; split
conversion does not re-encode or resample their artwork. The ARM allocation is
16,507,640 bytes, leaving 269,576 bytes below 16 MiB. WASM retains its fixed
16 MiB linear memory.

The final device session used a freshly rebooted isolated OS 6.4.0.74 work
image, a visibly installed Ndless r2022 and a matching new snapshot. A Chinese
new run created a real version-15 save. The later clean Chinese and English
routes both used Continue to reach the same initial Neow event, saved through
normal controls and exited through the title's Quit confirmation with ENTER
held exactly 80 ms. After seven simulated seconds, both were still in the OS
document browser. The initial, clean Chinese and clean English USB save exports
are all 230,088 bytes with SHA-256
`9965d829dd1231146cdc7ee75c1d0e6a0ce07556dc1819b4f9d3ed0c46911d4c`.
The actual core decoded, validated and re-encoded all three identically. The
language preference exports were exactly `NSL1zhs\n` and `NSL1eng\n`. Neither
clean route edited the save externally or used the O checkpoint-reload key.

`final-firebird-save-continue-0158306e39a1.json` is closed passed with unchanged
source. The first final-runtime journal remains failed for cancelled stalled
USB requests, despite its later successful profile export. An earlier ambiguous
menu sequence and an early language-selection key that was not accepted are
retained as excluded attempts. They are not substituted for the explicitly
observed clean routes. See [the normal-exit report](ndless-exit-handoff-0.17.md)
and [the platform receipt](validation-platform-feedback-0.17.json) for exact
journal status, screenshots and readback identities.

The first final Chinese startup sample took **50.472626 simulated seconds**
inside selected-resource validation and App initialization. Full integrity was
23.055786 seconds, or 45.68%. Language-selection waiting, the Ndless loader and
the first game render are outside that interval. Files had already been
transferred over emulated USB, and host/OS caches were not flushed; this is not
physical cold-boot performance. The later English invocation is warm. All full
integrity and structural checks remain enabled; the evaluated follow-up is
sequential-read reuse, as documented in
[the performance assessment](rendering-performance-0.17.md).
