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
