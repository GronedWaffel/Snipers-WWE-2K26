# Build

Dependencies: Node.js 24+, Zig 0.14.1 and PS5 payload SDK v0.43 from
https://github.com/ps5-payload-dev/sdk. Dependency tools are not bundled.

Set `ZIG` to the Zig executable and `PS5_PAYLOAD_SDK` to the SDK root containing
`target/include`, `target/lib` and `ldscripts`.

```sh
node scripts/build.mjs
node --test tests/*.test.mjs
```

Output: `build/Snipers-WWE-2K26.elf`. The build also produces a resident image,
patch manifest and ELF SHA-256 manifest. `node scripts/build.mjs --menu-only`
rebuilds the resident image and installer metadata without the SDK link step.
The automated installer test compiles a host executable using Zig.

The flat resident image has a verified fixed allocation at 0x4000, with menu
code at 0x6000. Allocation must return that free address; the loader does not
replace an occupied mapping. All mutations are confined to the selected WWE
process. The resident hooks run on existing game threads.

`src/profile.json` pins one executable. Supporting another update requires new
fingerprints, hook/calling-convention verification and hardware testing. Renaming
the title ID or removing fingerprint checks does not add compatibility.

`artifacts/` contains private development captures and is excluded from release.
`build/` and `release/` are generated. Do not distribute game memory captures.
