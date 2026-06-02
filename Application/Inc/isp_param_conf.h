

#ifndef __ISP_PARAM_CONF__H
#define __ISP_PARAM_CONF__H

#include "app_config.h"
#include "cmw_camera.h"

#include "imx335_isp_param_conf.h"
#include "vd66gy_isp_param_conf.h"
#include "vd1943_isp_param_conf.h"

static const ISP_IQParamTypeDef* ISP_IQParamCacheInit[] = {
    &ISP_IQParamCacheInit_IMX335,
    &ISP_IQParamCacheInit_VD66GY,
    &ISP_IQParamCacheInit_VD1943
};

#endif
