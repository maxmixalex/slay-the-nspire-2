# UI4 ordinary Firebird acceptance

The final runtime `76f0a01cb506fde7ac8b427e857462a35933ade3` completed both ordinary Chinese and English OS/Ndless routes with the existing v21 resources. The 997,748-byte launcher was transferred by USB and downloaded back exactly. Both closed journals report success, unchanged source and zero controller errors; both game reports contain zero resource-read failures. No snapshot restore, forged game state, RAM patch or long-hold input workaround was used in these accepted routes.

## Title usability correction

The earlier 9a candidate displayed the original title layers but rendered at 226.867 ms mean / 243.652 ms maximum and missed the 80 ms Continue press. It was blocked before delivery. The sparse scaling correction preserves the resources, all 280 particles, their sampling and the rendered pixel mapping while removing repeated source-pixel divisions.

The accepted Chinese title contains 350 samples at **42.236 ms mean, 38.605 ms minimum and 42.999 ms maximum**, with **zero scene resource I/O**. Both languages accepted the original 80 ms Continue input after a ten-second title dwell, and the same short holds worked through save/quit. The observed Chinese mean fell by about 81.4%; this is emulator render timing, not a physical-handheld FPS claim. The English title aggregate also includes returning from battle, where resource reload produces a 279.449 ms maximum; that aggregate must not be called an entirely warm window.

## Exercised UI and card movement

Chinese Continue returned to the saved floor 2 run, followed by the eight-card compendium, focused Block tooltip and Defend details. The four corresponding captures are byte-identical to the UI3 baseline. Normal Save and Quit and normal OS exit succeeded. The integrated collection still has a **1,273.224 ms maximum render** and an independently aggregated maximum resource interval of **1,224.335 ms**. These maxima do not decompose a single frame and must not be subtracted to estimate CPU time. Its first-entry pause remains open.

English was selected through the normal startup selector. Continue, Proceed and the reachable map Fight entered the actual floor 3 encounter. Ordinary navigation played Strike and Bash. Existing captures show the central Play card and the rotating, shrinking discard travel; the Bash card is still in flight in the capture named 730 and is absent by 1130. Those suffixes are command offsets, not exact animation ages.

After Space, the old hand leaves and the Enemy Turn banner appears with an empty hand. The player's HP changes from 61 to 58 before the next hand appears. Player Turn / Turn 2 restores energy to 3/3; successive captures show separate card entries and draw-pile changes before the final five-card hand settles. A second agent reviewed these existing frames independently. The run then used the ordinary pause menu to Save and Quit, showed Floor 3 / HP 58 / gold 118 on the Continue panel and exited to the OS. The encounter was not played to victory, and the changed combat save was not subsequently downloaded or reloaded in an additional game scenario.

## Evidence boundaries

`validation-ui4-firebird-0.17.json` preserves the two raw SP804 reports, every ordinary controller command, journal receipts, exact card-frame comparisons and private evidence paths. It references the isolated performance comparison and the deliberately retained failed 9a report. The source-only reports contain no original assets, NAND, screenshots, launchers or save payloads.

The accepted routes confirm the exercised movement and hand boundary, not exact original-PC timing for every enemy command, a complete five-character gameplay sweep, or physical-calculator speed. The remaining cold-card I/O and broader original-game animation comparisons remain separate work.
