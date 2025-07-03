// main.cpp - ESP-IDF LCD Initialization and Test Image Display
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <lvgl.h>
#include <TouchLib.h>

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272
#define LCD_BL 1
#define LEDC_CHANNEL_0 0
#define LEDC_TIMER_12_BIT 12
#define LEDC_BASE_FREQ 5000

// QSPI pins for JC4827W543_4.3inch_ESP32S3_board
Arduino_DataBus *bus = new Arduino_ESP32QSPI(
    45 /* cs */, 47 /* sck */, 21 /* d0 */, 48 /* d1 */, 40 /* d2 */, 39 /* d3 */);
Arduino_NV3041A *panel = new Arduino_NV3041A(bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */, true /* IPS */);
Arduino_GFX *gfx = new Arduino_Canvas(SCREEN_WIDTH, SCREEN_HEIGHT, panel);

#define SCR_BUF_LEN 32
static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t disp_drv;
static lv_color_t disp_draw_buf[SCREEN_WIDTH * SCR_BUF_LEN];

// Touch GT911 I2C pins for JC4827W543_4.3inch_ESP32S3_board
#define TOUCH_SCL 4
#define TOUCH_SDA 8
#define TOUCH_INT 3
#define TOUCH_RST 38

TouchLib touch(TOUCH_SCL, TOUCH_SDA, TOUCH_INT, TOUCH_RST, SCREEN_WIDTH, SCREEN_HEIGHT);

void IRAM_ATTR my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    panel->startWrite();
    panel->setAddrWindow(area->x1, area->y1, w, h);
    panel->writePixels((uint16_t *)&color_p->full, w * h);
    panel->endWrite();
    lv_disp_flush_ready(disp);
}

void setBrightness(uint8_t value) {
    uint32_t duty = 4095 * value / 255;
    ledcWrite(LEDC_CHANNEL_0, duty);
}

// LVGL touchpad read callback
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    if (touch.read()) {
        if (touch.isTouched()) {
            data->state = LV_INDEV_STATE_PR;
            data->point.x = touch.getX();
            data->point.y = touch.getY();
        } else {
            data->state = LV_INDEV_STATE_REL;
        }
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void lvgl_init() {
    ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
    ledcAttachPin(LCD_BL, LEDC_CHANNEL_0);
    setBrightness(250);
    if (!gfx->begin()) {
        Serial.println("gfx->begin() failed!");
    }
    gfx->fillScreen(BLACK);
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, SCREEN_WIDTH * SCR_BUF_LEN);
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // Touch init
    touch.begin();

    // Register LVGL input device driver
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);
}

extern "C" void app_main() {
    // Arduino core setup
    initArduino();
    Serial.begin(115200);
    lvgl_init();
    // Show a test LVGL widget
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello, JC4827W543 LCD with LVGL!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    while (1) {
        lv_timer_handler();
        delay(5);
    }
}
