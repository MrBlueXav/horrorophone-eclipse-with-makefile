/**
  ******************************************************************************
  * @file     main.c
  * @author  Xavier Halgand
  * @version
  * @date    october 2012
  * @brief   Press the user button and you will hear a sequence of random notes
  * produced by a sawtooth oscillator.
  * A pot wired on PC2 will control the speed of the sequence.
  * The oscillator is an alias-free minBLEP based generator :
  * Thanks to Sean Bolton (blepvco), Fons Adriaensen, Eli Brandt, ... for their work !
  ******************************************************************************
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
  */

/*----------------------------- Includes --------------------------------------*/

#include "main.h"
#include "stm32f4_discovery.h"
#include "minblep_tables.h"
#include "saw_osc.h"
/*******************************************************************************/

extern void sawtooth_active (void);
extern void sawtooth_runproc (uint16_t offset, unsigned long len);


/* -------------Global variables ---------------------------------------------*/

uint16_t        audiobuff[BUFF_LEN] = {0};  // The circular audio buffer
float           delayline[DELAYLINE_LEN] = {0.f};
float           phase2 = 0.0f , phase2Step;
float           f1 = FREQ1 , f2 = FREQ2 , freq;
float           delayVol = DELAY_VOLUME;
float           *readpos; // output pointer of delay line
float           *writepos; // input pointer of delay line
float           fdb = FEEDB;
float           pass = 1.f ;

__IO uint16_t 			ADC3ConvertedValue = 0;
RCC_ClocksTypeDef       RCC_Clocks;
GPIO_InitTypeDef        GPIO_InitStructure;
uint8_t                 state = OFF;
__IO uint32_t 	TimingDelay = 50;

/***------------- sawtooth osc variables -----------------------------------*/

float 	_fsam = SAMPLERATE;
float   _p, _w, _z;
float   _f [FILLEN + STEP_DD_PULSE_LENGTH];
uint16_t     _j, _init;


/* ----- function prototypes -----------------------------------------------*/

void ADC3_CH12_DMA_Config(void);
void Delay(__IO uint32_t nTime);

/*=============================== MAIN ======================================
==============================================================================*/

int main(void)
{
  /* Initialize LEDS */
  STM_EVAL_LEDInit(LED3); // orange LED
  STM_EVAL_LEDInit(LED4); // green LED
  STM_EVAL_LEDInit(LED5); // red LED
  STM_EVAL_LEDInit(LED6); // blue LED

  /* Green Led On: start of application */
  STM_EVAL_LEDOn(LED4);


  /* Initialize User Button */
  STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_GPIO);


  /* Initialise the onboard random number generator ! */
  RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG, ENABLE);
  RNG_Cmd(ENABLE);

  /* ADC3 configuration *******************************************************/
   /*  - Enable peripheral clocks                                              */
   /*  - DMA2_Stream0 channel2 configuration                                   */
   /*  - Configure ADC Channel12 pin as analog input  : PC2                    */
   /*  - Configure ADC3 Channel12                                              */
   ADC3_CH12_DMA_Config();

   /* Start ADC3 Software Conversion */
   ADC_SoftwareStartConv(ADC3);

   /***************************************************************************/


   if (SysTick_Config(SystemCoreClock / 50))  // 20 ms tick
     {
       /* Capture error */
       while (1);
     }

  readpos = delayline;
  writepos = delayline + DELAY;

  sawtooth_active();


  EVAL_AUDIO_Init( OUTPUT_DEVICE_AUTO, VOL, SAMPLERATE);
  EVAL_AUDIO_Play((uint16_t*)audiobuff, BUFF_LEN);


  while (1)
  {
    //Interrupts handlers are doing their job...;

    /* Let's poll the user button to hear the sounds */

    if (STM_EVAL_PBGetState(BUTTON_USER) && (state == OFF))
    {
      state = ON;
      STM_EVAL_LEDOn(LED6); // blue LED ON
      pass = 0.5f;
    }
    else
    {
      if (! STM_EVAL_PBGetState(BUTTON_USER))
      {
        STM_EVAL_LEDOff(LED6); // blue LED OFF
        pass = 0.0f;
        state = OFF;
      }
    }
  }
}
/*============================== End of main ===================================
==============================================================================*/

/**
  * @brief  Inserts a delay time.
  * @param  nTime: specifies the delay time length.
  * @retval None
  */
