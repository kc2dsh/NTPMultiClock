// main.cpp - ESP-IDF LCD Initialization and Test Image Display
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <lvgl.h>
extern "C" {
    extern const lv_font_t lv_font_unscii_16;
    extern const lv_font_t lv_font_mono_32; // Custom 32px monospace font
}

#include <TouchLib.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

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

//TouchLib touch(Wire, TOUCH_SDA, TOUCH_SCL, GT911_SLAVE_ADDRESS2, TOUCH_RST);
TouchLib touch(Wire, TOUCH_SDA, TOUCH_SCL, GT911_SLAVE_ADDRESS1, TOUCH_RST);

#define WIFI_SSID "TP-Link_CB58"
#define WIFI_PASS "13157005"
#define MQTT_BROKER "192.168.100.232"
#define MQTT_TOPIC "esp32s3-1/tele"
#define NTP_SERVER "pool.ntp.org"

// MQTT setup
WiFiClient espClient;
PubSubClient mqttClient(espClient);

lv_obj_t *time_label = nullptr;
lv_obj_t *time_label_local = nullptr;
lv_obj_t *time_label_edt = nullptr;
lv_obj_t *time_label_blr = nullptr;

// Add global LVGL label for the date
static lv_obj_t *date_label = nullptr;

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    // Handle incoming MQTT messages if needed
}

void connectToWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");
}

void connectToMQTT() {
    mqttClient.setServer(MQTT_BROKER, 1883);
    mqttClient.setCallback(mqttCallback);
    while (!mqttClient.connected()) {
        Serial.print("Connecting to MQTT...");
        if (mqttClient.connect("esp32s3-1")) {
            Serial.println("connected");
            mqttClient.subscribe(MQTT_TOPIC);
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 2 seconds");
            delay(2000);
        }
    }
}

void syncTime() {
    configTime(0, 0, NTP_SERVER);
    while (time(nullptr) < 100000) {
        delay(500);
        Serial.print("*");
    }
    Serial.println("\nTime synced");
}

void lvgl_show_time(const char* timestr) {
    if (!time_label) {
        time_label = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(time_label, &lv_font_montserrat_48, 0);
        lv_obj_set_style_text_color(time_label, lv_color_white(), 0);
        lv_obj_align(time_label, LV_ALIGN_CENTER, 0, 0);
    }
    lv_label_set_text(time_label, timestr);
}

