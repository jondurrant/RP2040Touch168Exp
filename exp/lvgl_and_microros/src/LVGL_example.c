/*****************************************************************************
* | File      	:   LCD_1in28_LVGL_test.c
* | Author      :   Waveshare team
* | Function    :   1.28inch LCD  test demo
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2024-06-27
* | Info        :
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/

#include "LVGL_example.h" 
#include "src/core/lv_obj.h"
#include "src/misc/lv_area.h"

// LVGL
static lv_disp_drv_t disp_drv;
static lv_disp_draw_buf_t disp_buf;
static lv_color_t buf0[DISP_HOR_RES * DISP_VER_RES/2];
static lv_color_t buf1[DISP_HOR_RES * DISP_VER_RES/2];

static lv_obj_t *tv;
static lv_obj_t *tile1;
static lv_obj_t *tile2;
static lv_obj_t *tile3;
static lv_obj_t *tile4;

static lv_obj_t *table_imu_data;
static lv_obj_t *table_rtc_date;
static lv_obj_t *table_rtc_time;
static lv_obj_t *sw;
static lv_obj_t *slider;
static lv_obj_t *roller;

// Touch
static uint16_t ts_x;
static uint16_t ts_y;
static lv_indev_state_t ts_act;
static uint8_t gesture = 0;
static lv_indev_drv_t indev_ts;

// Extern 
extern uint beep_slice_num;
extern uint bl_slice_num;

// Timer 
static struct repeating_timer lvgl_timer;
static struct repeating_timer imu_data_update_timer;
static struct repeating_timer rtc_update_timer;
 
static void disp_flush_cb(lv_disp_drv_t * disp, const lv_area_t * area, lv_color_t * color_p);
static void touch_callback(uint gpio, uint32_t events);
static void ts_read_cb(lv_indev_drv_t * drv, lv_indev_data_t*data);
static void dma_handler(void);
static void scroll_begin_event_cb(lv_event_t * eevent);
static void sw_event_cb(lv_event_t * event);
static void slider_event_cb(lv_event_t * event);
static void roller_event_cb(lv_event_t * event);
static void update_imu_data(void);
static void update_rtc_data(void);
static bool repeating_lvgl_timer_callback(struct repeating_timer *t); 
static bool repeating_imu_data_update_timer_callback(struct repeating_timer *t); 
static bool repeating_rtc_update_timer_callback(struct repeating_timer *t);

/********************************************************************************
function:	Initializes LVGL and enbable timers IRQ and DMA IRQ
parameter:
********************************************************************************/
void LVGL_Init(void)
{
    /*1.Init Timer*/ 
    add_repeating_timer_ms(300,  repeating_rtc_update_timer_callback,      NULL, &rtc_update_timer);
    add_repeating_timer_ms(500,  repeating_imu_data_update_timer_callback, NULL, &imu_data_update_timer);
    add_repeating_timer_ms(5,    repeating_lvgl_timer_callback,            NULL, &lvgl_timer);
    
    /*2.Init LVGL core*/
    lv_init();

    /*3.Init LVGL display*/
    lv_disp_draw_buf_init(&disp_buf, buf0, buf1, DISP_HOR_RES * DISP_VER_RES / 2); 
    lv_disp_drv_init(&disp_drv);    
    disp_drv.flush_cb = disp_flush_cb;
    disp_drv.draw_buf = &disp_buf;        
    disp_drv.hor_res = DISP_HOR_RES;
    disp_drv.ver_res = DISP_VER_RES;
    disp_drv.sw_rotate = 1;
    disp_drv.rotated = LV_DISP_ROT_90;

    lv_disp_t *disp= lv_disp_drv_register(&disp_drv);   

#if INPUTDEV_TS
    /*4.Init touch screen as input device*/ 
    lv_indev_drv_init(&indev_ts); 
    indev_ts.type = LV_INDEV_TYPE_POINTER;    
    indev_ts.read_cb = ts_read_cb;            
    lv_indev_t * ts_indev = lv_indev_drv_register(&indev_ts);
    //Enable touch IRQ
    DEV_IRQ_SET(Touch_INT_PIN, GPIO_IRQ_EDGE_RISE, &touch_callback);
#endif

    /*5.Init DMA for transmit color data from memory to SPI*/
    dma_channel_set_irq0_enabled(dma_tx, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);

}


