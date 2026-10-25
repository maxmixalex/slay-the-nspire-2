# Observed run metadata — UI10

The pinned `NContinueRunInfo.ShowInfo` formats `SerializableRun.SaveTime`,
and `NRunHistory` displays the selected record's `BuildId`. A profile's most
recent history or preference write cannot substitute for a run-save time.
The previous implementation therefore correctly left both fields unknown,
but could not record them for new activity.

The optional UI-history codec now accepts NSH9. Its entire NSH8 body is
unchanged, followed by a fixed536-byte metadata tail: two32-bit words for
the current run's successful save time and canonical revision, then the
current record and all32 history slots'16-byte build identifiers. The normal
canonical simulation format remains NSV1/version17 and contains none of
this UI data. NSH1–8 still decode with zero metadata; profiles without new
metadata keep their previous minimal encoding and exact bytes.

Only a successful run-write callback updates `current_saved_at`. The
timestamp is returned to Continue only when the recorded run identity and
canonical revision match the file that was actually loaded. Saving settings
or history changes only the existing profile timestamp. A failed canonical
write changes neither timestamp. If the run write succeeds but optional
history persistence fails, the session retains the true new time and a dirty
retry; an older on-disk metadata revision does not label the newer run.
The two files are not a cross-file transaction. If identical canonical bytes
are saved again, metadata persistence fails and the host is forcibly closed,
the unchanged revision can only recover the last persisted observed time.
Ordinary Save and Quit returns failure and keeps the session open for retry.
No extra invalidation writes, integrity scan or canonical-format change is
introduced to manufacture stronger crash guarantees.

Newly begun runs record the actual port identifier `nSpire II 0.17`. A legacy
record or first observation of an old resumed run keeps an empty identifier.
The History label displays that observed value or an unknown dash; it never
substitutes the current executable or the reference game's commit for an
unknown historical build.

Continue uses the pinned locale format: the English weekday, abbreviated
month, date, year and12-hour clock; or the Chinese date, weekday and morning/
afternoon clock. The DateLabel stays top-aligned, including wrapped dates.
The calendar uses the existing timestamp interpretation. Host-local timezone
conversion, including daylight-saving differences for historical browser
timestamps, remains a separately identified limitation.

`run_metadata_regression` covers successful and failed writes, profile-only
writes, stale revision rejection, legacy unknowns, exact canonical save bytes,
NSH8-prefix preservation, malformed/truncated metadata and Gregorian
calendar boundaries. `ftue_progress_regression --export DIRECTORY` retains
the old295,349-byte maximum NSH8 fixture and additionally exports the
295,885-byte maximum NSH9 fixture. New tests and source changes require
their matching frozen runtime and ordinary-host acceptance before delivery.
