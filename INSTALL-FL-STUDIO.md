# Install Chaz Chrome in FL Studio (Windows x64)

## 1. Get the plugin

Download **`ChazChrome-VST3-Windows-x64.zip`** from GitHub Actions artifacts / Release on
[chazmaniac302/chaz-chrome](https://github.com/chazmaniac302/chaz-chrome), or from your OneDrive
**Chaz Chrome** folder.

If Windows marks the zip as downloaded from the internet: right-click → **Properties** → tick **Unblock** → OK.
Or run `Install-ChazChrome.ps1` as Administrator (preferred).

## 2. Install the VST3

1. Unzip. You should see a folder named **`Chaz Chrome.vst3`** (bundle / folder, not a single file).
2. Copy the entire **`Chaz Chrome.vst3`** folder into:

   ```
   C:\Program Files\Common Files\VST3\
   ```

   Confirm `Contents\x86_64-win\Chaz Chrome.vst3` (binary) exists inside the bundle.
3. In FL Studio: **Options → Manage plugins**. Confirm that `C:\Program Files\Common Files\VST3\` is in the search paths.
4. Click **Find installed plugins** / **Start scan**.
5. Look for **Chaz Chrome** by **Chazmaniac** under **Generators / Instruments** (not Effects).

## 3. Load it

Channel Rack → **(+)** → generators → **Chaz Chrome**. It is an **instrument** — MIDI notes play leads. Do **not** put it on a Mixer FX slot as an effect.

## 4. First session

1. Open preset **CHROME**.
2. Draw a minor pentatonic top line (e.g. A–C–D–E–G), mid-high velocity.
3. Enable **MONO** and ride **PORTA** for trap slides.
4. Macros: **Heat** (drive), **Space** (short plate), **Width**, **Edge** (brightness), **Chaos** (vibe instability).
5. Toggle layers: WHISTLE / FLUTE / PLUCK / BELL.

## Vendor

Chazmaniac · GPL-3.0-or-later · Plugin code `ChCr`
