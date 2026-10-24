# Three save profiles

Pinned reference: v0.111.0 / 41cef1ea. Source implementation is pending its
checkpoint-6 build, native persistence scenarios, actual private artwork and
browser/device acceptance. This document is not a release receipt.

The main-menu profile button opens three profiles. Selecting the current one
closes this screen; selecting another persists the current observed metadata,
changes the host's selected slot, reloads that slot's metadata and returns to
the main menu. Continue separately validates and loads that slot's run. A
pending host operation owns input. Failure leaves the selected slot unchanged.
Deleting a slot requires the original generic-popup confirmation. Deleting the
active slot clears its live references and refreshes the profile screen.

Independent contracts use NOpenProfileScreenButton, NProfileScreen,
NProfileButton, NProfileIcon, NDeleteProfileButton and their five scenes. The
1680x1260 reference viewport is projected at 4/21. The profile HBox's child
minimum is 1368px (three 440px panels plus two 24px gaps), even though its
authored offset rectangle is 1328px. Its centered child positions match the
separately anchored delete buttons. Source focus scale/brightness and delete
label entry/fade timings are preserved with bounded UI state.

The v26 extension adds eight private derived images and twelve original
localization keys at 8400..8411. Its catalogs are 1889 auxiliary entries, 1753
clips and 8704 strings. Old images, clips, gameplay models and save bytes do not
move in their index spaces. Runtime full-pack and split readers accept the same
versions; they do not add full-file integrity scans. The original PCK and all
derived art/fonts remain private.

Slot 1 retains historical run/history paths or storage keys. Slots 2 and 3
receive distinct suffixes. The selector is separate from every run and from
the calculator's existing profiler marker. Optional NSH2 metadata provides a
bounded summary decoder; opening the profile screen does not allocate another
UiProgress/State or decode all historical runs. Legacy nonempty save presence
can mark a slot nonempty without inventing missing playtime or timestamps. It
does not bypass Continue's actual decoder.

The meaningful native scenario exercises two independent runs and metadata,
deferred host acknowledgements, unchanged historical save bytes, cancellation,
failed switching, inactive deletion and active deletion. Browser storage
transactions and calculator rename/backup behavior still require their own
actual platform validation. UI settings/language currently remain global;
the original separately reloads profile-specific Prefs on a switch. Calendar
formatting also retains the port's UTC timestamp boundary. These remain named
differences rather than implicitly claimed profile equivalence.
