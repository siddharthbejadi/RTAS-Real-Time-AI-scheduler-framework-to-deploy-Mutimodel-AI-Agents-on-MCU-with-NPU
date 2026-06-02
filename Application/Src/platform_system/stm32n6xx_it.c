

#include "stm32n6xx_hal.h"
#include "stm32n6xx_it.h"

#include "cmw_camera.h"

extern volatile uint32_t g_fault_cfsr;
extern volatile uint32_t g_fault_hfsr;
extern volatile uint32_t g_fault_bfar;
extern volatile uint32_t g_fault_mmfar;
extern volatile uint32_t g_fault_break_seen;


void NMI_Handler(void)
{
}


void HardFault_Handler(void)
{
	g_fault_cfsr = SCB->CFSR;
	  g_fault_hfsr = SCB->HFSR;
	  g_fault_bfar = SCB->BFAR;
	  g_fault_mmfar = SCB->MMFAR;
	  if (g_fault_break_seen == 0U)
	  {
	    g_fault_break_seen = 1U;
	    __BKPT(0);
	  }


  while (1)
  {
  }
}


void MemManage_Handler(void)
{

  while (1)
  {
  }
}


void BusFault_Handler(void)
{

  while (1)
  {
  }
}


void UsageFault_Handler(void)
{

  while (1)
  {
  }
}


void SecureFault_Handler(void)
{

  while (1)
  {
  }
}


void SVC_Handler(void)
{
}


void DebugMon_Handler(void)
{
  while (1)
  {
  }
}


void PendSV_Handler(void)
{
  while (1)
  {
  }
}


void SysTick_Handler(void)
{
  HAL_IncTick();
}


void CSI_IRQHandler(void)
{
  DCMIPP_HandleTypeDef *hcamera_dcmipp = CMW_CAMERA_GetDCMIPPHandle();
  HAL_DCMIPP_CSI_IRQHandler(hcamera_dcmipp);
}

void DCMIPP_IRQHandler(void)
{
  DCMIPP_HandleTypeDef *hcamera_dcmipp = CMW_CAMERA_GetDCMIPPHandle();
  HAL_DCMIPP_IRQHandler(hcamera_dcmipp);
}