void Delay(__IO uint32_t nTime)
{
  TimingDelay = nTime;

  while(TimingDelay != 0);
}
//---------------------------------------------------------------------------
/**
  * @brief  Decrements the TimingDelay variable.
  * @param  None
  * @retval None
  */
void TimingDelay_Decrement(void)
{
  if (TimingDelay != 0x00)
  {
    TimingDelay--;
  }
}
//---------------------------------------------------------------------------

/**
  * @brief  Basic management of the timeout situation.
  * @param  None
  * @retval None
  */
uint32_t Codec_TIMEOUT_UserCallback(void)
{
	STM_EVAL_LEDOn(LED5); /*  alert : red LED !  */
	return (0);
}
//---------------------------------------------------------------------------
/**
* @brief  Manages the DMA Half Transfer complete interrupt.
* @param  None
* @retval None
*/
void EVAL_AUDIO_HalfTransfer_CallBack(uint32_t pBuffer, uint32_t Size)
{
  /* Generally this interrupt routine is used to load the buffer when
  a streaming scheme is used: When first Half buffer is already transferred load
  the new data to the first half of buffer while DMA is transferring data from
  the second half. And when Transfer complete occurs, load the second half of
  the buffer while the DMA is transferring from the first half ... */

	sawtooth_runproc(0, BUFF_LEN_DIV4);

}
//---------------------------------------------------------------------------
/**
* @brief  Manages the DMA Complete Transfer complete interrupt.
* @param  None
* @retval None
*/
void EVAL_AUDIO_TransferComplete_CallBack(uint32_t pBuffer, uint32_t Size)
{

	sawtooth_runproc(BUFF_LEN_DIV2, BUFF_LEN_DIV4);

}
//---------------------------------------------------------------------------
/**
* @brief  Get next data sample callback
* @param  None
* @retval Next data sample to be sent
*/
uint16_t EVAL_AUDIO_GetSampleCallBack(void)
{
  return 0;
}
//---------------------------------------------------------------------------
/**************
* returns a random float between 0 and 1
*****************/
float randomNum(void)
  {
    float random = 1.0f;
    if (RNG_GetFlagStatus(RNG_FLAG_DRDY) == SET)
    {
      random = (float)(RNG_GetRandomNumber()/4294967294.0f);
    }
  return random;
  }
//---------------------------------------------------------------------------
/**
  * @brief  ADC3 channel12 with DMA configuration
  * @param  None
  * @retval None
  */
void ADC3_CH12_DMA_Config(void)
{
  ADC_InitTypeDef       ADC_InitStructure;
  ADC_CommonInitTypeDef ADC_CommonInitStructure;
  DMA_InitTypeDef       DMA_InitStructure;
  GPIO_InitTypeDef      GPIO_InitStructure;

  /* Enable ADC3, DMA2 and GPIO clocks ****************************************/
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2 | RCC_AHB1Periph_GPIOC, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC3, ENABLE);

  /* DMA2 Stream0 channel0 configuration **************************************/
  DMA_InitStructure.DMA_Channel = DMA_Channel_2;
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)ADC3_DR_ADDRESS;
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&ADC3ConvertedValue;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_BufferSize = 1;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_Init(DMA2_Stream0, &DMA_InitStructure);
  DMA_Cmd(DMA2_Stream0, ENABLE);

  /* Configure ADC3 Channel12 pin as analog input ******************************/
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  /* ADC Common Init **********************************************************/
  ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;
  ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
  ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
  ADC_CommonInit(&ADC_CommonInitStructure);

  /* ADC3 Init ****************************************************************/
  ADC_InitStructure.ADC_Resolution = ADC_Resolution_8b;
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
  ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfConversion = 1;
  ADC_Init(ADC3, &ADC_InitStructure);

  /* ADC3 regular channel12 configuration *************************************/
  ADC_RegularChannelConfig(ADC3, ADC_Channel_12, 1, ADC_SampleTime_3Cycles);

 /* Enable DMA request after last transfer (Single-ADC mode) */
  ADC_DMARequestAfterLastTransferCmd(ADC3, ENABLE);

  /* Enable ADC3 DMA */
  ADC_DMACmd(ADC3, ENABLE);

  /* Enable ADC3 */
  ADC_Cmd(ADC3, ENABLE);
}

/***********************END OF FILE**********************************************/
