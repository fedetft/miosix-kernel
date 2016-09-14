/***************************************************************************
 *  FIXXX ME
 ***************************************************************************/

#ifndef PORTABILITY_H
#define	PORTABILITY_H


#define DMA_CHANNEL_ADC                   0

#if ( ( DMA_CHAN_COUNT > 4 ) && ( DMA_CHAN_COUNT <= 8 ) )
#define DMACTRL_CH_CNT      8
#define DMACTRL_ALIGNMENT   256

#elif ( ( DMA_CHAN_COUNT > 8 ) && ( DMA_CHAN_COUNT <= 16 ) )
#define DMACTRL_CH_CNT      16
#define DMACTRL_ALIGNMENT   512

#endif

typedef enum
{
  dmaDataInc1    = _DMA_CTRL_SRC_INC_BYTE,     /**< Increment address 1 byte. */
  dmaDataInc2    = _DMA_CTRL_SRC_INC_HALFWORD, /**< Increment address 2 bytes. */
  dmaDataInc4    = _DMA_CTRL_SRC_INC_WORD,     /**< Increment address 4 bytes. */
  dmaDataIncNone = _DMA_CTRL_SRC_INC_NONE      /**< Do not increment address. */
} DMA_DataInc_TypeDef;

typedef enum
{
  dmaDataSize1 = _DMA_CTRL_SRC_SIZE_BYTE,     /**< 1 byte DMA transfer size. */
  dmaDataSize2 = _DMA_CTRL_SRC_SIZE_HALFWORD, /**< 2 byte DMA transfer size. */
  dmaDataSize4 = _DMA_CTRL_SRC_SIZE_WORD      /**< 4 byte DMA transfer size. */
} DMA_DataSize_TypeDef;


typedef enum
{
    dmaCycleCtrlBasic            = _DMA_CTRL_CYCLE_CTRL_BASIC,
    dmaCycleCtrlAuto             = _DMA_CTRL_CYCLE_CTRL_AUTO,
    dmaCycleCtrlPingPong         = _DMA_CTRL_CYCLE_CTRL_PINGPONG,
    dmaCycleCtrlMemScatterGather = _DMA_CTRL_CYCLE_CTRL_MEM_SCATTER_GATHER,
    dmaCycleCtrlPerScatterGather = _DMA_CTRL_CYCLE_CTRL_PER_SCATTER_GATHER
          
} DMA_CycleCtrl_TypeDef;

typedef enum
{
    dmaArbitrate1    = _DMA_CTRL_R_POWER_1,    /**< Arbitrate after 1 DMA transfer. */
    dmaArbitrate2    = _DMA_CTRL_R_POWER_2,    /**< Arbitrate after 2 DMA transfers. */
    dmaArbitrate4    = _DMA_CTRL_R_POWER_4,    /**< Arbitrate after 4 DMA transfers. */
    dmaArbitrate8    = _DMA_CTRL_R_POWER_8,    /**< Arbitrate after 8 DMA transfers. */
    dmaArbitrate16   = _DMA_CTRL_R_POWER_16,   /**< Arbitrate after 16 DMA transfers. */
    dmaArbitrate32   = _DMA_CTRL_R_POWER_32,   /**< Arbitrate after 32 DMA transfers. */
    dmaArbitrate64   = _DMA_CTRL_R_POWER_64,   /**< Arbitrate after 64 DMA transfers. */
    dmaArbitrate128  = _DMA_CTRL_R_POWER_128,  /**< Arbitrate after 128 DMA transfers. */
    dmaArbitrate256  = _DMA_CTRL_R_POWER_256,  /**< Arbitrate after 256 DMA transfers. */
    dmaArbitrate512  = _DMA_CTRL_R_POWER_512,  /**< Arbitrate after 512 DMA transfers. */
    dmaArbitrate1024 = _DMA_CTRL_R_POWER_1024  /**< Arbitrate after 1024 DMA transfers. */

} DMA_ArbiterConfig_TypeDef;

typedef struct
{
  uint8_t hprot;    //priviliged or not
  DMA_DESCRIPTOR_TypeDef *controlBlock;
} DMA_Init_TypeDef;

typedef struct
{
    DMA_DataInc_TypeDef       dstInc;
    DMA_DataInc_TypeDef       srcInc;
    DMA_DataSize_TypeDef      size;
    DMA_ArbiterConfig_TypeDef arbRate;
    uint8_t hprot;
    
} DMA_CfgDescr_TypeDef;

typedef void (*DMA_FuncPtr_TypeDef)(unsigned int channel, bool primary, void *user);

typedef struct
{
     DMA_FuncPtr_TypeDef cbFunc;
    void                *userPtr;
    uint8_t             primary;
    
} DMA_CB_TypeDef;

typedef struct
{
    bool     highPri;
    bool     enableInt;
    uint32_t select;
    DMA_CB_TypeDef *cb;
    
} DMA_CfgChannel_TypeDef;

void DMA_Pause(void);
void DMA_begin(void);

void setup_DMA(DMA_CB_TypeDef cbtemp);
void DMA_CfgChannel(unsigned int channel, DMA_CfgChannel_TypeDef *cfg);
void DMA_Reset(void);
void DMA_CfgDescr(unsigned int channel, bool primary,DMA_CfgDescr_TypeDef *cfg);
void DMA_Init(DMA_Init_TypeDef *init);

void DMA_ActivatePingPong(unsigned int channel,bool useBurst,
                          void *primDst,void *primSrc,unsigned int primNMinus1,
                          void *altDst,void *altSrc,unsigned int altNMinus1);

void DMA_RefreshPingPong(unsigned int channel, bool primary,bool useBurst,
                         void *dst, void *src, unsigned int nMinus1,bool stop);

void DMA_Prepare(unsigned int channel,DMA_CycleCtrl_TypeDef cycleCtrl,
                 bool primary,bool useBurst,void *dst,void *src,unsigned int nMinus1);
    
void waitRtc(int val);
void initRtc(); 
void IRQdeepSleep();
    


#endif
