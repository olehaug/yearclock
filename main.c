#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_pwr.h>
#include <stm32f10x_rtc.h>
#include <stm32f10x_bkp.h>
#include <stdbool.h>

void delay(int millis) {
    while (millis-- > 0) {
        volatile int x = 5971;
        while (x-- > 0) {
            __asm("nop");
        }
    }
}

#define RTC_LSB_MASK ((uint32_t)0x0000FFFF)
void RTC_UpdateAlarm(uint32_t delta)
{
    uint32_t alarm;
    RTC_WaitForLastTask();
    RTC_EnterConfigMode();
    alarm = RTC->ALRH << 16;
    alarm &= (RTC->ALRL & RTC_LSB_MASK);
    alarm += delta;
    RTC->ALRH = alarm >> 16;
    RTC->ALRL = (alarm & RTC_LSB_MASK);
    RTC_ExitConfigMode();
    RTC_WaitForLastTask();
}

void tick()
{
    uint16_t polarity = BKP_ReadBackupRegister(BKP_DR1);
    if(polarity)
    {
        BKP_WriteBackupRegister(BKP_DR1, 0);
        GPIO_SetBits(GPIOB, GPIO_Pin_7);
        delay(25);
        GPIO_ResetBits(GPIOB, GPIO_Pin_7);
    }
    else
    {
        BKP_WriteBackupRegister(BKP_DR1, 1);
        GPIO_SetBits(GPIOB, GPIO_Pin_9);
        delay(25);
        GPIO_ResetBits(GPIOB, GPIO_Pin_9);
    }
}


int main(void) {

    // GPIO structure for port initialization
    GPIO_InitTypeDef GPIO_InitStructure;

    // enable clock on APB2
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB1Periph_BKP,  ENABLE);

    // configure port A1 for driving an LED
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;    // output push-pull mode
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;   // highest speed
    GPIO_Init(GPIOB, &GPIO_InitStructure) ;             // initialize port


    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);

    PWR_BackupAccessCmd(ENABLE);

    if(PWR_GetFlagStatus(PWR_FLAG_SB) != RESET)
    { /* Resumed from standby */
        PWR_ClearFlag(PWR_FLAG_SB);
        RTC_WaitForSynchro();

        RTC_WaitForLastTask();
        RTC_SetAlarm(RTC_GetCounter() + 729);
        RTC_WaitForLastTask();
    }
    else
    { /* Fresh boot */

        // Enable LSE
        RCC_LSEConfig(RCC_LSE_ON);
        while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);

        // Set RTC source
        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);

        // Enable RTC
        RCC_RTCCLKCmd(ENABLE);
        BKP_TamperPinCmd(DISABLE);
        BKP_RTCOutputConfig(BKP_RTCOutputSource_CalibClock);
        BKP_SetRTCCalibrationValue(49);

        RTC_WaitForSynchro();

        //RTC_SetPrescaler(0x7FFF);
        RTC_SetPrescaler(0x7fff);

        RTC_WaitForLastTask();

        // Enable RTC Alarm Interrupt
        RTC_ITConfig(RTC_IT_ALR, ENABLE);
        RTC_WaitForLastTask();
        RTC_SetAlarm(RTC_GetCounter() + 1);
        RTC_WaitForLastTask();

    }
    RTC_ClearFlag(RTC_FLAG_ALR);
    tick();
    
    //GPIO_ResetBits(GPIOA, GPIO_Pin_1);  // turn the LED on
    //delay(40);
    //GPIO_SetBits(GPIOA, GPIO_Pin_1);    // turn the LED off

    PWR_EnterSTANDBYMode();
    while(1);
}