// In lvgl_show_times, use a fixed-width font for all time labels
void lvgl_show_times(const char* london, const char* ny, const char* blr, const char* phx, const char* palo, const char* chi, const char* date_str) {
    int y = 10;
    int spacing = 40;
    // Use LVGL's Montserrat 32 font for city labels, and custom 32px monospace font for time strings
    const lv_font_t* city_font = &lv_font_montserrat_32;
    const lv_font_t* time_font = &lv_font_mono_32;
    lv_color_t city_color = lv_palette_main(LV_PALETTE_BLUE);
    lv_color_t time_color = lv_palette_main(LV_PALETTE_YELLOW);
    static lv_obj_t *city_labels[6] = {nullptr};
    static lv_obj_t *time_labels[6] = {nullptr};
    const char* city_names[] = {"London", "New York", "Bangalore", "Phoenix", "Palo Alto", "Chicago"};
    const char* time_strs[] = {london, ny, blr, phx, palo, chi};
    for (int i = 0; i < 6; ++i) {
        if (!city_labels[i]) {
            city_labels[i] = lv_label_create(lv_scr_act());
            lv_obj_set_style_text_font(city_labels[i], city_font, 0);
            lv_obj_set_style_text_color(city_labels[i], city_color, 0);
            lv_obj_align(city_labels[i], LV_ALIGN_TOP_LEFT, 10, y);
        } else {
            lv_obj_set_style_text_font(city_labels[i], city_font, 0);
            lv_obj_set_style_text_color(city_labels[i], city_color, 0);
            lv_obj_align(city_labels[i], LV_ALIGN_TOP_LEFT, 10, y);
        }
        lv_label_set_text(city_labels[i], city_names[i]);
        if (!time_labels[i]) {
            time_labels[i] = lv_label_create(lv_scr_act());
            lv_obj_set_style_text_font(time_labels[i], time_font, 0);
            lv_obj_set_style_text_color(time_labels[i], time_color, 0);
            lv_obj_align(time_labels[i], LV_ALIGN_TOP_LEFT, 200, y);
        } else {
            lv_obj_set_style_text_font(time_labels[i], time_font, 0);
            lv_obj_set_style_text_color(time_labels[i], time_color, 0);
            lv_obj_align(time_labels[i], LV_ALIGN_TOP_LEFT, 200, y);
        }
        // Extract only the time part (HH:MM:SS) from the formatted string
        const char* time_part = time_strs[i];
        size_t city_len = strlen(city_names[i]);
        if (strncmp(time_part, city_names[i], city_len) == 0) {
            time_part += city_len;
            while (*time_part == ' ') time_part++;
        }
        char time_buf[16];
        if (i == 0) {
            // London: keep seconds
            strncpy(time_buf, time_part, sizeof(time_buf) - 1);
            time_buf[sizeof(time_buf) - 1] = '\0';
        } else {
            // Others: remove seconds (show HH:MM only)
            strncpy(time_buf, time_part, 5); // HH:MM
            time_buf[5] = '\0';
        }
        lv_label_set_text(time_labels[i], time_buf);
        y += spacing;
    }
    // Date label at the bottom left, with IP address appended
    char date_ip_str[80];
    // Get local IP address as string
    String ipStr = WiFi.localIP().toString();
    snprintf(date_ip_str, sizeof(date_ip_str), "%s     %s", date_str, ipStr.c_str());
    if (!date_label) {
        date_label = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(date_label, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(date_label, lv_palette_main(LV_PALETTE_CYAN), 0);
        lv_obj_align(date_label, LV_ALIGN_BOTTOM_LEFT, 10, 0); // Align to bottom left with 10px padding
    } else {
        lv_obj_set_style_text_font(date_label, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(date_label, lv_palette_main(LV_PALETTE_CYAN), 0);
        lv_obj_align(date_label, LV_ALIGN_BOTTOM_LEFT, 10, 0);
    }
    lv_label_set_text(date_label, date_ip_str);
}

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

// Global LVGL label for touch coordinates
static lv_obj_t *touch_label = nullptr;

// LVGL touchpad read callback
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    if (touch.read()) {
        auto p = touch.getPoint(0);
        Serial.printf("Touch at: %d, %d\n", p.x, p.y);
        data->state = LV_INDEV_STATE_PR;
        data->point.x = p.x;
        data->point.y = p.y;
        // Update the label with coordinates if it exists
        if (touch_label) {
            // char buf[32];
            // snprintf(buf, sizeof(buf), "Touch: %d, %d", p.x, p.y);
            // lv_label_set_text(touch_label, buf);
            // lv_obj_align(touch_label, LV_ALIGN_TOP_MID, 0, 0);
        }
    } else {
        Serial.println("No touch detected");
        data->state = LV_INDEV_STATE_REL;
        // Optionally clear the label when not touching
        if (touch_label) {
            // lv_label_set_text(touch_label, "Touch: -,-");
        }
    }
}

void lvgl_init() {
    ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
    ledcAttachPin(LCD_BL, LEDC_CHANNEL_0);
    setBrightness(250);
    if (gfx->begin()) {
        gfx->fillScreen(BLACK);
    } else {
        Serial.println("gfx->begin() failed!");
    }
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, SCREEN_WIDTH * SCR_BUF_LEN);
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // Touch init
    // touch.begin(); // No longer needed, begin() is protected or handled by constructor

    // Register LVGL input device driver
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    // Create a label to display touch coordinates (disabled by default)
    touch_label = lv_label_create(lv_scr_act());
    lv_label_set_text(touch_label, ""); // Hide the label by setting empty text
    lv_obj_align(touch_label, LV_ALIGN_TOP_MID, 0, 0);
}

