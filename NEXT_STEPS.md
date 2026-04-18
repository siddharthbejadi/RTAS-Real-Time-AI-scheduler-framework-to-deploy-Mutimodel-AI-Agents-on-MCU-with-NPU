# Secure Access — Finish-the-project checklist

This doc is the punch-list for getting the project from "builds + detects faces"
to "detects **and** recognises, with the enrolled face persisted across reboots."

Everything below is sequenced so nothing breaks what already works.

---

## 0.  What is already done

- **Model #1 (BlazeFace)** is present in `Model/STM32N6570-DK/` (`network_face.*`,
  `stai_network_face.*`, `network_data.hex`). Detection works.
- **Display pipe** drives the LCD and the UI state machine (`app_ui.c`).
  Screens: SPLASH → AUTH → MAIN → BACKING_OFF / SETTINGS.
- **Face-recognition scaffolding** is in place:
  - `Application/Inc/face_recog.h` + `Application/Src/face_recog.c`
    (bilinear crop → MobileFaceNet → 128-d embedding)
  - `Application/Inc/face_store.h` + `Application/Src/face_store.c`
    (persistent store at NOR flash `0x71F00000`, 64 KB sector)
  - `main.c` calls `FaceStore_Init()`, `FaceRecog_Init()`, runs
    `FaceRecog_Identify()` after each postprocess, and handles
    `g_enroll_requested` from the UI.
  - `app_ui.c` displays the recognised person's name (from FaceStore) on
    the MAIN screen and shows an enrollment toast.
- **Pipeline guard:** `HAVE_RECOG_NETWORK` in `app_config.h`. While it is `0`
  the project **builds cleanly** — `FaceRecog_*` returns
  `FACE_RECOG_ERR_NOT_READY` and the UI falls back to "Detected
  (recog disabled)". Flip it to `1` after step 1 below.

---

## 1.  Generate the MobileFaceNet network_recog files

You already have a generator script for BlazeFace at
`Model/generate-n6-model_STM32N6570-DK.sh`. Mirror it for MobileFaceNet.

### 1.1 Drop your `.tflite` into `Model/`

Any standard MobileFaceNet int8 variant with **112×112 RGB input** and
**128-d float output** works. If yours has a different embedding size, edit
`FACE_STORE_EMB_DIM` in `Application/Inc/face_store.h` to match.

### 1.2 Save this as `Model/generate-recog_STM32N6570-DK.sh`

```bash
#!/bin/bash
set -eu

# ── Adjust these two lines to match your MobileFaceNet file name ──────────
MODEL=mobilefacenet_112_int8.tflite
PREFIX=network_recog

stedgeai generate \
    --model "$MODEL" \
    --target stm32n6 \
    --st-neural-art default@user_neuralart_STM32N6570-DK.json \
    --input-data-type uint8 \
    --output-data-type float32 \
    --name "$PREFIX"

cp st_ai_output/${PREFIX}.c                   STM32N6570-DK/
cp st_ai_output/${PREFIX}_ecblobs.h           STM32N6570-DK/
cp st_ai_output/stai_${PREFIX}.c              STM32N6570-DK/
cp st_ai_output/stai_${PREFIX}.h              STM32N6570-DK/
cp st_ai_output/${PREFIX}_atonbuf.xSPI2.raw   STM32N6570-DK/${PREFIX}_data.xSPI2.bin

# Flash address for the second model (first is at 0x70380000).
arm-none-eabi-objcopy -I binary \
    STM32N6570-DK/${PREFIX}_data.xSPI2.bin \
    --change-addresses 0x71000000 \
    -O ihex STM32N6570-DK/${PREFIX}_data.hex
```

Run it:

```bash
cd Model
chmod +x generate-recog_STM32N6570-DK.sh
./generate-recog_STM32N6570-DK.sh
```

After it finishes, `Model/STM32N6570-DK/` should contain:

```
network_recog.c            network_recog.h
stai_network_recog.c       stai_network_recog.h
network_recog_ecblobs.h
network_recog_data.xSPI2.bin
network_recog_data.hex
```

### 1.3 Flash the MobileFaceNet weights blob

Using CubeProgrammer, drag `network_recog_data.xSPI2.bin` (or the `.hex`) to
**address `0x71000000`** on the external XSPI2 NOR. This is the second slot
you already use (detection weights live at `0x70380000`).

> 64 KB block starting at `0x71F00000` is reserved for the enrollment
> store (`FACE_STORE_FLASH_BASE`) — don't flash anything there.

---

## 2.  Turn the second model on in firmware

In `Application/Inc/app_config.h`:

```c
#define HAVE_RECOG_NETWORK    1
```

Rebuild. As soon as this flag flips, `face_recog.c` will `#include` the
`stai_network_recog.h` you just generated and spin up a second STAI context
alongside the BlazeFace one.

If your MobileFaceNet variant outputs a size other than 128 floats, also
edit:

- `Application/Inc/face_store.h` → `FACE_STORE_EMB_DIM`
- Check `face_recog.c` line that reads `STAI_NETWORK_RECOG_OUT_1_SIZE_BYTES`
  matches `FACE_STORE_EMB_DIM * sizeof(float)`.

Tune thresholds on a handful of your own faces:

```c
// app_config.h
#define FACE_RECOG_MATCH_THRESHOLD   0.55f   // 0.50 – 0.70 is typical
#define FACE_RECOG_MIN_DET_CONF      0.70f
```

---

## 3.  Add MobileFaceNet's generated `.c` files to the IDE build

