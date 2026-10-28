# Chaz Chrome

**Melodic trap / hip-hop lead** instrument by **Chazmaniac** — the top-line companion to [Chaz Trap](https://github.com/chazmaniac302/chaz-trap) (808/kit) and [Chaz Velvet](https://github.com/chazmaniac302/chaz-velvet) (neo-soul keys).

Pure synthesis (no samples). Whistle/air, dark flute, chrome pluck, ice bell. Portamento + mono-legato for classic trap leads. Macros: **Heat · Space · Width · Edge · Chaos**.

| | |
|---|---|
| Product | Chaz Chrome |
| Vendor | Chazmaniac (`Chzm`) |
| Plugin code | `ChCr` |
| Bundle ID | `com.chazmaniac.chazchrome` |
| Formats | VST3 + Standalone (Windows x64) |
| JUCE | `8.0.16-security` |
| License | GPL-3.0-or-later |
| Factory presets | 24 (signature **CHROME**) |

## Build (engine tests on Linux)

```bash
cmake -B build-engine -DCHAZ_BUILD_PLUGIN=OFF
cmake --build build-engine -j
ctest --test-dir build-engine --output-on-failure
```

## Build (full plugin)

```bash
cmake -B build -A x64 -DCHAZ_JUCE_TAG=8.0.16-security   # Windows MSVC
cmake --build build --config Release --target ChazChrome_VST3
```

Or push to `main` and let `.github/workflows/build-windows-vst3.yml` produce the zip + pluginval.

## Install (FL Studio)

See [INSTALL-FL-STUDIO.md](INSTALL-FL-STUDIO.md). Quick path: copy `Chaz Chrome.vst3` into
`C:\Program Files\Common Files\VST3\`, Unblock-File if needed, rescan plugins.
Load as a **generator / instrument**, not an FX.

## Sound layers

1. **Whistle** — breathy high sine + soft noise air + vibrato  
2. **Flute** — filtered pulse/sine wood lead  
3. **Pluck** — sharp chrome transient + FM body  
4. **Bell** — short ice/metallic stab  

Enable **MONO** + **PORTA** for sliding trap leads. Play a minor pentatonic on preset **CHROME**.
