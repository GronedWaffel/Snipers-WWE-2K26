# WWE menu research history

Target: WWE 2K26, PPSA34299, content version 01.000.000, PS5 firmware 13.60.

These chronological notes retain intermediate experiments and their validation
status at the time. Later entries supersede earlier pending results. The finished
standalone release and current validation are documented in README.md and
VALIDATION.md.

## Finisher trace

The executable registers `FinisherStock` with callback 0x2dfb6c0. It calls
0x2d10970, resolving the character and invoking accessor 0x107bdb0 followed by
0x107bfa0. The latter reads a float at component + 8. Whole units are stocks;
the fractional part is meter progress.

Character table: 0x5352548, 255 pointers. Character + 0xa0 is its slot ID.
Character + 0x25c8 -> components; components + 0x2f0 -> finisher component.
The component's first pointer references its owning character.

Character + 0x25f8 -> ComInterface. The accessor 0x153c4b0 uses 0xd1d3d0,
reading the byte at ComInterface + 0xca. In the observed match, the human was 0
and AI was 1. Active match slots 0/1 and additional character copies at 58/59
exist; do not assume a scan match is necessarily the active gameplay actor.

The setter at 0x107b3f0 clamps the requested float and dispatches a change event
when it changes. The raw-memory refill test changed memory to 3 but the user
reported no visible refill; it is not a validated cheat implementation.

## Installed test

`src/finisher-hook.mjs` builds a small resident hook for that setter. It changes
the requested stock to 3 for human-controlled characters while enabled, then
executes the original game setter. AI follows the original behavior. It preserves
the displaced instructions, scratch RAX, and flags. The game clamps and sends its
normal meter-change event. Turning the enable byte off restores original setter
behavior; closing the game removes the entire in-memory patch.

The installer verifies the exact executable bytes and allocated code, pauses
the game with PS5Debug's process-stop command during the entry patch, then resumes
in a finally block. The local ignored recovery JSON records the original bytes
and installed location. No game files or saves are modified by the installer.

Observed: after the user started a new match as Roman Reigns, the new active
human character read 3, the opponent read 0, and the hook's human execution
counter advanced. The user confirmed both starting with 3 and repeated successful
finisher use without losing availability. They also confirmed the native pause
row's X button switches ON/OFF without leaving the pause menu.

## Native menu and additional features

PauseMenuTopScreen vtable is 0x4e060f8. Its input/update callback at +0xb8
is 0x1cefa30, and selection callback at +0x268 is 0x1d88280. The initial
single-row prototype was hardware verified; the expanded Snipers Mods submenu
uses one spare row in the game's existing capacity and restores original row
labels/actions when returning. It calls native list refresh 0x1ce1d00 on the
UI thread. Never increase the row count beyond either data or widget capacity.

Signature: character components +0x2e8, owner at component +0x28, meter +0x30.
The hook at 0x1082115 replaces the clamped result with the game's configured
maximum only for human-controlled characters. The original comparison, write,
and HUD event remain intact; the requested delta and reward flags are unchanged.

Super Speed: wraps ComputeDesiredSpeed at 0x10bdde0 and multiplies its result
by 1.8 for a human owner at locomotion +0x88. It preserves zero movement and
normal behavior when disabled. Both signature and speed are installed in the
live test but await gameplay confirmation.

Ref Ignore (installed for testing; gameplay not yet confirmed): wraps the specific virtual
eligibility callbacks StateTransitionRefereeSignalDQ (0x13c6ef0, vtable entry
0x4ce47e0) and StateTransitionRefereeRingOutCount (0x1154dd0, entry 0x4b78598).
When enabled they return false. Other referee transitions are untouched. This
still requires testing weapons, ringside counts, pins, and submissions. Do not
claim that warnings or every special-match rule are disabled without testing.

The user confirmed the expanded menu was visible, but its first version could
resume the match when opened and disappear after leaving pause. Follow-up code
returns the game's consumed-action sentinel 0xffffffff (instead of the default
action hash 0xa1e41043) for custom selections, and checks rebuilt native rows on
each UI callback. This correction has been installed, with callback activity
verified; reopening and gameplay confirmation remain user tests.

The user subsequently confirmed entering the corrected submenu works, but Circle
resumed the match. The installed follow-up hooks pause vtable +0x4d0 (original
0x1cf0360): on the custom page it restores the original rows and returns true
to consume Cancel; on the ordinary page it delegates unchanged. That Circle
behavior still awaits hardware confirmation. Controller input-table inspection
at 0x48c0590 established this callback's consumed-input convention.

