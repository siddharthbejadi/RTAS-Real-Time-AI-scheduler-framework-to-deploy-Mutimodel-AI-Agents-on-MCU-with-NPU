

#ifndef PARTITION_STM32N657XX_H
#define PARTITION_STM32N657XX_H


#define SAU_INIT_CTRL          0


#define SAU_INIT_CTRL_ENABLE   0


#define SAU_INIT_CTRL_ALLNS  0


#define SAU_REGIONS_MAX   8


#define SAU_INIT_REGION0    0


#define SAU_INIT_START0     0x00000000


#define SAU_INIT_END0       0x00000000


#define SAU_INIT_NSC0       0


#define SAU_INIT_REGION1    0


#define SAU_INIT_START1     0x00000000


#define SAU_INIT_END1       0x00000000


#define SAU_INIT_NSC1       0


#define SAU_INIT_REGION2    0


#define SAU_INIT_START2     0x00000000


#define SAU_INIT_END2       0x00000000


#define SAU_INIT_NSC2       0


#define SAU_INIT_REGION3    0


#define SAU_INIT_START3     0x00000000


#define SAU_INIT_END3       0x00000000


#define SAU_INIT_NSC3       0


#define SAU_INIT_REGION4    0


#define SAU_INIT_START4     0x00000000


#define SAU_INIT_END4       0x00000000


#define SAU_INIT_NSC4       0


#define SAU_INIT_REGION5    0


#define SAU_INIT_START5     0x00000000


#define SAU_INIT_END5       0x00000000


#define SAU_INIT_NSC5       0


#define SAU_INIT_REGION6    0


#define SAU_INIT_START6     0x00000000


#define SAU_INIT_END6       0x00000000


#define SAU_INIT_NSC6       0


#define SAU_INIT_REGION7    0


#define SAU_INIT_START7     0x00000000


#define SAU_INIT_END7       0x00000000


#define SAU_INIT_NSC7       0


#define SCB_CSR_AIRCR_INIT  0


#define SCB_CSR_DEEPSLEEPS_VAL  0


#define SCB_AIRCR_SYSRESETREQS_VAL  0


#define SCB_AIRCR_PRIS_VAL      0


#define SCB_AIRCR_BFHFNMINS_VAL 0


#define TZ_FPU_NS_USAGE 1


#define SCB_NSACR_CP10_11_VAL       3


#define FPU_FPCCR_TS_VAL            0


#define FPU_FPCCR_CLRONRETS_VAL     0


#define FPU_FPCCR_CLRONRET_VAL      1


#define NVIC_INIT_ITNS0    1


#define NVIC_INIT_ITNS0_VAL     0x00000000


#define NVIC_INIT_ITNS1    1


#define NVIC_INIT_ITNS1_VAL     0x00000000


#define NVIC_INIT_ITNS2    1


#define NVIC_INIT_ITNS2_VAL     0x00000000


#define NVIC_INIT_ITNS3    1


#define NVIC_INIT_ITNS3_VAL     0x00000000


#define NVIC_INIT_ITNS4    1


#define NVIC_INIT_ITNS4_VAL      0x00000000


#define NVIC_INIT_ITNS5    1


#define NVIC_INIT_ITNS5_VAL      0x00000000


#define NVIC_INIT_ITNS6    1


#define NVIC_INIT_ITNS6_VAL      0x00000000