// Helper: Find last Sunday of a month
int last_sunday(int year, int month) {
    struct tm t = {0};
    t.tm_year = year - 1900;
    t.tm_mon = month;
    t.tm_mday = 31;
    t.tm_hour = 1;
    mktime(&t);
    int wday = t.tm_wday;
    t.tm_mday -= wday;
    mktime(&t);
    return t.tm_mday;
}

// Helper: Find Nth Sunday of a month (n >= 1)
int nth_sunday(int year, int month, int n) {
    struct tm t = {0};
    t.tm_year = year - 1900;
    t.tm_mon = month;
    t.tm_mday = 1;
    t.tm_hour = 1;
    mktime(&t);
    int wday = t.tm_wday;
    int first_sunday = (wday == 0) ? 1 : (8 - wday);
    return first_sunday + 7 * (n - 1);
}

// Returns true if DST is in effect for London (last Sunday March to last Sunday October)
bool is_dst_london(const struct tm* t) {
    int year = t->tm_year + 1900;
    int mon = t->tm_mon;
    int mday = t->tm_mday;
    int wday = t->tm_wday;
    if (mon < 2 || mon > 9) return false; // Jan, Feb, Nov, Dec: no DST
    if (mon > 2 && mon < 9) return true;  // Apr-Sep: DST
    if (mon == 2) { // March
        int last = last_sunday(year, 2);
        if (mday > last) return true;
        if (mday < last) return false;
        // On last Sunday: DST starts at 1:00 UTC
        return t->tm_hour >= 1;
    }
    if (mon == 9) { // October
        int last = last_sunday(year, 9);
        if (mday < last) return true;
        if (mday > last) return false;
        // On last Sunday: DST ends at 1:00 UTC
        return t->tm_hour < 1;
    }
    return false;
}

// Returns true if DST is in effect for New York (second Sunday March to first Sunday November)
bool is_dst_ny(const struct tm* t) {
    int year = t->tm_year + 1900;
    int mon = t->tm_mon;
    int mday = t->tm_mday;
    int wday = t->tm_wday;
    if (mon < 2 || mon > 10) return false; // Jan, Feb, Dec: no DST
    if (mon > 2 && mon < 10) return true;  // Apr-Oct: DST
    if (mon == 2) { // March
        int second = nth_sunday(year, 2, 2);
        if (mday > second) return true;
        if (mday < second) return false;
        // On second Sunday: DST starts at 2:00 local time (7:00 UTC)
        return t->tm_hour >= 7;
    }
    if (mon == 10) { // November
        int first = nth_sunday(year, 10, 1);
        if (mday < first) return true;
        if (mday > first) return false;
        // On first Sunday: DST ends at 2:00 local time (6:00 UTC)
        return t->tm_hour < 6;
    }
    return false;
}

// Returns true if DST is in effect for California (same as New York)
bool is_dst_ca(const struct tm* t) {
    return is_dst_ny(t); // Follows US DST rules
}

// Returns true if DST is in effect for Chicago (same as New York)
bool is_dst_chicago(const struct tm* t) {
    return is_dst_ny(t); // Follows US DST rules
}

