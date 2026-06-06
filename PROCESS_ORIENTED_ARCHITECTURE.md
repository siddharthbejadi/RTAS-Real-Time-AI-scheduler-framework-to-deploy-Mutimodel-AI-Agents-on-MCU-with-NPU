# implementing RTAS-MCU Process-Oriented Architecture

Project root after cloning:

```text
<repo-root>
```

## Important Meaning

In normal desktop/server C, "process-oriented" often means separate OS processes using
`fork()`, `exec()`, pipes, shared memory, sockets, and semaphores.

This STM32N6 project is different. It is an embedded bare-metal/STM32Cube project, not a
Linux application. There is no normal operating system process table, no `fork()`, no
separate address spaces, and no desktop IPC.

So in this project, process-oriented means:

```text
one firmware image
one main loop
four cooperative application processes
clear responsibility boundaries
shared state passed through C structures and module APIs
```

This keeps the firmware realistic for STM32 while still giving the design a clean
process-oriented structure.

## Main Loop

The main loop is in:

```text
Application/Src/main.c
```

Search for:

```text
Cooperative process-oriented loop
```

The loop is intentionally simple:

```c
while (1)
{
  CameraFrame_Process();
  ModelScheduler_Process();
  AuthDecision_Process();
  UiSystem_Process();
}
```

This is the main conversion. Instead of one long sequential body doing everything, the
firmware is now organized as four cooperative processes.

The process bodies are not in `main.c`; they live in:

```text
Application/Src/process_orchestration/app_processes.c
Application/Inc/app_processes.h
```

`main.c` keeps hardware/display startup and calls the four process APIs. The process
module owns the AI runtime context, NN input/output pointers, model scheduler state, and
the per-frame process context.

The `Application/Src` root is intentionally kept small:

```text
Application/Src/main.c
Application/Src/blazeface_face_detection/
Application/Src/fastdepth_liveness/
Application/Src/mobilefacenet_face_recognition/
Application/Src/platform_system/
Application/Src/process_orchestration/
Application/Src/tim_assistant/
Application/Src/ui_services/
```

Generated STEdgeAI model files stay in `Model/STM32N6570-DK` because the ST-generated
runtime and include paths already expect that model artifact location.

## The Four Processes

### Process 1: Camera Frame Process

Location:

```text
Application/Src/process_orchestration/app_processes.c
CameraFrame_Process()
```

Purpose:

```text
camera snapshot -> cache invalidate -> crop/pack if needed -> normalize into NN input
```

Main modules used:

```text
Application/Src/blazeface_face_detection/app_camerapipeline.c
Application/Src/blazeface_face_detection/crop_img.c
Model/STM32N6570-DK/stai_network.h
```

Why it is a process:

It owns the frame acquisition stage. Later processes do not need to know how the camera
snapshot was captured, how the row pitch works, or whether the input needed cropping.

### Process 2: Model Scheduler and Inference Process

Location:

```text
Application/Src/process_orchestration/app_processes.c
ModelScheduler_Process()
ModelScheduler_SelectRecognition()
ModelScheduler_ShouldRunDepth()
```

Purpose:

```text
run BlazeFace -> choose best face -> decide which extra models should run
```

Main modules used:

```text
Middlewares/ai-postprocessing-wrapper
Application/Src/mobilefacenet_face_recognition/face_recog.c
Application/Src/fastdepth_liveness/app_depth.c
Application/Src/mobilefacenet_face_recognition/face_store.c
Model/STM32N6570-DK/network_face.c
Model/STM32N6570-DK/network_embed.c
Model/STM32N6570-DK/depth.c
```

Scheduler rules:

```text
BlazeFace detection:
  runs every frame

Face recognition:
  runs only when there is one stable face
  runs during auth at a controlled recheck interval
  runs during enrollment when needed
  runs in main mode only as a periodic recheck

FastDepth:
  runs only when the face is stable
  runs during enrollment or after identity is known
  runs at a slower interval than detection
```

Why it is important:

Running every model on every frame wastes NPU time and increases latency. The scheduler
is the key optimization trick: it decides what should run now, what can wait, and what
does not need to run for the current frame.

### Process 3: Authentication Decision Process

Location:

```text
Application/Src/process_orchestration/app_processes.c
AuthDecision_Process()
```

Purpose:

```text
model outputs -> enrollment/authentication state changes -> timing metrics
```

Main modules used:

```text
Application/Src/mobilefacenet_face_recognition/face_recog.c
Application/Src/mobilefacenet_face_recognition/face_store.c
Application/Src/ui_services/app_ui.c
```

Why it is a process:

The model process should only produce evidence. This process decides what that evidence
means for the application: enroll success, enroll failure, duplicate face, access match,
and CPU/NPU timing.

### Process 4: UI and System Services Process

Location:

```text
Application/Src/process_orchestration/app_processes.c
UiSystem_Process()
```

Purpose:

```text
poll TIM/text intent system -> render UI -> update buzzer -> clean output cache
```

Main modules used:

```text
Application/Src/ui_services/app_ui.c
Application/Src/ui_services/app_touch.c
Application/Src/ui_services/app_buzzer.c
Application/Src/tim_assistant/tim_app.c
Application/Src/tim_assistant/tim_inference.c
Application/Src/tim_assistant/tim_tokenizer.c
```

Why it is a process:

UI, touch, buzzer, and text-intent polling are system services. They should not be mixed
inside camera capture or model scheduling logic.

## Data Flow

