

#include "stm32n6xx_hal_bsec.h"
static void ErrorHandler(void);

#define BSEC_HW_CONFIG_ID        124U
#define BSEC_HWS_HSLV_VDDIO3     15U
#define BSEC_HWS_HSLV_VDDIO2     16U

#define BSEC_FUSE_ADDRESS        BSEC_HW_CONFIG_ID
#define BSEC_FUSE_MASK           ((uint32_t)((1U << BSEC_HWS_HSLV_VDDIO3) | (1U << BSEC_HWS_HSLV_VDDIO2)));


void Fuse_Programming(void)
{
  uint32_t fuse_id, bit_mask, data;

  BSEC_HandleTypeDef sBsecHandler;

  sBsecHandler.Instance = BSEC;


  fuse_id = BSEC_FUSE_ADDRESS;
  if (HAL_BSEC_OTP_Read(&sBsecHandler, fuse_id, &data) == HAL_OK)
  {

    bit_mask = BSEC_FUSE_MASK;
    if ((data & bit_mask) != bit_mask)
    {
      data |= bit_mask;

      if (HAL_BSEC_OTP_Program(&sBsecHandler, fuse_id, data, HAL_BSEC_NORMAL_PROG) == HAL_OK)
      {

        if (HAL_BSEC_OTP_Read(&sBsecHandler, fuse_id, &data) == HAL_OK)
        {
          if ((data & bit_mask) != bit_mask)
          {

            ErrorHandler();
          }
        }
        else
        {

          ErrorHandler();
        }
      }
      else
      {

        ErrorHandler();
      }
    }
    else
    {

    }
  }
  else
  {

    ErrorHandler();
  }
}


static void ErrorHandler(void)
{
  while(1);
}
