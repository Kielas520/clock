#include "stm32f10x.h"                
#include "Delay.h"
#include "global.h"
#include "Key.h"

void Key_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_1 | GPIO_Pin_0;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void Key_GetNum(uint8_t *key_states)
{
    // 初始化key_states
    key_states[0] = 0; // PB13 (set_mode)
    key_states[1] = 0; // PB15 (trigger_up)
    key_states[2] = 0; // PA9 (trigger_down)
    key_states[3] = 0; // PA11 (confirm)

    // 定义按键引脚和端口
    GPIO_TypeDef* ports[4] = {GPIOB, GPIOB, GPIOA, GPIOA};
    uint16_t pins[4] = {GPIO_Pin_10 , GPIO_Pin_11 , GPIO_Pin_1 , GPIO_Pin_0};

    // 逐个检测按键
    for (int i = 0; i < 4; i++) {
        if (GPIO_ReadInputDataBit(ports[i], pins[i]) == 0) { // 按下为0
            Delay_ms(10); // 消抖
            if (GPIO_ReadInputDataBit(ports[i], pins[i]) == 0) {
                while (GPIO_ReadInputDataBit(ports[i], pins[i]) == 0); 
                Delay_ms(10); // 消抖
                key_states[i] = 1;
                // 更新全局变量
                if (i == 0) smart_clock.key_function.set_mode = !smart_clock.key_function.set_mode; // PB13
                else if (i == 1) smart_clock.key_function.trigger_down = 1;  // PB15
                else if (i == 2) smart_clock.key_function.trigger_up = 1; // PA9
                else if (i == 3) smart_clock.key_function.confirm = 1;     // PA11
            }
        }
    }
}
