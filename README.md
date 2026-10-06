# Snipers WWE 2K26 Menu

A standalone PS5 ELF installer for an in-game mod menu. Load it while WWE is
running, then open **Options → Snipers Mods** in an offline match.

## Supported setup

- **WWE 2K26, PPSA34299, version 01.000.000** (US).
- A jailbroken PS5 with **PS5Debug-NG 1.3.2 running on port 744**.
- Console testing used **PS5 firmware 13.60**. Other firmware, regions and game
  updates are not hardware-validated by this release.

The ELF connects only to the debugger on the same console (`127.0.0.1`).
It does not require a PC or PS Neighborhood after the ELF is on your console.
PS5Debug-NG is a separate prerequisite; it is not included.

## Install and use

1. Start PS5Debug-NG, then open the supported WWE build.
2. Load **Snipers-WWE-2K26.elf** through etaHEN Payload Manager or your usual ELF
   loader. PS Neighborhood can also send the file from a PC.
3. Wait for the **Loaded!** notification.
4. In an offline match, press **Options → Snipers Mods**. Press **X** to toggle.

All options start **OFF**. **More Mods** opens the second page. Circle moves
back one menu level; from the normal pause menu it resumes the match.
Loading this same ELF again leaves the installed menu and toggle states alone.
Closing WWE removes the mod; load the ELF again after the next game launch.
If you were using a development menu, close WWE completely before using this ELF.

## Features

- Infinite Finishers
- Infinite Signature
- Super Speed (3×)
- Ref Ignore (DQ/count-out)
- Infinite Stamina
- Infinite Health
- Perfect Reversal (strikes, grapples and breakers)
- Can't Be Reversed
- Auto Kick-Out — under More Mods
- Unlock All Wrestlers — under More Mods; the tester also confirmed arenas unlock

Enable unlocks before returning to wrestler/arena selection. Unlock availability
is changed in memory for the current session. The loader does not write unlock
progress to the save or download missing content. Regular game autosaves still
operate normally.

All listed gameplay features were confirmed by the tester in the live development
menu, including the final held-grapple fix. The tester also confirmed the standalone
ELF worked after restarting the game. Clean installation and repeat loading were
verified independently; see [VALIDATION.md](VALIDATION.md).

## Troubleshooting

- **Start PS5Debug-NG:** enable version 1.3.2 on port 744 before loading the ELF.
- **Open WWE:** check the exact title ID and game version above.
- **Unsupported executable:** the fingerprint does not match; no patches are applied.
- **Another menu/incompatible layout:** close WWE completely, reopen it and retry.
- **No menu yet:** enter a match; the menu is integrated into the match pause screen.

The loader writes `/data/snipers-wwe-menu.log` on the console when writable.
It checks executable fingerprints and original patch bytes, verifies copied code,
pauses the game while publishing patches, attempts rollback on partial failure,
and attempts to resume the game even when a debugger acknowledgement is lost.

## Source and credits

Original mod and installer by **GronedWaffel / Snipers**. See [CREDITS.md](CREDITS.md),
[LICENSE](LICENSE) and [BUILDING.md](BUILDING.md). The source contains our resident
code, patch metadata and fingerprints, not a game executable or memory dump.