// Get local time for each city, with DST
void get_city_times(time_t utc, struct tm* london, struct tm* ny, struct tm* blr, struct tm* phx, struct tm* palo, struct tm* chi) {
    gmtime_r(&utc, london);
    gmtime_r(&utc, ny);
    gmtime_r(&utc, blr);
    gmtime_r(&utc, phx);
    gmtime_r(&utc, palo);
    gmtime_r(&utc, chi);
    // London: UTC+0, DST +1
    struct tm t_london = *london;
    if (is_dst_london(&t_london))
        t_london.tm_hour += 1;
    mktime(&t_london);
    *london = t_london;
    // New York: UTC-5, DST -4
    struct tm t_ny = *ny;
    t_ny.tm_hour -= 5;
    mktime(&t_ny);
    if (is_dst_ny(&t_ny)) {
        t_ny.tm_hour += 1; // UTC-4
        mktime(&t_ny);
    }
    *ny = t_ny;
    // Bangalore: UTC+5:30, no DST
    struct tm t_blr = *blr;
    t_blr.tm_hour += 5;
    t_blr.tm_min += 30;
    mktime(&t_blr);
    *blr = t_blr;
    // Phoenix: UTC-7, no DST
    struct tm t_phx = *phx;
    t_phx.tm_hour -= 7;
    mktime(&t_phx);
    *phx = t_phx;
    // Palo Alto: UTC-8, DST -7
    struct tm t_palo = *palo;
    t_palo.tm_hour -= 8;
    mktime(&t_palo);
    if (is_dst_ca(&t_palo)) {
        t_palo.tm_hour += 1; // UTC-7
        mktime(&t_palo);
    }
    *palo = t_palo;
    // Chicago: UTC-6, DST -5
    struct tm t_chi = *chi;
    t_chi.tm_hour -= 6;
    mktime(&t_chi);
    if (is_dst_chicago(&t_chi)) {
        t_chi.tm_hour += 1; // UTC-5
        mktime(&t_chi);
    }
    *chi = t_chi;
}

extern "C" void app_main() {
    // Arduino core setup
    initArduino();
    Serial.begin(115200);
    Wire.begin(TOUCH_SDA, TOUCH_SCL); // Initialize I2C before using TouchLib
    connectToWiFi();
    syncTime();
    connectToMQTT();
    lvgl_init();
    // Set black background
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);
    unsigned long last_ntp_sync = millis();
    const unsigned long ntp_interval = 57UL * 60UL * 1000UL; // 57 minutes in ms
    while (1) {
        static time_t last_time = 0;
        time_t now = time(nullptr);
        if (now != last_time) {
            last_time = now;
            struct tm t_london, t_ny, t_blr, t_phx, t_palo, t_chi;
            get_city_times(now, &t_london, &t_ny, &t_blr, &t_phx, &t_palo, &t_chi);
            // Pad city names to align times
            const char* city_names[] = {"London", "New York", "Bangalore", "Phoenix", "Palo Alto", "Chicago"};
            char buf_london[40], buf_ny[40], buf_blr[40], buf_phx[40], buf_palo[40], buf_chi[40];
            int pad = 11; // Longest city name + 1
            snprintf(buf_london, sizeof(buf_london), "%-*s %02d:%02d:%02d", pad, city_names[0], t_london.tm_hour, t_london.tm_min, t_london.tm_sec);
            snprintf(buf_ny, sizeof(buf_ny),     "%-*s %02d:%02d:%02d", pad, city_names[1], t_ny.tm_hour, t_ny.tm_min, t_ny.tm_sec);
            snprintf(buf_blr, sizeof(buf_blr),   "%-*s %02d:%02d:%02d", pad, city_names[2], t_blr.tm_hour, t_blr.tm_min, t_blr.tm_sec);
            snprintf(buf_phx, sizeof(buf_phx),   "%-*s %02d:%02d:%02d", pad, city_names[3], t_phx.tm_hour, t_phx.tm_min, t_phx.tm_sec);
            snprintf(buf_palo, sizeof(buf_palo), "%-*s %02d:%02d:%02d", pad, city_names[4], t_palo.tm_hour, t_palo.tm_min, t_palo.tm_sec);
            snprintf(buf_chi, sizeof(buf_chi),   "%-*s %02d:%02d:%02d", pad, city_names[5], t_chi.tm_hour, t_chi.tm_min, t_chi.tm_sec);
            // Format date string from London time
            char date_str[40];
            strftime(date_str, sizeof(date_str), "%A, %d %B %Y", &t_london);
            lvgl_show_times(buf_london, buf_ny, buf_blr, buf_phx, buf_palo, buf_chi, date_str);
        }
        // NTP resync every 57 minutes
        if (millis() - last_ntp_sync > ntp_interval) {
            syncTime();
            last_ntp_sync = millis();
        }
        mqttClient.loop();
        lv_timer_handler();
        delay(10);
    }
}
