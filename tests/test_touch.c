#include "stm_lcd_touch_axs15231b.h"
#include <assert.h>
#include <string.h>
static uint8_t frame[8]; static int fail_write,fail_read,reads;
static stm_err_t write_cb(void *io,const uint8_t *p,size_t n) {
    (void)io; static const uint8_t req[11]={0xB5,0xAB,0xA5,0x5A,0,0,0,8,0,0,0};
    assert(n==11 && memcmp(p,req,n)==0); return fail_write ? STM_ERR_TIMEOUT : STM_OK;
}
static stm_err_t read_cb(void *io,uint8_t *p,size_t n) {
    (void)io; assert(n==8); ++reads; if (fail_read) return STM_ERR_IO;
    memcpy(p,frame,n); return STM_OK;
}
int main(void) {
    lcd_touch_axs15231b_handle_t h=NULL;
    lcd_touch_axs15231b_config_t c={.write=write_cb,.read=read_cb,.x_max=560,.y_max=170,
                                     .mirror_x=1,.mirror_y=1};
    assert(lcd_touch_axs15231b_create(&c,&h)==STM_OK);
    assert(lcd_touch_axs15231b_create(&c,&h)==STM_ERR_INVALID_STATE);
    frame[1]=1; frame[2]=0x41;frame[3]=0xF4;frame[4]=0;frame[5]=0x64;
    assert(lcd_touch_axs15231b_read_data(h)==STM_OK);
    lcd_touch_axs15231b_point_t p={0};size_t count=0;
    assert(lcd_touch_axs15231b_get_data(h,&p,1,&count)==STM_OK && count==1 && p.x==59 && p.y==69);
    fail_read=1;assert(lcd_touch_axs15231b_read_data(h)==STM_ERR_IO);
    assert(lcd_touch_axs15231b_get_data(h,&p,1,&count)==STM_OK && count==0);
    fail_read=0;fail_write=1;int before=reads;
    assert(lcd_touch_axs15231b_read_data(h)==STM_ERR_TIMEOUT && reads==before);
    fail_write=0;frame[1]=2;
 assert(lcd_touch_axs15231b_read_data(h)==STM_ERR_VERIFY);
 frame[1]=0;assert(lcd_touch_axs15231b_read_data(h)==STM_OK);
 c.x_max=0; lcd_touch_axs15231b_handle_t empty=NULL;
 assert(lcd_touch_axs15231b_create(&c,&empty)==STM_ERR_INVALID_CONFIG);
 assert(lcd_touch_axs15231b_delete(&h)==STM_OK && !h);
 return 0;
}
