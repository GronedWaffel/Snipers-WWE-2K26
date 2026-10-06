# Validation

Target: WWE 2K26 PPSA34299 01.000.000 on PS5 firmware 13.60.

## Gameplay

The tester confirmed all menu features in the live development build. The final
Perfect Reversal fix adds the native ability-11 counter used for held grabs and
Irish whips; after that change the tester reported it countered everything they
tested. This is not a claim that every match type and move has been exhaustively
covered. Unlocks were observed for wrestlers and arenas. Automatic kick-outs and
Can't Be Reversed were also confirmed.

## Standalone ELF

- Resident image and release ELF compile successfully.
- ELF header, segment bounds/alignment and executable entry point verified.
- Six automated tests pass, including an installer test with 20 scenarios:
  duplicate load, unsupported executable, existing modifications, occupied memory,
  allocation mismatch, failed uploads, partial writes, rollback, lost stop/resume
  acknowledgements and incomplete recovery reporting.
- Patch layout and jump targets are checked, including the final grapple call.
- Clean installation into a freshly launched game succeeded on October 6, 2026.
  The console log confirmed allocation, pause, resume and successful installation.
  Independent debugger readback verified all 27 hooks and the resident image's
  immutable build identity. The tester then confirmed the standalone menu worked.
- Loading the same ELF again returned "Already loaded" without pausing or
  repatching the game. All ten enabled toggle bytes were unchanged before/after.

A successful file transfer alone is not considered a successful installation.
The above console checks verified both the installer log and applied hooks.
Other firmware, game versions, regions and untested match types remain unverified.