The new files `face_store.c` and `face_recog.c` are **already registered**
as linked resources in `Application/STM32CubeIDE/.project` — no action
needed for those.

But the MobileFaceNet files you just generated (`network_recog.c` and
`stai_network_recog.c`) are *not* yet linked. Add three entries to
`Application/STM32CubeIDE/.project` next to the existing
`Application/network_face.c` / `Application/stai_network_face.c`
entries:

```xml
<link>
    <name>Application/network_recog.c</name>
    <type>1</type>
    <locationURI>PARENT-2-PROJECT_LOC/Model/STM32N6570-DK/network_recog.c</locationURI>
</link>
<link>
    <name>Application/stai_network_recog.c</name>
    <type>1</type>
    <locationURI>PARENT-2-PROJECT_LOC/Model/STM32N6570-DK/stai_network_recog.c</locationURI>
</link>
```

Then, inside STM32CubeIDE, right-click the project → *Refresh (F5)* →
*Clean* → *Build*.

Clean + Build. You should see, in the UART console at boot:

```
[FaceStore] No enrollment data in flash — starting empty
[FaceRecog] MobileFaceNet ready (in=37632 bytes, 1 outputs)
```

(If `HAVE_RECOG_NETWORK` is still 0 you'll see
`HAVE_RECOG_NETWORK not set — recognition disabled` instead.)

---

## 4.  Enroll your first face

1. Power the board up. Wait for the AUTH screen, then walk into the frame.
2. Once on MAIN, tap **+ ADD** in the toolbar while your face is framed
   within the cyan guide box.
3. Toast appears: *"Enrolling… hold still"* → *"Face enrolled & saved to
   flash"*. Under the hood this calls:
   - `FaceRecog_EnrollFromFrame("Person 1", …)` — bilinear-resizes the face
     crop, runs MobileFaceNet, saves the 128-d embedding + name into
     `FaceStore`.
   - `FaceStore_Commit()` — disables XSPI memory-mapped mode, erases the
     64 KB sector at `0x71F00000`, writes the blob, re-enables
     memory-mapped mode.
4. Open **SETTINGS** (tap or press USER button). The "Authorized Persons"
   list should now show *Person 1* in green.

Reboot the board. On the next MAIN screen, as soon as a face is detected
with `det ≥ 70 %`, the overlay should read:

```
Access granted.
Hello, Person 1
det 93 %  /  match 78 %
```

That round-trip (enroll → reboot → recognise) proves the whole pipeline.

---

## 5.  House-keeping before pushing to git

A PowerShell cleanup script is included at the project root:
`cleanup_project.ps1`. Run it once from an elevated PowerShell to strip
~950 MB of duplicated STM32Cube firmware, unused sensor drivers, and
`.cproject` build artefacts.

```powershell
cd "D:\BAC\year 3\embedded computing\stm32n6570_secure_access"
powershell -ExecutionPolicy Bypass -File .\cleanup_project.ps1
```

Then verify the project still builds cleanly before committing.

---

## 6.  Known follow-ups (time-permitting)

- The enrollment currently crops from the **128×128 NN-pipe** frame, not
  the 480×480 display-pipe frame. Faces farther than ~60 cm will
  pixellate. Swap the `nn_in` pointer in `main.c` (stage 3 enrollment
  block) for the display-pipe buffer `lcd_bg_buffer` and pass the full
  480×480 dimensions for higher-quality embeddings. Everything else in
  `face_recog.c` already handles arbitrary `frame_w`/`frame_h`.
- Default enroll name is `Person N`. If you want user-entered names,
  extend SETTINGS with a small on-screen keyboard and a rename path
  through `FaceStore_Get`.
- Only one face is processed per inference (`best_idx`). If the demo
  needs multi-face recognition, loop over `pp_output.pOutBuff` in the
  Stage-2 block of `main.c`.

---

## 7.  File map (what lives where)

```
Application/
├── Inc/
│   ├── app_config.h          ← HAVE_RECOG_NETWORK, thresholds
│   ├── app_ui.h
│   ├── face_recog.h          ← MobileFaceNet pipeline API
│   └── face_store.h          ← Persistent enrollment API
├── Src/
│   ├── main.c                ← Stage 1/2/3 (detect, recognise, enroll)
│   ├── app_ui.c              ← "Hello, <name>" + SETTINGS list + toast
│   ├── face_recog.c          ← 112×112 crop + NPU inference
│   └── face_store.c          ← RAM copy + 64 KB flash commit
└── STM32CubeIDE/
    └── .cproject             ← Include paths (already fixed)

Model/
├── STM32N6570-DK/
│   ├── network_face.*        ← BlazeFace (existing)
│   ├── stai_network_face.*
│   └── network_recog.*       ← MobileFaceNet (you generate these)
├── generate-n6-model_STM32N6570-DK.sh   ← existing, for BlazeFace
└── generate-recog_STM32N6570-DK.sh      ← NEW, you write via §1.2
```

Flash map (external XSPI2 NOR, base `0x70000000`):

| Address      | Size   | Contents                               |
| ------------ | ------ | -------------------------------------- |
| `0x70380000` | —      | BlazeFace weights (`network_data.hex`) |
| `0x71000000` | —      | MobileFaceNet weights (new)            |
| `0x71200000` | —      | Reserved (user-flash slot 2)           |
| `0x71F00000` | 64 KB  | **FaceStore** — enrollment blob        |

Good luck — ping me if step 1 or 2 throws anything unexpected.
