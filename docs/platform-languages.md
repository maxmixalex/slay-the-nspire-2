# Shared resources and platform language selection

**Historical v18/v19 platform record.** The filenames, bilingual startup
selector, full-integrity policy, timings and acceptance receipts below describe
those older implementations. They are preserved as historical evidence, not
current UI18 installation or language-selection instructions. Use the current
[README](../README.md), [UI18 delivery record](ui18-delivery-0.17.md) and
[WORK_STATE.md](WORK_STATE.md) for the small index plus eight required parts,
optional language files, system-default selection and English technical-error
behavior. No historical runtime, resource size or device result below is
relabeled as UI18 evidence.

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

An update can require more temporary storage than the final installation:
TI OS may keep the old large common file while receiving its replacement.
This was observed during the v19 emulator update before any new common bytes
were transferred. Exit normally and preserve the save and language preference.
If the OS rejects an overwrite for insufficient space, remove only the old
common resource, then transfer its replacement and finish the launcher/locale
update before starting the game. The final v19 files are verified independently
after transfer; a successful USB operation alone is not a content check.
When migrating from the old complete pack with too little free space, back up
`nSpire2.nsp.tns` to a computer and move it off the calculator before sending
the split files. Preserve the separate save and preference files, and finish
the new launcher/common/locale installation before launch.

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

## Current v19 platform acceptance

The final ARM, WASM and tested offline HTML use runtime
`4f50193b212bb5d27e4a7e82deaa7622a5d4757d`. The append-only resource conversion
retains its actual `06c7fd16e6d4d47dc25b628913ecdeef3a4fe76d` identity. Later
test/documentation and attribution-only packaging checkpoints do not relabel
these executions. The final packaging gate compares all conservative runtime
and packaging inputs and preserves every HTML byte outside its EOF license
comment, including every inline script and embedded resource.

| Executed gate | Result |
| --- | --- |
| WASM split versus full v19 | 10 exact framebuffer pairs across both languages |
| WASM legacy compatibility | Both v17 and v18 complete language packs opened |
| WASM invalid inputs | Wrong language and actual common/locale payload corruption rejected |
| Offline Chromium | English launch, Chinese switch, remembered preference after reload, compendium input and cancelled restart; zero page errors |
| Firebird CX II CAS / OS 6.4.0.74 / Ndless r2022 | Both languages continued the original Neow save, saved normally, returned to title, then exited with Enter held exactly 80 ms; stable OS screen after seven simulated seconds |
| USB installation identity | Launcher, common, English and Chinese files fully read back and byte-identical to source |
| Save continuity | Original and final Chinese/English v15 saves passed exact-runtime load, validation and byte-identical re-encoding |

The v19 shared file is **87,405,187 bytes**, English is **1,801,249 bytes**,
and Chinese is **4,576,927 bytes**. The **93,783,363-byte** total saves
**48.239815%** against two complete packs. The ARM launcher is **978,599 bytes**,
with **16,513,432 bytes** allocated and **263,784 bytes** below 16 MiB. WASM is
**5,388,416 bytes** and retains fixed 16 MiB linear memory. The full bilingual
calculator installation occupies **94,761,962 bytes** before saves.

All three saves are 230,088 bytes, SHA-256
`9965d829dd1231146cdc7ee75c1d0e6a0ce07556dc1819b4f9d3ed0c46911d4c`.
Final exported language preferences are exactly `NSL1zhs\n` and `NSL1eng\n`.
The direct flows did not edit saves externally, substitute a checkpoint reload
or use another key to bypass the 80 ms Enter exit case. Initial update failure
for OS temporary space and a missing unversioned compiler are retained as
separate failures; their later retries are independently closed passed.

Final selected-resource validation and App initialization observed **49.774567
simulated seconds** in Chinese and **48.061584 seconds** in English. USB
readback had warmed host caches, English followed Chinese, and no cache flush
was performed. This timer excludes OS boot, Ndless installation, initial file
opening, language waiting and first-frame drawing. It is neither a physical
calculator cold-start measurement nor a strict cold A/B. Full integrity and
structural checks remain enabled. The separately controlled warm-host metadata
reuse experiment is in [the performance report](rendering-performance-0.17.md).
See [the final platform receipt](validation-platform-ui2-0.17.json) for exact
artifact identities, complete device metrics, closed journals and failures.

## Earlier v18 delivery acceptance and artifact identities

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
