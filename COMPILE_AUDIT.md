# Compile Audit

CubeIDE is now configured to exclude the physical `Drivers` and `Middlewares` source trees and compile explicit linked sources only.

Linked compile units: 74
Missing linked resources: 0

## Linked Compile Units

- `Application/app_buzzer.c`
- `Application/app_camerapipeline.c`
- `Application/app_fuseprogramming.c`
- `Application/app_postprocess_fd_blazeface_ui.c`
- `Application/app_postprocess_od_blazeface_ui.c`
- `Application/app_touch.c`
- `Application/app_ui.c`
- `Application/bsp/stm32n6570_discovery.c`
- `Application/bsp/stm32n6570_discovery_bus.c`
- `Application/bsp/stm32n6570_discovery_lcd.c`
- `Application/bsp/stm32n6570_discovery_xspi.c`
- `Application/camera/cmw_camera.c`
- `Application/camera/cmw_imx335.c`
- `Application/camera/cmw_utils.c`
- `Application/camera/imx335.c`
- `Application/camera/imx335_reg.c`
- `Application/components/aps256xx.c`
- `Application/components/mx66uw1g45g.c`
- `Application/crop_img.c`
- `Application/face_recog.c`
- `Application/face_store.c`
- `Application/hal/stm32n6xx_hal.c`
- `Application/hal/stm32n6xx_hal_bsec.c`
- `Application/hal/stm32n6xx_hal_cacheaxi.c`
- `Application/hal/stm32n6xx_hal_cortex.c`
- `Application/hal/stm32n6xx_hal_dcmipp.c`
- `Application/hal/stm32n6xx_hal_dma.c`
- `Application/hal/stm32n6xx_hal_dma_ex.c`
- `Application/hal/stm32n6xx_hal_dma2d.c`
- `Application/hal/stm32n6xx_hal_gpio.c`
- `Application/hal/stm32n6xx_hal_i2c.c`
- `Application/hal/stm32n6xx_hal_i2c_ex.c`
- `Application/hal/stm32n6xx_hal_ltdc.c`
- `Application/hal/stm32n6xx_hal_ltdc_ex.c`
- `Application/hal/stm32n6xx_hal_pwr.c`
- `Application/hal/stm32n6xx_hal_pwr_ex.c`
- `Application/hal/stm32n6xx_hal_ramcfg.c`
- `Application/hal/stm32n6xx_hal_rcc.c`
- `Application/hal/stm32n6xx_hal_rcc_ex.c`
- `Application/hal/stm32n6xx_hal_rif.c`
- `Application/hal/stm32n6xx_hal_uart.c`
- `Application/hal/stm32n6xx_hal_xspi.c`
- `Application/isp/isp_ae_algo.c`
- `Application/isp/isp_algo.c`
- `Application/isp/isp_awb_algo.c`
- `Application/isp/isp_cmd_parser.c`
- `Application/isp/isp_core.c`
- `Application/isp/isp_services.c`
- `Application/isp/isp_tool_com.c`
- `Application/main.c`
- `Application/mcu_cache.c`
- `Application/network_embed.c`
- `Application/network_face.c`
- `Application/npu_cache.c`
- `Application/postprocess/od_pp_blazeface.c`
- `Application/postprocess/vision_models_pp.c`
- `Application/stai_network_embed.c`
- `Application/stai_network_face.c`
- `Application/stm32_lcd_ex.c`
- `Application/stm32n6xx_it.c`
- `Application/syscalls.c`
- `Application/system/system_stm32n6xx_fsbl.c`
- `ll_aton/ll_aton.c`
- `ll_aton/ll_aton_cipher.c`
- `ll_aton/ll_aton_lib.c`
- `ll_aton/ll_aton_lib_sw_operators.c`
- `ll_aton/ll_aton_rt_main.c`
- `ll_aton/ll_aton_runtime.c`
- `ll_aton/ll_aton_stai_internal.c`
- `ll_aton/ll_aton_util.c`
- `ll_aton/ll_sw_float.c`
- `ll_aton/ll_sw_integer.c`
- `startup_stm32n657xx.s`
- `Utilities/lcd/stm32_lcd.c`

## Kept For Touch

- `Application/app_touch.c`
- `Application/bsp/stm32n6570_discovery_bus.c`
- `Application/hal/stm32n6xx_hal_i2c.c`
- `Application/hal/stm32n6xx_hal_i2c_ex.c`
- `Application/hal/stm32n6xx_hal_gpio.c`
- `Application/hal/stm32n6xx_hal.c`

## Deliberately Not Compiled

- HAL templates: `stm32n6xx_hal_timebase_*_template.c`, `stm32n6xx_hal_msp_template.c`
- LL drivers like `stm32n6xx_ll_adc.c` and `stm32n6xx_ll_crc.c`
- Unused camera sensors: VD/OV component sources
- BSP touch component `stm32n6570_discovery_ts.c` and `gt911.c`, because the app uses the custom minimal GT911 driver in `Application/app_touch.c`