/********************************************************************************
function:	Initializes the layout of LVGL widgets
parameter:
********************************************************************************/
void Widgets_Init(void)
{
    /*Style Config*/
    static lv_style_t style_slider;
    lv_style_set_bg_color(&style_slider, lv_palette_main(LV_PALETTE_ORANGE));
    lv_style_set_border_color(&style_slider, lv_palette_darken(LV_PALETTE_ORANGE, 3));

    static lv_style_t style_indic;
    lv_style_init(&style_indic);
    lv_style_set_bg_color(&style_indic, lv_palette_lighten(LV_PALETTE_DEEP_ORANGE, 3));
    lv_style_set_bg_grad_color(&style_indic, lv_palette_main(LV_PALETTE_DEEP_ORANGE));
    lv_style_set_bg_grad_dir(&style_indic, LV_GRAD_DIR_HOR);

    static lv_style_t style_indic_pr;
    lv_style_init(&style_indic_pr);
    lv_style_set_shadow_color(&style_indic_pr, lv_palette_main(LV_PALETTE_DEEP_ORANGE));
    lv_style_set_shadow_width(&style_indic_pr, 10);
    lv_style_set_shadow_spread(&style_indic_pr, 3);

    static lv_style_t style_sw;
    lv_style_init(&style_sw);
    lv_style_set_bg_opa(&style_sw, LV_OPA_COVER);
    lv_style_set_bg_color(&style_sw, lv_palette_lighten(LV_PALETTE_RED, 1));
    lv_style_set_shadow_width(&style_sw, 55);
    lv_style_set_shadow_color(&style_sw, lv_palette_main(LV_PALETTE_PINK));

    static lv_style_t style_roller;
    lv_style_init(&style_roller);
    lv_style_set_border_color(&style_roller, lv_palette_darken(LV_PALETTE_BLUE, 3));
    lv_style_set_shadow_width(&style_roller, 55);
    lv_style_set_shadow_color(&style_roller, lv_palette_main(LV_PALETTE_BLUE_GREY));
    
    static lv_style_t style_imu_table;
    lv_style_init(&style_imu_table);
    lv_style_set_border_width(&style_imu_table, 1);
    lv_style_set_text_font(&style_imu_table, &lv_font_montserrat_16);

    static lv_style_t style_rtc_table;
    lv_style_init(&style_rtc_table);
    lv_style_set_border_width(&style_rtc_table, 1);
    lv_style_set_text_font(&style_rtc_table, &lv_font_montserrat_16);

    /*Create tileview*/
    tv = lv_tileview_create(lv_scr_act());
    lv_obj_set_scrollbar_mode(tv,  LV_SCROLLBAR_MODE_OFF);

    /*Tile1: Just a pic*/
    tile1 = lv_tileview_add_tile(tv, 0, 0, LV_DIR_BOTTOM);
    
    static lv_coord_t col_dsc[] = {100, 100, LV_GRID_TEMPLATE_LAST};
	static lv_coord_t row_dsc[] = {100,100, LV_GRID_TEMPLATE_LAST};

	/*Create a container with grid*/
	lv_obj_t * cont = lv_obj_create(tile1);
	lv_obj_set_style_grid_column_dsc_array(cont, col_dsc, 0);
	lv_obj_set_style_grid_row_dsc_array(cont, row_dsc, 0);
	lv_obj_set_size(cont, 300, 220);
	lv_obj_center(cont);
	lv_obj_set_layout(cont, LV_LAYOUT_GRID);


    lv_obj_t * meterJog;
    meterJog = lv_meter_create(cont);
	//lv_obj_center(meterJog);
	//lv_obj_set_size(meterJog, 100, 100);
    lv_obj_set_grid_cell(meterJog,
    		LV_GRID_ALIGN_STRETCH, 0, 1,
			 LV_GRID_ALIGN_STRETCH, 1, 1);
	lv_meter_scale_t * meterJogScale = lv_meter_add_scale(meterJog);
	lv_meter_set_scale_ticks(meterJog, meterJogScale, 4, 2, 10, lv_palette_main(LV_PALETTE_GREY));
	lv_meter_set_scale_major_ticks(meterJog, meterJogScale, 1, 2, 30, lv_color_hex3(0xeee), 15);
	lv_meter_set_scale_range(meterJog, meterJogScale, -10, 10, 270, 90);

	/*Add a three arc indicator*/
	lv_meter_indicator_t * indicJog1 = lv_meter_add_arc(meterJog, meterJogScale, -10, lv_palette_main(LV_PALETTE_RED), 0);
	lv_meter_indicator_t * indicJog2 = lv_meter_add_arc(meterJog, meterJogScale, 10, lv_palette_main(LV_PALETTE_GREEN), -10);

	lv_meter_set_indicator_end_value(meterJog, indicJog1, 3);
	lv_meter_set_indicator_end_value(meterJog, indicJog2, 5);

	lv_obj_t * meterState;
	meterState = lv_meter_create(cont);
	//lv_obj_center(meterState);
	//lv_obj_set_size(meterState, 100, 100);
	lv_obj_set_grid_cell(meterState,
	    		LV_GRID_ALIGN_STRETCH, 1, 1,
				LV_GRID_ALIGN_STRETCH, 1, 1);
	lv_meter_scale_t * meterStateScale = lv_meter_add_scale(meterState);
	lv_meter_set_scale_ticks(meterState, meterStateScale, 4, 2, 10, lv_palette_main(LV_PALETTE_GREY));
	lv_meter_set_scale_major_ticks(meterState, meterStateScale, 1, 2, 30, lv_color_hex3(0xeee), 15);
	lv_meter_set_scale_range(meterState, meterStateScale, -10, 10, 270, 90);

	/*Add a three arc indicator*/
	lv_meter_indicator_t * indicState1 = lv_meter_add_arc(meterState, meterStateScale, -10, lv_palette_main(LV_PALETTE_RED), 0);
	lv_meter_indicator_t * indicState2 = lv_meter_add_arc(meterState, meterStateScale, 10, lv_palette_main(LV_PALETTE_GREEN), -10);

	lv_meter_set_indicator_end_value(meterState, indicState1, 9);
	lv_meter_set_indicator_end_value(meterState, indicState2, 6);



	lv_obj_t * meterX;
	meterX = lv_meter_create(cont);
	lv_obj_set_grid_cell(meterX,
			LV_GRID_ALIGN_STRETCH, 0, 1,
			LV_GRID_ALIGN_STRETCH, 0, 1);

	/*Add a scale first*/
	lv_meter_scale_t * scaleX = lv_meter_add_scale(meterX);
	lv_meter_set_scale_ticks(meterX, scaleX, 4, 2, 10, lv_palette_main(LV_PALETTE_GREY));
	lv_meter_set_scale_major_ticks(meterX, scaleX, 1, 2, 30, lv_color_black(), 10);
	lv_meter_set_scale_range(meterX, scaleX, -10, 10, 270, 90);

	lv_meter_indicator_t * indicX;

	/*Add a blue arc to the start*/
	indicX = lv_meter_add_arc(meterX, scaleX, 3, lv_palette_main(LV_PALETTE_BLUE), 0);
	lv_meter_set_indicator_start_value(meterX, indicX, 0);
	lv_meter_set_indicator_end_value(meterX, indicX, 10);

	lv_meter_set_indicator_end_value(meterX, indicX, 9);

	lv_obj_t * meterZ;
	meterZ = lv_meter_create(cont);
	lv_obj_set_grid_cell(meterZ,
			LV_GRID_ALIGN_STRETCH, 1, 1,
			LV_GRID_ALIGN_STRETCH, 0, 1);

	/*Add a scale first*/
	lv_meter_scale_t * scaleZ = lv_meter_add_scale(meterZ);
	lv_meter_set_scale_ticks(meterZ, scaleZ, 4, 2, 10, lv_palette_main(LV_PALETTE_GREY));
	lv_meter_set_scale_major_ticks(meterZ, scaleZ, 1, 2, 30, lv_color_black(), 10);
	lv_meter_set_scale_range(meterZ, scaleZ, -10, 10, 270, 90);

	lv_meter_indicator_t * indicZ;

	/*Add a blue arc to the start*/
	indicZ = lv_meter_add_arc(meterZ, scaleZ, 3, lv_palette_main(LV_PALETTE_RED), 0);
	lv_meter_set_indicator_start_value(meterZ, indicZ, 0);
	lv_meter_set_indicator_end_value(meterZ, indicZ, 10);

	lv_meter_set_indicator_end_value(meterZ, indicZ, 6);


#if 0
    LV_IMG_DECLARE(pic);
    lv_obj_t *img1 = lv_img_create(tile1);
    lv_img_set_src(img1, &pic);
    lv_obj_align(img1, LV_ALIGN_CENTER, 0, 0);
#endif
 
    /*Tile2: Show IMU data*/
    tile2 = lv_tileview_add_tile(tv, 0, 1, LV_DIR_TOP|LV_DIR_BOTTOM);
    
    lv_obj_t *table = lv_table_create(tile2);
    lv_obj_clear_flag(table, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(table, 85, 265); 
    lv_obj_align(table, LV_ALIGN_LEFT_MID, 15 ,0); 
    lv_obj_clear_flag(table, LV_OBJ_FLAG_SCROLLABLE);
    lv_table_set_cell_value(table, 0, 0, "ACC_X");
    lv_table_set_cell_value(table, 1, 0, "ACC_Y");
    lv_table_set_cell_value(table, 2, 0, "ACC_Z");
    lv_table_set_cell_value(table, 3, 0, "CYRO_X");
    lv_table_set_cell_value(table, 4, 0, "CYRO_Y");
    lv_table_set_cell_value(table, 5, 0, "CYRO_Z");
    lv_obj_set_style_bg_color(table, lv_palette_main(LV_PALETTE_LIGHT_BLUE), LV_PART_ITEMS);
    lv_obj_add_style(table, &style_imu_table, 0); 

    table_imu_data = lv_table_create(tile2);
    lv_obj_clear_flag(table_imu_data, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(table_imu_data, 78, 265); 
    lv_obj_align(table_imu_data, LV_ALIGN_CENTER, 15, 0); 
    lv_obj_clear_flag(table_imu_data, LV_OBJ_FLAG_SCROLLABLE);
    lv_table_set_cell_value(table_imu_data, 0, 0, "0");
    lv_table_set_cell_value(table_imu_data, 1, 0, "0");
    lv_table_set_cell_value(table_imu_data, 2, 0, "0");
    lv_table_set_cell_value(table_imu_data, 3, 0, "0");
    lv_table_set_cell_value(table_imu_data, 4, 0, "0");
    lv_table_set_cell_value(table_imu_data, 5, 0, "0");
    lv_obj_set_style_bg_color(table_imu_data, lv_color_make(250, 144, 181), LV_PART_ITEMS);
    lv_obj_add_style(table_imu_data, &style_imu_table, 0); 

    table = lv_table_create(tile2);
    lv_obj_clear_flag(table, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(table, 51, 265); 
    lv_obj_align(table, LV_ALIGN_RIGHT_MID, -16, 0); 
    lv_obj_clear_flag(table, LV_OBJ_FLAG_SCROLLABLE);
    lv_table_set_cell_value(table, 0, 0, "mg");
    lv_table_set_cell_value(table, 1, 0, "mg");
    lv_table_set_cell_value(table, 2, 0, "mg");
    lv_table_set_cell_value(table, 3, 0, "dps"); 
    lv_table_set_cell_value(table, 4, 0, "dps");
    lv_table_set_cell_value(table, 5, 0, "dps");
    lv_obj_set_style_bg_color(table, lv_palette_main(LV_PALETTE_LIGHT_BLUE), LV_PART_ITEMS);
    lv_obj_add_style(table, &style_imu_table, 0); 

    /*Tile3: Show RTC data*/
    tile3 = lv_tileview_add_tile(tv, 0, 2, LV_DIR_TOP|LV_DIR_BOTTOM);

    table = lv_table_create(tile3);
    lv_obj_clear_flag(table, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(table, 210, 43);
    lv_table_set_col_width(table, 0, 70);
    lv_table_set_col_width(table, 1, 70);
    lv_table_set_col_width(table, 2, 70);
    lv_obj_align(table, LV_ALIGN_CENTER, 0, -63);
    lv_obj_clear_flag(table, LV_OBJ_FLAG_SCROLLABLE);
    lv_table_set_cell_value(table, 0, 0, "Year");
    lv_table_set_cell_value(table, 0, 1, "Mon");
    lv_table_set_cell_value(table, 0, 2, "Day");
    lv_obj_set_style_bg_color(table, lv_palette_main(LV_PALETTE_LIGHT_BLUE), LV_PART_ITEMS);
    lv_obj_add_style(table, &style_rtc_table, 0);

    table_rtc_date = lv_table_create(tile3);
    lv_obj_clear_flag(table_rtc_date, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(table_rtc_date, 210, 43);
    lv_table_set_col_width(table_rtc_date, 0, 70);
    lv_table_set_col_width(table_rtc_date, 1, 70);
    lv_table_set_col_width(table_rtc_date, 2, 70);
    lv_obj_align(table_rtc_date, LV_ALIGN_CENTER, 0, -21);
    lv_obj_clear_flag(table_rtc_date, LV_OBJ_FLAG_SCROLLABLE);
    lv_table_set_cell_value(table_rtc_date, 0, 0, "0");
    lv_table_set_cell_value(table_rtc_date, 0, 1, "0");
    lv_table_set_cell_value(table_rtc_date, 0, 2, "0");
    lv_obj_set_style_bg_color(table_rtc_date, lv_color_make(250, 144, 181), LV_PART_ITEMS);
    lv_obj_add_style(table_rtc_date, &style_rtc_table, 0);

    table = lv_table_create(tile3);
    lv_obj_clear_flag(table, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(table, 210, 43);
    lv_table_set_col_width(table, 0, 70);
    lv_table_set_col_width(table, 1, 70);
    lv_table_set_col_width(table, 2, 70);
    lv_obj_align(table, LV_ALIGN_CENTER, 0, 21);
    lv_obj_clear_flag(table, LV_OBJ_FLAG_SCROLLABLE);
    lv_table_set_cell_value(table, 0, 0, "Hour");
    lv_table_set_cell_value(table, 0, 1, "Min");
    lv_table_set_cell_value(table, 0, 2, "Sec");
    lv_obj_set_style_bg_color(table, lv_palette_main(LV_PALETTE_LIGHT_BLUE), LV_PART_ITEMS);
    lv_obj_add_style(table, &style_rtc_table, 0);

    table_rtc_time = lv_table_create(tile3);
    lv_obj_clear_flag(table_rtc_time, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(table_rtc_time, 210, 43);
    lv_table_set_col_width(table_rtc_time, 0, 70);
    lv_table_set_col_width(table_rtc_time, 1, 70);
    lv_table_set_col_width(table_rtc_time, 2, 70);
    lv_obj_align(table_rtc_time, LV_ALIGN_CENTER, 0, 63);
    lv_obj_clear_flag(table_rtc_time, LV_OBJ_FLAG_SCROLLABLE);
    lv_table_set_cell_value(table_rtc_time, 0, 0, "0");
    lv_table_set_cell_value(table_rtc_time, 0, 1, "0");
    lv_table_set_cell_value(table_rtc_time, 0, 2, "0");
    lv_obj_set_style_bg_color(table_rtc_time, lv_color_make(250, 144, 181), LV_PART_ITEMS);
    lv_obj_add_style(table_rtc_time, &style_rtc_table, 0);

    /*Tile4: Some widgets*/
    tile4 = lv_tileview_add_tile(tv, 0, 3, LV_DIR_TOP);

    lv_obj_t *tabview = lv_tabview_create(tile4, LV_DIR_TOP, 50);
    lv_obj_t *tab1 = lv_tabview_add_tab(tabview, "BEEP");
    lv_obj_t *tab2 = lv_tabview_add_tab(tabview, "LCD BL");
    lv_obj_add_event_cb(lv_tabview_get_content(tabview), scroll_begin_event_cb, LV_EVENT_SCROLL_BEGIN, NULL);

    sw = lv_switch_create(tab1);
    lv_obj_set_size(sw, 70, 40);                          
    lv_obj_align(sw, LV_ALIGN_CENTER, -45, 0);
    lv_obj_add_style(sw,&style_sw,LV_STATE_CHECKED|LV_PART_INDICATOR);
    lv_obj_add_event_cb(sw, sw_event_cb,LV_EVENT_VALUE_CHANGED,NULL);

    slider = lv_slider_create(tab1);
    lv_obj_set_size(slider, 10, 100);
    lv_obj_align(slider, LV_ALIGN_CENTER, 45, 0);
    lv_obj_add_style(slider,&style_slider,0);
    lv_obj_add_style(slider,&style_indic,LV_PART_INDICATOR);
    lv_obj_add_style(slider, &style_indic_pr, LV_PART_INDICATOR | LV_STATE_PRESSED);
    lv_obj_add_style(slider,&style_slider,LV_PART_KNOB);
    lv_obj_add_event_cb(slider, slider_event_cb,LV_EVENT_VALUE_CHANGED,NULL);

    roller = lv_roller_create(tab2);
    const char * opts = "1\n2\n3\n4\n5\n6\n7\n8\n9\n10";
    lv_roller_set_options(roller, opts, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(roller, 3);
    lv_obj_align(roller, LV_ALIGN_CENTER, 0, 0);
    lv_roller_set_selected(roller, 5, LV_ANIM_OFF);
    lv_obj_add_style(roller, &style_roller,0);
    lv_obj_add_event_cb(roller, roller_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /*Initialize Display*/
    update_imu_data(); 
    update_rtc_data(); 
}


/********************************************************************************
function:	Disable scroll animations when a tab button is clicked in a tabview
parameter:
********************************************************************************/
static void scroll_begin_event_cb(lv_event_t * event)
{
    lv_anim_t * a = lv_event_get_param(event);
    if (a) a->time = 0; 
}

/********************************************************************************
function:	Refresh image by transferring the color data to the SPI bus by DMA
parameter:
********************************************************************************/
static void disp_flush_cb(lv_disp_drv_t * disp, const lv_area_t * area, lv_color_t * color_p)
{
    LCD_1IN69_SetWindows(area->x1, area->y1, area->x2+1 , area->y2+1);  // Set the LVGL interface display position
    DEV_Digital_Write(LCD_DC_PIN, 1);
    DEV_Digital_Write(LCD_CS_PIN, 0);
    dma_channel_configure(dma_tx,   
                          &c,
                          &spi_get_hw(LCD_SPI_PORT)->dr, 
                          color_p, // read address
                          ((area->x2 + 1 - area-> x1)*(area->y2 + 1 - area -> y1))*2,
                          true);// Start DMA transfer
}

/********************************************************************************
function:   Touch interrupt handler
parameter:
********************************************************************************/
static void touch_callback(uint gpio, uint32_t events)
{
    if (gpio == Touch_INT_PIN)
    {
        CST816S_Get_Point(); // Get coordinate data
        gesture = CST816S_Get_Gesture(); // Get gesture data
        ts_x = Touch_CTS816.x_point;
        ts_y = Touch_CTS816.y_point;
        ts_act = LV_INDEV_STATE_PRESSED;
    }
}

/********************************************************************************
function:   Update touch screen input device status
parameter:
********************************************************************************/
static void ts_read_cb(lv_indev_drv_t * drv, lv_indev_data_t*data)
{
    data->point.x = ts_x;
    data->point.y = ts_y; 
    data->state = ts_act;
    ts_act = LV_INDEV_STATE_RELEASED;
}

/********************************************************************************
function:   Indicate ready with the flushing when DMA complete transmission
parameter:
********************************************************************************/
static void dma_handler(void)
{
    if (dma_channel_get_irq0_status(dma_tx)) 
    {
        dma_channel_acknowledge_irq0(dma_tx);
        DEV_Digital_Write(LCD_CS_PIN, 1);
        lv_disp_flush_ready(&disp_drv); // Indicate you are ready with the flushing
    }
}

/********************************************************************************
function:   Check if the page needs to be updated
parameter:
********************************************************************************/
static bool update_check(lv_obj_t *tv,lv_obj_t *tilex) 
{
    uint8_t ret = true; 

    lv_obj_t *active_tile = lv_tileview_get_tile_act(tv); // Get the current active interface
    if (active_tile != tilex) 
    {
        ret = false;
    }

    if(gesture != CST816S_Gesture_None && gesture != CST816S_Gesture_Click) // The current gesture is not empty and is not a click
    {
        gesture = CST816S_Gesture_None;
        ret = false;
    }

    return ret;
}

/********************************************************************************
function:   Update IMU data
parameter:
********************************************************************************/
static void update_imu_data()
{
    float acc[3], gyro[3];
    unsigned int tim_count = 0;
    QMI8658_read_xyz(acc, gyro, &tim_count); // Reading IMU data

    char table_text[64];
    for(int i = 0; i < 3; i++)
    {
        sprintf(table_text,"%4.1f",acc[i]);
        lv_table_set_cell_value(table_imu_data, i, 0, table_text); // Update table data
    }

    for(int i = 0; i < 3; i++)
    {
        sprintf(table_text,"%4.1f",gyro[i]);
        lv_table_set_cell_value(table_imu_data, i+3, 0, table_text);
    }
}

/********************************************************************************
function:   Update RTC data
parameter:
********************************************************************************/
static void update_rtc_data()
{
    datetime_t Now_time;
    PCF85063A_Read_now(&Now_time); //Reading RTC dat1a

    char table_text[64];
    sprintf(table_text,"%d",Now_time.year);
    lv_table_set_cell_value(table_rtc_date, 0, 0, table_text); // Update table data
    sprintf(table_text,"%02d",Now_time.month);
    lv_table_set_cell_value(table_rtc_date, 0, 1, table_text);
    sprintf(table_text,"%02d",Now_time.day);
    lv_table_set_cell_value(table_rtc_date, 0, 2, table_text);
    sprintf(table_text,"%02d",Now_time.hour);
    lv_table_set_cell_value(table_rtc_time, 0, 0, table_text);
    sprintf(table_text,"%02d",Now_time.min);
    lv_table_set_cell_value(table_rtc_time, 0, 1, table_text);
    sprintf(table_text,"%02d",Now_time.sec);
    lv_table_set_cell_value(table_rtc_time, 0, 2, table_text);
}

/********************************************************************************
function:   Report the elapsed time to LVGL each 5ms
parameter:
********************************************************************************/
static bool repeating_lvgl_timer_callback(struct repeating_timer *t) 
{
    lv_tick_inc(5);
    return true;
}

/********************************************************************************
function:   Update IMU label data each 500ms
parameter:
********************************************************************************/
static bool repeating_imu_data_update_timer_callback(struct repeating_timer *t) 
{
    if(update_check(tv,tile2) == true) // Need to update the interface
        update_imu_data(); // Update data

    return true;
}

/**********`**********************************************************************
function:   Update RTC label data each 300ms
parameter:
********************************************************************************/
static bool repeating_rtc_update_timer_callback(struct repeating_timer *t)
{
    if(update_check(tv,tile3) == true) // Need to update the interface
        update_rtc_data(); // Update data

    return true;
}

/********************************************************************************
function:   Change Beep statue
parameter:
********************************************************************************/
static void sw_event_cb(lv_event_t * event)
{
    if(lv_obj_has_state(sw,LV_STATE_CHECKED)) 
    {
        pwm_set_enabled(beep_slice_num, true); // Buzzer sounds
    } 
    else 
    {
        pwm_set_enabled(beep_slice_num, false); // The buzzer does not sound
    }
}

/********************************************************************************
function:   Change Beep frequency
parameter:
********************************************************************************/
static void slider_event_cb(lv_event_t * event)
{
    int value = lv_slider_get_value(slider); // Get the value of the slider
    pwm_set_wrap(beep_slice_num, 2000-10*value); // Modify buzzer frequency
    pwm_set_chan_level(beep_slice_num, PWM_CHAN_A, (2000-10*value)/2); // Set the duty cycle to 50%
}

/********************************************************************************
function:   Change LCD brightness
parameter:
********************************************************************************/
static void roller_event_cb(lv_event_t * event) 
{
    lv_obj_t * obj = lv_event_get_target(event); // Get the object that triggered the event
    int selected_index = lv_roller_get_selected(obj); // Get the index of the currently selected item of the scroll bar
    pwm_set_chan_level(bl_slice_num, PWM_CHAN_B, (selected_index+1)*10); // Set the output level of the PWM channel according to the selected item index
}
