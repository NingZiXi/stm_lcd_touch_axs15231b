/* F407 HAL I2C example; board_init must supply actual wiring. */
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_i2c.h"
#include "stm_lcd_touch_axs15231b.h"
typedef struct { I2C_HandleTypeDef *i2c; GPIO_TypeDef *rst_port; uint16_t rst_pin; } board_t;
static board_t board;
static stm_err_t board_init(void) {
    /* TODO: initialize I2C, GPIO, pull-ups and board.i2c/rst_port/rst_pin.
       Touch INT is optional; poll periodically when not using interrupt. */
    return STM_ERR_INVALID_CONFIG;
}
static stm_err_t result(HAL_StatusTypeDef status) {
    return status==HAL_OK ? STM_OK : status==HAL_TIMEOUT ? STM_ERR_TIMEOUT : STM_ERR_IO;
}
static stm_err_t write_cb(void *io,const uint8_t *data,size_t n) {
    board_t *b=io;
    if (!b || !b->i2c || !data || n!=11u) return STM_ERR_INVALID_ARG;
    return result(HAL_I2C_Master_Transmit(b->i2c,LCD_TOUCH_AXS15231B_ADDR_7BIT<<1,(uint8_t *)data,(uint16_t)n,100));
}
static stm_err_t read_cb(void *io,uint8_t *data,size_t n) {
    board_t *b=io;
    if (!b || !b->i2c || !data || n!=8u) return STM_ERR_INVALID_ARG;
    return result(HAL_I2C_Master_Receive(b->i2c,LCD_TOUCH_AXS15231B_ADDR_7BIT<<1,data,(uint16_t)n,100));
}
static stm_err_t reset(void *io,int high) {
    board_t *b=io;HAL_GPIO_WritePin(b->rst_port,b->rst_pin,high?GPIO_PIN_SET:GPIO_PIN_RESET);return STM_OK;
}
static void delay_ms(void *io,uint32_t ms) {(void)io;HAL_Delay(ms);}
int main(void) {
    HAL_Init();
    if (board_init()!=STM_OK) for (;;) {} /* intentionally require real board implementation */
    lcd_touch_axs15231b_handle_t touch=NULL;
    /* AgentDock's portrait orientation uses raw x in [0,560), raw y in [0,170), mirrored. */
    lcd_touch_axs15231b_config_t cfg={.write=write_cb,.read=read_cb,.reset=reset,.delay_ms=delay_ms,
        .io=&board,.x_max=560,.y_max=170,.mirror_x=1,.mirror_y=1};
    stm_err_t err=lcd_touch_axs15231b_create(&cfg,&touch);
    if (err==STM_OK) err=lcd_touch_axs15231b_reset(touch);
    if (err!=STM_OK) {lcd_touch_axs15231b_delete(&touch);for (;;) {}}
    for (;;) {
        lcd_touch_axs15231b_point_t p;size_t count=0;
        err=lcd_touch_axs15231b_read_data(touch);
        if (err==STM_OK) err=lcd_touch_axs15231b_get_data(touch,&p,1,&count);
        /* TODO: map raw axes to LCD coordinates after calibrating physical orientation. */
        HAL_Delay(20);
    }
}

