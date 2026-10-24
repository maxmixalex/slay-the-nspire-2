# Run timer and General settings

Reference: the owner's pinned v0.111.0 / 41cef1ea. Relevant original contracts
are `NRunTimer`, `NRunTimerTickbox`, `NRunTimerHoverTip`, `PrefsSave`,
`NResetGameplayButton`, the top-bar scene and the General settings scene.

## Implemented behavior

The real Show Run Timer checkbox follows Phobia Mode and precedes Show Hand
Card Count. It defaults off and the actual Reset action restores that default.
When enabled, the top bar displays the observed run duration. With the setting
off, map and capstone-style panels still expose the timer, following the
original visibility predicate. The timer stops with the existing run-time
observer during pause/settings and after the run ends. It uses `MM:SS` or
`H:MM:SS`; old or partial records without a trustworthy elapsed prefix display
an em dash. Rendering never changes the canonical game save or simulates time.

The original timer icon is appended in v29. The time uses the actual original
bold font face at a six-pixel target. To accommodate the fixed 320×240 display,
the three existing right-hand map/deck/settings images retain their 18-pixel
size while their hit areas become 20/20/23 pixels when the timer is visible.
Their order and interactions remain the same. There is no startup preference
screen and language remains in the original title Settings flow.

The preference record is now NSPF4, still exactly 108 bytes including both key
binding banks. Bit 6 is assigned to the timer only in v4. Readers retain v1–v3
support and preserve their old flag masks/defaults. Web writes v4 and compatible
older records atomically; old records mask out the new bit. Profiles isolate
this preference and deletion clears every version. Language remains global.

The pinned scene hides CommonTooltips and its source untick handler is a stub.
It is intentionally not exposed as an invented working toggle. Audio and
desktop-only settings remain outside the calculator host's supported controls.

## Verification scope

`ui_preferences_regression` exercises actual observed time, pause/map/panel
visibility, persistence/reset defaults, old-record migration, unknown-prefix
display and byte-identical canonical saves. The Node profile suite exercises
v4 rollback, deletion, migration and isolation. The ordinary browser acceptance
driver clicks the real checkbox and checks persistence after reload. Native,
actual-resource and browser outcomes are recorded at the frozen runtime stage;
the existence of these checks alone is not an executed pass.