Speed's installed branch targets a C export whose address changes on rebuild.
Before replacing the menu image, detach speed and referee callbacks, allow
in-flight calls to finish, then reinstall their new export addresses. Never
overwrite a live menu image while those entries still point into the old image.
All new feature toggles default OFF; preserve them on a same-session update.

The debugger/FTP temporarily became unreachable during this work, but the next
read found the same process, finisher patch, allocation and menu callback still
intact. The attempted signature/speed install during that outage made no writes;
installation succeeded after the connection returned. No reboot was inferred.

## Rendering research

The executable's `UI_ShowDebugString` scripting callback resolves to the empty
function at 0x400460 in this retail build. It cannot draw the menu. GTA native
drawing functions are not applicable to WWE. A real WWE rendering/input path
must be established before installing a visible menu.

## Checks

Run `node --test tests/trainer.test.mjs` from this folder. Four checks cover
human-only writes, corrupt/foreign objects, match changes, and jump relocation.
Hook disassembly was inspected separately. This does not replace console testing.

## References and credits

PS5Debug wire client reused from PS Neighborhood in the adjacent workspace.
Debugger protocol: https://github.com/Pharaoh2k/ps5debug-NG/blob/master/PROTOCOL.md
PS5Debug-NG credits belong to its upstream authors (OSR, Ctn and contributors).
No public WWE cheat offsets were found in the checked etaHEN/TeeKay cheat trees;
the addresses above were traced from the user's installed executable.

Keep RAM captures, controller data, and local recovery records under ignored
`artifacts/`. Do not publish the executable or memory dumps.

## Oct 6 additions

The tester confirmed Infinite Signature, then reported all current toggles
working after the 3x speed, stamina and health updates. This confirmation
preceded Perfect Reversal testing and does not validate that feature.

Stamina component: character components +0x308, owner +8, value +0x10.
Native setter 0x10941f0 and current maximum 0x1094390. The hook replaces the
requested value only for a human owner whose component pointer matches, then
calls the original setter so HUD events and normal bounds remain intact.

Health component: components +0x2e0, owner +0. Native setters 0x10885c0
(maximum), 0x10887c0 (current), 0x10888f0 (recoverable). Human-only values
use 100.0; native bounds and HUD events remain active.

Reversal research: the first 0xc754c0 timing experiment received thousands of
calls but no eligible results and failed the user's test. It was removed and
its original entry restored. Do not reinstall or advertise that experiment.

The active reversal state machine updates at 0x117d970. It selects normal,
major, or breaker paths after checking attacker, animation, distance and window.
The completion routines 0x117cea0 / 0x117d020 use native ability 6 for automatic
reversals. Breaker completion 0x117d150 uses ability 12, which explicitly selects
the attack's expected button and accepts its comparison. Only four calls to
0x10704b0 inside these completion routines are redirected (0x117ced1,
0x117d049, 0x117d19e, 0x117d1da). The wrapper enables those abilities only for
the verified human character with Perfect Reversal ON. Other ability queries
are untouched. This replacement awaits user verification.

All C hook exports can move on rebuild. Detach every C entry/call-site hook
before replacing menu.bin, including stamina, health and reversal, in addition
to UI/referee callbacks; preserve the separate static signature/finisher stubs.

The tester confirmed the corrected automatic reversal implementation works.
Hidden breakers use the traced ability-12 path but have not been separately
confirmed by the tester. New counters are at scratch+0x1948 (ability 6) and
+0x1950 (ability 12); old 0x1930/38/40 counters belong to the failed experiment.

Can't Be Reversed is now a separate toggle, default OFF at scratch+0x1906.
Three calls from the existing eligible reversal state machine (0x117db56,
0x117dc50, 0x117dc96) route to wrappers of normal/major/breaker completion.
They return false before mutation when the defender is not human and the current
attacker at defender+0xf70 is the verified human character. Other matchups and
OFF delegate to the original functions. This feature awaits gameplay confirmation.
Scratch+0x1958 counts suppressed completion attempts.

Current export header appends automatic reversal at offset 96, then three
counter-completion wrappers at offsets 104,112,120. The submenu has 9 rows,
within the native tested capacity of 10. Source compiled successfully; live code
and all changed branch/call/vtable targets were read back and verified on PID115.
The earlier four trainer/finisher tests passed; they do not prove these gameplay
semantics. Do not label the new blocking feature hardware-confirmed until tested.

