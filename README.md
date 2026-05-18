# implementing RTAS-MCU

An embedded AI secure-access system built on the `STM32N6570-DK` platform.

This project uses on-device computer vision and embedded UI logic to authenticate users with face recognition, support face enrollment, store authorized users in flash, and add an extra depth-based live check to reduce simple spoofing attempts.

Everything runs locally on the STM32N6 platform without requiring a cloud connection.

## Project Overview

The goal of this project is to turn the STM32N6570 Discovery Kit into a smart access-control prototype.

The system:

- detects a face from the live camera feed
- extracts a face embedding for identity matching
- checks a depth template for an additional liveness-style verification step
- grants or denies access based on the stored enrolled profiles
- provides a touchscreen UI for authentication, enrollment, settings, and logout
- stores enrolled users persistently in external flash memory
- includes a simple TIM assistant interface for access and system commands

## Main Features

- Real-time face detection using `BlazeFace`
- Face recognition using `MobileFaceNet`
- Depth estimation / liveness support using `FastDepth`
- Touchscreen-based user interface
- User enrollment directly from the board
- Persistent face database stored in XSPI flash
- Admin / non-admin user roles
- Buzzer feedback for authentication and UI events
- UART debug output and TIM text-command support
- Fully on-device inference on the STM32N6 NPU

## What I Used

### Hardware

- `STM32N6570-DK` Discovery Kit
- On-board / supported camera pipeline through ST camera middleware
- LCD display
- Touch controller (`GT911`)
- Buzzer
- XSPI flash for persistent storage

### Software / Frameworks

- `C`
- `STM32CubeIDE`
- `STM32Cube HAL / BSP`
- `STM32Cube_FW_N6`
- `STEdgeAI`
- `LL_ATON` NPU runtime
- ST camera middleware
- ST vision post-processing middleware
- `arm-none-eabi-gcc`
- `STM32_Programmer_CLI` / STM32CubeProgrammer

### AI Models Used

- `blazeface_front_128_int8_OE_3_3_1` for face detection
- `mobilefacenet_int8_faces_OE_3_3_1` for face recognition embeddings
- `fastdepth_224_int8_OE_3_3_1` for depth estimation

Note: TIM intent support exists in the project, but the current default build uses a lightweight rule-based intent parser in `Application/Src/tim_assistant/tim_inference.c`. Generated TIM model files are also present in the `Model` folder for future use.

## How It Works

1. `CameraFrame_Process()` captures and prepares a live frame.
2. `ModelScheduler_Process()` runs BlazeFace every frame and schedules MobileFaceNet / FastDepth only when useful.
3. `AuthDecision_Process()` converts model evidence into enrollment and authentication decisions.
4. `UiSystem_Process()` polls services and renders the UI.

## Current System Rules

- Maximum enrolled users: `5`
- The first enrolled user becomes the default admin
- Face names are stored with a maximum length of `20` characters
- Face embeddings and depth templates are stored persistently in external flash
- Authentication thresholds are configurable in `Application/Inc/app_config.h`

## Repository Structure

```text
Application/
  Inc/                  Header files
  Src/                  Main application source
    main.c              Top-level four-process loop
    blazeface_face_detection/       BlazeFace camera input and crop helpers
    fastdepth_liveness/ FastDepth liveness/depth application code
    mobilefacenet_face_recognition/ Face recognition and face store
    platform_system/    Fuse, syscalls, interrupt, and LCD support
    process_orchestration/ Four-process implementation
    tim_assistant/      TIM command / intent support
    ui_services/        UI, touch, and buzzer services
  STM32CubeIDE/         STM32CubeIDE project files
Drivers/                Driver sources
Middlewares/            ST middleware and AI runtime
Model/                  Generated AI model sources and binaries
STM32Cube_FW_N6/        STM32 firmware package content
Utilities/              Utility modules
PROCESS_ORIENTED_ARCHITECTURE.md  Four-process firmware architecture
UI_FLOWCHART.md                   UI state flow documentation
```

## Important Source Files

- `Application/Src/main.c` - startup plus the four-process cooperative main loop
- `Application/Src/process_orchestration/app_processes.c` - camera, model scheduler, auth decision, and UI/service process bodies
- `Application/Inc/app_processes.h` - public process API used by `main.c`
- `Application/Src/mobilefacenet_face_recognition/face_recog.c` - face embedding extraction and identification
- `Application/Src/mobilefacenet_face_recognition/face_store.c` - persistent storage of enrolled users
- `Application/Src/fastdepth_liveness/app_depth.c` - depth inference and live-check scoring
- `Application/Src/ui_services/app_ui.c` - UI states and rendering logic
- `Application/Src/ui_services/app_touch.c` - touch input handling
- `Application/Src/tim_assistant/tim_app.c` - TIM chat / command handling
- `Application/Inc/app_config.h` - thresholds and application configuration

## Build and Run

### Using STM32CubeIDE

1. Open `STM32CubeIDE`
2. Import the project from `Application/STM32CubeIDE`
3. Build the project
4. Connect the `STM32N6570-DK`
5. Flash and run the generated target

The STM32CubeIDE project name is:

```text
implementing RTAS-MCU
```

### Build Requirements

Make sure these tools are installed:

- `STM32CubeIDE`
- `STM32CubeProgrammer`
- `arm-none-eabi-gcc`
- ST Edge AI toolchain required by the generated model flow

## Using the System

- On startup, the board enters the authentication screen
- If a known user is recognized and the depth check passes, access is granted
- From the main screen, the user can:
  - add a new person
  - open settings
  - log out
  - use the TIM command input
- Admin users can manage enrolled users and delete profiles

For the UI state machine, see [UI_FLOWCHART.md](UI_FLOWCHART.md).

## Notes

- This project is based on STM32N6 embedded AI components and generated model artifacts.
- Some project files come from ST firmware, BSP, middleware, and generated STEdgeAI outputs.
- If you make this repository public, it is a good idea to keep the original ST license files and review third-party license terms before publishing.

## Future Improvements

- stronger anti-spoofing / liveness checks
- better user management and audit logging
- door lock or relay integration
- encrypted storage for enrolled templates
- full TIM model inference enabled by default
- remote monitoring or event logging

## License

This repository contains both custom application code and third-party ST components.

Please review the license files included in the project before publishing or reusing the code publicly.
