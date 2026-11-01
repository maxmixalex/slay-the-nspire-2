# UI23 persistent run-history pages

The calculator and native host now keep completed runs and older floor/journal
pages in a profile-owned append archive. The existing 32-run and 96-floor arrays
are working windows; they no longer define how many successful archived records
remain available. The archive uses actual state-directory storage as it grows.

Old NSH1--9 progress files retain their existing schema and recent-run records.
Only a profile with paged floor totals uses the new NSHA extension, which keeps
all older fields and appends the packed prefix totals. The old formats remain
readable, and ordinary unpaged profiles retain their original encoded bytes.
Opening History copies those existing completed records without destroying or
relabeling their original observations. Interrupted migration is idempotent.
No earlier facts missing from an old save are invented. Canonical game save
formats and gameplay random streams remain unchanged.

The archive publishes a new committed prefix only after the new page has been
written, closed and read back. Two independently validated heads preserve the
previous committed prefix on a torn write. Retrying an already completed serial
does not replace its first detailed record with a later evicted RAM journal.
These page checks are for durable write/recovery; no full archive is read or
hashed during game startup.

Before dispatching a foreground command at a window boundary, the App preserves
the old run or completed floor prefix. A failed write keeps that window intact
and reports a retryable storage error. Ordinary inputs below these boundaries
perform no archive IO and preserve warm renderer caches. Long journals spill
their observed rows separately while retaining live scalar totals and pending
transformations. The current room stays in RAM when older floors are paged out.
Escape/Quit and the existing exit confirmation remain usable after a storage
failure, so the user can cancel or leave to free space without advancing play.
These new storage diagnostics have reviewed direct bilingual locale bindings;
they do not change the stable indexes in existing original-language packs.

History opens the latest completed run. Previous/Next traverse the run's older
or newer pages before moving to another run; the existing floor hover and card
inspection use the selected page. Earlier pages keep the completed run's final
header, deck, outcome, score and badges alongside their observed older rooms.
Temporary decoding borrows the existing foreground scratch; a failed read does
not replace the visible page. A single decoded page reuses the existing
scene union. The two transient progress flags occupy prior alignment padding;
the persistent run/journal arrays and scene-union allocation are not enlarged.
Four former tail-padding bytes in each run retain archived floor weights and
MysteryMachine/Restful/Restless facts, so paging cannot reduce scores or badges.
All profile deletion paths include the new archive and preserve other profiles.

The archive is bounded by available disk space, the existing 32-bit run serial
identity and signed file offsets, not an unlimited-storage guarantee. The
portable callback-free host retains its previous bounded history behavior.

Validation for this implementation must include real App recording beyond 32
runs and 96 floors, legacy migration/reopen, journal overflow, truncated writes,
profile isolation, final ARM allocation and actual calculator-firmware History
navigation/save/relaunch. Source presence or codec tests alone do not establish
that last UI/device gate. See the current execution receipts for passed checks;
this document does not convert pending checks into completed acceptance.