## Oct 6: held grapples, kick-outs and roster availability

The user confirmed Can't Be Reversed works. Perfect Reversal initially worked
mostly on strikes, with held grapples and Irish whips still missed. The first
follow-up added the eligibility input-buffer wrapper at call 0x117d0ad and a
read-only gate observation at 0x117d101; the user reported wider coverage but
still missed Irish whips. No full-coverage claim is warranted.

A separate held-grapple handler 0x1055dd0 checks defender ability 12 at
0x1055dff. Native success selects the expected input and bypasses its timing
check while retaining attacker/defender and animation constraints. That call
now uses the same human-only Perfect Reversal wrapper. Original call bytes:
e8aca60100. This latest held-grapple change awaits user verification.

Pin minigame action 0x134a590 is reached by call sites 0x1346d67, 0x1347ba0
and tail jump 0x1346db5. The new wrapper retains each original transfer kind.
Controller+0x18 is the defender, +8 is state. Only known derived vtables
4cd0be8/4cd0cd0/4cd0e00/4cd0f30 and active state 3 trigger the native +0xa0
success transition, then the original action. The transition sets state 6 and
publishes the derived HUD event. It is human-only and separate from reversals.
Awaiting actual automatic pin escape confirmation.

Roster availability 0x80a1c0 now has a 6-byte entry hook with original prologue
554889e54157 and trampoline at 0x5bc0. 0x809ea0 retrieves the catalog item for
an unlock hash. Live typed wrestler records (catalog table +0x40, key type 1)
and attire records (+0x298, key type 16) confirmed item byte +0x89 is their
category. The wrapper returns true only for categories 1/16 while enabled;
all other queries call the original trampoline. No catalog/save mutations.
Awaiting roster UI and selection verification; missing installed content is
not supplied by an availability override.

Flags 0x5907 auto kick-out and 0x5908 roster unlock. Counters 0x5960 input
buffer, 0x5968/70 breaker gate calls/accepted, 0x5978 last behavior+c4,
0x5980 pin success transitions, 0x5988 wrestler availability overrides.
Exports append at offsets 128/136/144/152. The menu now has two pages, with
10 and 3 rows respectively, and capacity guards. Circle from page 2 returns
to page 1; Circle from page 1 restores native pause rows. Reload recovery now
restores saved native rows from either custom page. Existing flag bytes remain.

The current image is 5872 bytes (under the 6144-byte installer limit), rebuilt
and attached to PID115. Live branch/vtable byte verification passed and all
four existing trainer tests passed. Those tests do not validate the three new
gameplay behaviors. Latest readback showed a responsive process with the new
hooks attached; no reboot was performed. No public release was made.

### Follow-up: grab/Irish-whip counters

Tester confirmed unlocks (including arenas) and all features except grapple
reversals/breakers. The previous ability-12 held-combo addition did not solve
ordinary held grabs. Do not describe that earlier revision as a grapple fix.

The Irish-whip transition vtable getter at 0x1131700 identifies
StateTransitionGrabStanding_IrishWhip; its vtable uses shared handler 0x1130700.
That handler finds the held opponent at attacker+0x700, applies transition
eligibility through vtable+0x128, then checks defender ability **11** at call
0x1130764 (original e847fdf3ff). Success follows the game's native counter event
and transition via vtable+0x120. The fallback compares native controller input
through +0xdf8. This is distinct from the combo behavior's ability 12.

Only that ability call now routes through the existing human-only Perfect
Reversal wrapper. Eligibility, native success handling, OFF behavior and other
features are preserved. Counter 0x5990 measures ability-11 activations. Updated
5888-byte image compiled and was loaded to PID115; original bytes and resulting
hooks were verified. The user has been asked to retry a held grab/Irish whip.
The tester subsequently confirmed it was countering everything they tested,
including the previously missed grapple/breaker cases. This is the final
implementation included in v1.0.0; earlier pending notes above are historical.

### Standalone release validation

The console-side ELF installed the same resident logic into a freshly launched
game. Installer logs confirmed allocation, pause, patch completion and resume;
independent readback verified all 27 hooks and the resident build identity.
The tester confirmed the ELF worked. A second load returned Already loaded,
with all ten enabled toggles preserved. Six automated checks passed, including
20 installer transaction scenarios. See VALIDATION.md for scope and limitations.
