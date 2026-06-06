# Accelerator Optimizations: Arm Helium and Display DMA2D

This note documents the first accelerator pass added to the STM32N6570 RTAS
vision pipeline.

The goal was to reduce CPU work without changing the model pipeline behavior:

- Use Arm Helium on the Cortex-M55 for face-embedding cosine matching.
- Use the available display hardware acceleration path for the depth preview UI.
- Keep scalar/software fallbacks so the firmware remains build-safe.

## Summary

| Area | File | What changed |
| --- | --- | --- |
| Face recognition | `Application/Src/mobilefacenet_face_recognition/face_store.c` | Replaced scalar cosine accumulation with a Helium-aware fused vector path. |
| Depth preview UI | `Application/Src/ui_services/app_ui.c` | Replaced thousands of tiny rectangle draws with one packed ARGB4444 DMA2D blit. |
| Local build | `tools/build_debug.ps1` | Added a wrapper that finds the STM32CubeIDE GCC toolchain and runs the generated Debug makefile. |

## Arm Helium Optimization

### Previous Behavior

Face matching used a scalar loop inside `cosine_similarity()`:

```c
dot += av * bv;
na  += av * av;
nb  += bv * bv;
```

For every stored face embedding, the CPU walked the embedding one float at a
time. This was correct but did not explicitly use the Cortex-M55 MVE-F vector
hardware.

### New Behavior

`face_store.c` now includes CMSIS DSP/Helium headers:

```c
#include "arm_math.h"

#if (FACE_STORE_USE_HELIUM != 0) && defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)
#include "arm_helium_utils.h"
#define FACE_STORE_HELIUM_F32 1
#else
#define FACE_STORE_HELIUM_F32 0
#endif
```

The cosine math is now fused into one helper:

```c
static void cosine_terms_f32(const float *a, const float *b, uint32_t n,
                             float *dot_out, float *na_out, float *nb_out)
```

When `ARM_MATH_MVEF` is available, it processes four `float32` values per vector
step:

```c
f32x4_t vec_a = vld1q(a);
f32x4_t vec_b = vld1q(b);

vec_dot = vfmaq(vec_dot, vec_a, vec_b);
vec_na  = vfmaq(vec_na,  vec_a, vec_a);
vec_nb  = vfmaq(vec_nb,  vec_b, vec_b);
```

The remaining 0 to 3 tail elements still use the scalar loop, so the function is
safe for non-multiple-of-four dimensions.

### Why This Is Better

The previous scalar path did one multiply-accumulate at a time. The new Helium
path performs four float lanes per vector instruction and calculates all three
cosine terms in one pass:

- Dot product: `a * b`
- Probe norm: `a * a`
- Stored norm: `b * b`

This reduces both instruction count and memory traffic during face matching.

### Fallback Behavior

If Helium MVE-F is not enabled, the same function falls back to scalar C. This
keeps the code portable and avoids adding a CMSIS-DSP library link dependency.

## Display Acceleration

### Important Naming Note

The STM32N6 has NeoChrom GPU2D hardware, but this project currently exposes only
low-level GPU2D HAL pieces. The high-level NeoChrom command-list middleware is
not wired into this firmware.

For this step, the implemented display acceleration uses the display path that
is already enabled and linked in the project: DMA2D through the STM32 LCD/BSP
stack.

So the practical result is:

- Not a full NeoChrom command-list renderer yet.
- Yes, a real hardware-assisted display transfer using DMA2D.

### Previous Behavior

`DrawDepthPreviewMap()` rendered the depth heat map with many tiny calls:

```c
UTIL_LCD_FillRect(x + px, y + py, 2U, 2U, DepthHeatColor(v));
```

For a full preview, this could submit thousands of small blocking rectangle
draws. Even if each rectangle uses hardware fill internally, the CPU still has
to babysit every tiny draw call.

### New Behavior

The depth preview now builds one packed ARGB4444 preview buffer in PSRAM:

```c
static uint16_t s_depth_preview_argb4444[
    DEPTH_PREVIEW_BLIT_MAX_W * DEPTH_PREVIEW_BLIT_MAX_H
];
```

Then it transfers the whole preview rectangle in one DMA2D operation:

```c
HAL_DMA2D_Start(&hlcd_dma2d,
                (uint32_t)s_depth_preview_argb4444,
                dst_addr,
                w,
                h);
HAL_DMA2D_PollForTransfer(&hlcd_dma2d, 50U);
```

The foreground LCD layer is ARGB4444, so the buffer is packed as ARGB4444 rather
than RGB565. This avoids fast-but-wrong color output.

### Fallback Behavior

If DMA2D cannot run or the preview size exceeds the fixed buffer limit, the code
falls back to the original tiny-rectangle renderer:

```c
DrawDepthPreviewFallback(x, y, w, h);
```

This keeps the UI functional even if the accelerated path is unavailable.

## Build Wrapper

The generated CubeIDE makefile expects `arm-none-eabi-gcc` to be on `PATH`.
Locally, the compiler was installed but not visible to PowerShell.

The new helper script solves that:

```powershell
powershell -ExecutionPolicy Bypass -File tools\build_debug.ps1 -Jobs 4
```

It searches common STM32CubeIDE install folders, prepends the discovered
`tools\bin` directory to `PATH` for that build process only, and then runs:

```powershell
mingw32-make -C Application\STM32CubeIDE\Debug -j4
```

## Verification Performed

The firmware was rebuilt successfully with:

```powershell
powershell -ExecutionPolicy Bypass -File tools\build_debug.ps1 -Jobs 4
```

The final linked image was generated:

```text
STM32N6570-DK_Implementing_RTAS.elf
STM32N6570-DK_Implementing_RTAS.bin
STM32N6570-DK_Implementing_RTAS.list
```

The linked firmware size after this pass was:

```text
text   data    bss      dec      hex
461384 10020   3213488  3684892  383a1c
```

The compiled `face_store.o` was also inspected with `arm-none-eabi-objdump`.
The object contains MVE-F instructions such as:

```text
vldrw.u32
vfma.f32
```

That confirms the Helium vector path is active in the generated code.

## Expected Performance Impact

### Face Matching

Expected improvement:

- Lower CPU time inside face embedding comparison.
- Better scaling as the number of enrolled faces grows.
- Less memory traffic because dot/norm terms are calculated in one pass.

This will mostly help the MobileFaceNet post-processing and identity matching
stage, not the camera frame cadence.

### Depth Preview UI

Expected improvement:

- Far fewer draw submissions per UI refresh.
- Less CPU time spent issuing small display operations.
- Smoother UI updates when the depth preview is visible.

This should help prevent the display path from stealing time from camera/NPU
orchestration.

## What This Does Not Fix Yet

The earlier 19.6 FPS ceiling is still parked. These accelerator changes do not
directly change camera sensor cadence, VSYNC pacing, DCMIPP frame rate, or the
main frame-wait loop.

The next camera-side investigation should still focus on:

- Sensor FPS configuration.
- DCMIPP frame-end interrupt cadence.
- Any forced 50 ms wait in the scheduler loop.
- Whether the sensor is actually producing 30 FPS on the selected mode.

## Next Steps

1. Flash the new firmware and capture telemetry.
2. Compare MobileFaceNet matching/post-processing time before and after Helium.
3. Compare UI update time with the depth preview visible.
4. If display is still expensive, add a dedicated bounding-box overlay batching
   path.
5. Later, consider wiring true NeoChrom GPU2D command-list middleware if the ST
   stack for this board is added to the project.