#define SAU_INIT_REGION(n) \
    SAU->RNR  =  (n                                     & SAU_RNR_REGION_Msk); \
    SAU->RBAR =  (SAU_INIT_START##n                     & SAU_RBAR_BADDR_Msk); \
    SAU->RLAR =  (SAU_INIT_END##n                       & SAU_RLAR_LADDR_Msk) | \
                ((SAU_INIT_NSC##n << SAU_RLAR_NSC_Pos)  & SAU_RLAR_NSC_Msk)   | 1U


__STATIC_INLINE void TZ_SAU_Setup (void)
{

#if defined (__SAUREGION_PRESENT) && (__SAUREGION_PRESENT == 1U)

  #if defined (SAU_INIT_REGION0) && (SAU_INIT_REGION0 == 1U)
    SAU_INIT_REGION(0);
  #endif

  #if defined (SAU_INIT_REGION1) && (SAU_INIT_REGION1 == 1U)
    SAU_INIT_REGION(1);
  #endif

  #if defined (SAU_INIT_REGION2) && (SAU_INIT_REGION2 == 1U)
    SAU_INIT_REGION(2);
  #endif

  #if defined (SAU_INIT_REGION3) && (SAU_INIT_REGION3 == 1U)
    SAU_INIT_REGION(3);
  #endif

  #if defined (SAU_INIT_REGION4) && (SAU_INIT_REGION4 == 1U)
    SAU_INIT_REGION(4);
  #endif

  #if defined (SAU_INIT_REGION5) && (SAU_INIT_REGION5 == 1U)
    SAU_INIT_REGION(5);
  #endif

  #if defined (SAU_INIT_REGION6) && (SAU_INIT_REGION6 == 1U)
    SAU_INIT_REGION(6);
  #endif

  #if defined (SAU_INIT_REGION7) && (SAU_INIT_REGION7 == 1U)
    SAU_INIT_REGION(7);
  #endif


#endif


  #if defined (SAU_INIT_CTRL) && (SAU_INIT_CTRL == 1U)
    SAU->CTRL = ((SAU_INIT_CTRL_ENABLE << SAU_CTRL_ENABLE_Pos) & SAU_CTRL_ENABLE_Msk) |
                ((SAU_INIT_CTRL_ALLNS  << SAU_CTRL_ALLNS_Pos)  & SAU_CTRL_ALLNS_Msk)   ;
  #endif

  #if defined (SCB_CSR_AIRCR_INIT) && (SCB_CSR_AIRCR_INIT == 1U)
    SCB->SCR   = (SCB->SCR   & ~(SCB_SCR_SLEEPDEEPS_Msk    )) |
                   ((SCB_CSR_DEEPSLEEPS_VAL     << SCB_SCR_SLEEPDEEPS_Pos)     & SCB_SCR_SLEEPDEEPS_Msk);

    SCB->AIRCR = (SCB->AIRCR & ~(SCB_AIRCR_VECTKEY_Msk   | SCB_AIRCR_SYSRESETREQS_Msk |
                                 SCB_AIRCR_BFHFNMINS_Msk | SCB_AIRCR_PRIS_Msk)        )                     |
                   ((0x05FAU                    << SCB_AIRCR_VECTKEY_Pos)      & SCB_AIRCR_VECTKEY_Msk)      |
                   ((SCB_AIRCR_SYSRESETREQS_VAL << SCB_AIRCR_SYSRESETREQS_Pos) & SCB_AIRCR_SYSRESETREQS_Msk) |
                   ((SCB_AIRCR_PRIS_VAL         << SCB_AIRCR_PRIS_Pos)         & SCB_AIRCR_PRIS_Msk)         |
                   ((SCB_AIRCR_BFHFNMINS_VAL    << SCB_AIRCR_BFHFNMINS_Pos)    & SCB_AIRCR_BFHFNMINS_Msk);
  #endif

  #if defined (__FPU_USED) && (__FPU_USED == 1U) && \
      defined (TZ_FPU_NS_USAGE) && (TZ_FPU_NS_USAGE == 1U)

    SCB->NSACR = (SCB->NSACR & ~(SCB_NSACR_CP10_Msk | SCB_NSACR_CP11_Msk)) |
                   ((SCB_NSACR_CP10_11_VAL << SCB_NSACR_CP10_Pos) & (SCB_NSACR_CP10_Msk | SCB_NSACR_CP11_Msk));

    FPU->FPCCR = (FPU->FPCCR & ~(FPU_FPCCR_TS_Msk | FPU_FPCCR_CLRONRETS_Msk | FPU_FPCCR_CLRONRET_Msk)) |
                   ((FPU_FPCCR_TS_VAL        << FPU_FPCCR_TS_Pos       ) & FPU_FPCCR_TS_Msk       ) |
                   ((FPU_FPCCR_CLRONRETS_VAL << FPU_FPCCR_CLRONRETS_Pos) & FPU_FPCCR_CLRONRETS_Msk) |
                   ((FPU_FPCCR_CLRONRET_VAL  << FPU_FPCCR_CLRONRET_Pos ) & FPU_FPCCR_CLRONRET_Msk );
  #endif

  #if defined (NVIC_INIT_ITNS0) && (NVIC_INIT_ITNS0 == 1U)
    NVIC->ITNS[0] = NVIC_INIT_ITNS0_VAL;
  #endif

  #if defined (NVIC_INIT_ITNS1) && (NVIC_INIT_ITNS1 == 1U)
    NVIC->ITNS[1] = NVIC_INIT_ITNS1_VAL;
  #endif

  #if defined (NVIC_INIT_ITNS2) && (NVIC_INIT_ITNS2 == 1U)
    NVIC->ITNS[2] = NVIC_INIT_ITNS2_VAL;
  #endif

  #if defined (NVIC_INIT_ITNS3) && (NVIC_INIT_ITNS3 == 1U)
    NVIC->ITNS[3] = NVIC_INIT_ITNS3_VAL;
  #endif

  #if defined (NVIC_INIT_ITNS4) && (NVIC_INIT_ITNS4 == 1U)
    NVIC->ITNS[4] = NVIC_INIT_ITNS4_VAL;
  #endif

  #if defined (NVIC_INIT_ITNS5) && (NVIC_INIT_ITNS5 == 1U)
    NVIC->ITNS[5] = NVIC_INIT_ITNS5_VAL;
  #endif

  #if defined (NVIC_INIT_ITNS6) && (NVIC_INIT_ITNS6 == 1U)
    NVIC->ITNS[6] = NVIC_INIT_ITNS6_VAL;
  #endif

}

#endif