```text
Camera frame
    |
    v
Process 1: CameraFrame_Process
    prepares nn_in and ctx->nn_src_u8
    |
    v
Process 2: ModelScheduler_Process
    detection -> recognition decision -> depth decision
    |
    v
Process 3: AuthDecision_Process
    enrollment/authentication state changes
    |
    v
Process 4: UiSystem_Process
    render UI and poll services
```

## Shared Runtime State

The process loop uses:

```text
static AppProcessContext_t s_proc
```

This context carries per-frame/runtime information such as:

```text
pitch_nn
nn_in_len
nn_out
nn_out_len
nn_info
nn_src_u8
best_idx
frame_cpu_start
```

The context is private to `Application/Src/process_orchestration/app_processes.c`, so the public surface is only:

```text
AppProcesses_Init()
AppProcesses_GetNnPitch()
CameraFrame_Process()
ModelScheduler_Process()
AuthDecision_Process()
UiSystem_Process()
```

Global state still exists because this is embedded firmware and several modules already
share UI/authentication variables. The process-oriented change is not about removing all
globals immediately; it is about making the execution flow controlled and understandable.

## File Grouping

```text
Process 1: Camera
  Application/Src/process_orchestration/app_processes.c
  Application/Src/blazeface_face_detection/app_camerapipeline.c
  Application/Src/blazeface_face_detection/crop_img.c

Process 2: Model scheduler and AI
  Application/Src/process_orchestration/app_processes.c
  Application/Src/mobilefacenet_face_recognition/face_recog.c
  Application/Src/fastdepth_liveness/app_depth.c
  Application/Src/mobilefacenet_face_recognition/face_store.c
  Model/STM32N6570-DK/*
  Middlewares/ai-postprocessing-wrapper/*

Process 3: Auth decisions
  Application/Src/process_orchestration/app_processes.c
  Application/Src/mobilefacenet_face_recognition/face_recog.c
  Application/Src/mobilefacenet_face_recognition/face_store.c
  Application/Src/ui_services/app_ui.c

Process 4: UI and services
  Application/Src/process_orchestration/app_processes.c
  Application/Src/ui_services/app_ui.c
  Application/Src/ui_services/app_touch.c
  Application/Src/ui_services/app_buzzer.c
  Application/Src/tim_assistant/tim_app.c
  Application/Src/tim_assistant/tim_inference.c
  Application/Src/tim_assistant/tim_tokenizer.c
```

## Why We Did Not Use Linux-Style Processes

Linux-style process-oriented code would require:

```text
fork()
exec()
pipes
shared memory
message queues
semaphores
multiple executable binaries
an operating system with process isolation
```

This firmware targets STM32N6, so that model is not appropriate unless the project is
ported to an operating system that supports real processes. For this embedded firmware,
the correct version is cooperative process-oriented C.

## How To Explain This Design

Short explanation:

```text
The original firmware used a sequential super-loop where camera capture, AI inference,
authentication, and UI logic were mixed together. I converted it into a cooperative
process-oriented architecture with four processes. Each process owns one stage of the
pipeline, and the model scheduler decides which AI models run on each frame to reduce
latency while preserving recognition accuracy.
```

One-sentence version:

```text
The project is still one STM32 firmware image, but its main loop is now organized as four
cooperative processes: camera, model scheduler, authentication, and UI/services.
```

## Time and Space Optimizations Already Applied

The optimization target is MCU-friendly behavior: fewer wasted memory writes, fewer heavy
model runs, static buffers instead of heap allocation, and linker/compiler settings that
remove unused code.

Applied in the camera process:

```text
Removed full-frame crop-buffer memset on every frame.
Removed full NN input memset on every frame.
Only clears the padded tail of the NN input if the model buffer is larger than the pixels written.
Keeps camera/NN buffers static and aligned instead of allocating at runtime.
Uses pointer stepping in the RGB-to-float normalization loop.
```

Applied in the model process:

```text
BlazeFace runs every frame because it is the trigger model.
Face recognition waits for a stable single face.
Authentication recognition is rate-limited with a short recheck interval.
Main-screen recognition is rate-limited with a slower recheck interval.
FastDepth runs only when it adds security value, not blindly on every frame.
```

Applied in the image crop helper:

```text
img_crop() now has a fast path for contiguous source rows.
The source pointer is const, so callers and the compiler know it is not modified.
An unused include was removed.
```

Applied in the recognition and depth model helpers:

```text
Face recognition now resizes directly into the MobileFaceNet input buffer.
Removed the separate face-recognition crop staging buffer.
FastDepth now resizes directly into the FastDepth input buffer.
Removed the separate FastDepth input staging buffer.
Depth preview uses a direct memcpy when preview size equals model output size.
Depth resize avoids per-pixel division in the hot loop by using accumulators.
Depth scoring and embedding similarity use pointer stepping to reduce address math.
```

Approximate static RAM saved:

```text
FastDepth staging buffer: 150,528 bytes
Face recognition staging buffer: 37,632 bytes
Total direct buffer saving: 188,160 bytes
```

Existing project build optimizations:

```text
-Os
-ffunction-sections
-fdata-sections
--gc-sections
static aligned buffers for DMA/NPU data
```

Further optimization ideas:

```text
Measure real frame timing on UART before changing thresholds again.
Tune MODEL_SCHED_AUTH_RECHECK_MS and MODEL_SCHED_MAIN_RECHECK_MS for the best latency/accuracy balance.
Avoid rendering full UI areas every frame if only small text fields change.
Keep generated model buffers in the intended memory regions and avoid copying model outputs.
Use full project map files to find the largest RAM/flash users.
```
