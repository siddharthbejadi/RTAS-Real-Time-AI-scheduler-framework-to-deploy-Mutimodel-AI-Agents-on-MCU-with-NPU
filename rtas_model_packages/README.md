# RTAS model packages

This folder is the RTAS model drop zone. Each model package should contain the
ST Edge AI generated files inside a `generated` folder:

```text
rtas_model_packages/
  <model_id>/
    generated/
      stai_*.c
      stai_*.h
      *.c
      *.h
      *_atonbuf.xSPI2.bin
```

During the build, RTAS runs this flow automatically:

```text
tools/rtas_scan_packages.c
  -> scans rtas_model_packages/*/generated
  -> updates rtas_model_packages/rtas_models.pbtxt

tools/rtas_codegen.c
  -> reads rtas_models.pbtxt
  -> updates Application/Inc/rtas_generated_config.h
```

`rtas_models.pbtxt` is generated. Do not edit it by hand. If a model is
changed, replaced, or added, rebuild the project and RTAS will refresh the
registry from the generated files.

RTAS can auto-detect artifact names, STAI API prefix, ATON binary, tensor
shapes, data type, layout, and buffer sizes. The model role is inferred from
controlled naming rules:

```text
embed/facenet  -> face_embedding
depth/liveness -> depth_liveness
pose/keypoint  -> pose_estimation
blazeface/face -> face_detection
detect/yolo    -> object_detection
otherwise      -> generic_tensor
```

For a new role to become a full pipeline, the firmware still needs a matching
adapter for preprocessing, postprocessing, scheduling, and UI drawing. This is
intentional: RTAS automates the package and build integration, while keeping the
runtime behavior controlled.

Current CubeIDE sources are still wired for the active face detection, face
embedding, and depth liveness packages. New package discovery is automatic, but
executing a new role in firmware still needs the matching adapter and source
integration step.
