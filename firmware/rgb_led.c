#include "ch32fun.h"
#include "rgb_led.h"

#ifdef CUSTOM_RGB_ENABLE

#define WS2812DMA_IMPLEMENTATION
#define WSGRB
#define DMALEDS 32

#include "../ch32v003fun/extralibs/ws2812b_dma_spi_led_driver.h"

// アニメーション状態管理
static uint32_t current_time_ms = 0;
static uint32_t last_anim_time = 0;
static uint8_t reactive_intensity = 0;

// シンプルな HSV から RGB への変換
static uint32_t hsv_to_grb(uint8_t h, uint8_t s, uint8_t v) {
    uint8_t r = 0, g = 0, b = 0;
    
    if (s == 0) {
        r = g = b = v;
    } else {
        uint8_t region = h / 43;
        uint8_t remainder = (h - (region * 43)) * 6;
        
        uint8_t p = (v * (255 - s)) >> 8;
        uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
        uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;
        
        switch (region) {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            default: r = v; g = p; b = q; break;
        }
    }
    
    // WSGRB mode expects 0xGGRRBB natively 
    // Wait, the regular macro outputs bits 20..16..12..
    // If we define WSGRB, it extracts (val>>12)(val>>8) (val>>4)(val>>0) (val>>20)(val>>16).
    // So if val = 0xRRGGBB, WSGRB -> RR, GB. This means it outputs Green then Red then Blue!
    // But let's supply standard 0xRRGGBB to WSGRB to get it outputting GRB sequence to LED.
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | ((uint32_t)b);
}

// ユーザーが実装すべきコールバック
uint32_t WS2812BLEDCallback( int ledno ) {
    if (ledno >= CUSTOM_RGB_NUM_LEDS) return 0;

    uint32_t color = 0;
    uint8_t mode = CUSTOM_RGB_MODE;

    if (mode == 0) { // Rainbow
        uint8_t hue = (current_time_ms / 10 + (ledno * 255 / CUSTOM_RGB_NUM_LEDS)) % 256;
        color = hsv_to_grb(hue, 255, 60); // やや輝度を落とす
    } 
    else if (mode == 1) { // Static
        color = hsv_to_grb(128, 255, 60); // 固定のシアン
    } 
    else if (mode == 2) { // Breathing
        // 2秒周期 (2000ms)
        uint16_t cycle = current_time_ms % 2000;
        uint8_t val;
        if (cycle < 1000) {
            val = (cycle * 100) / 1000; // 0 to 100
        } else {
            val = ((2000 - cycle) * 100) / 1000; // 100 to 0
        }
        color = hsv_to_grb(200, 255, val);
    } 
    else if (mode == 3) { // Reactive
        color = hsv_to_grb(20, 255, reactive_intensity);
    }

    return color;
}

void rgb_led_init(void) {
    // init
    WS2812BDMAInit();
}

void rgb_led_task(uint32_t system_millis) {
    current_time_ms = system_millis;
    
    // Dim reactive intensity
    if (reactive_intensity > 0) {
        if (system_millis % 5 == 0) {
            reactive_intensity--;
        }
    }

    // 更新レート: ~33Hz (30ms毎)
    if (system_millis - last_anim_time > 30) {
        last_anim_time = system_millis;
        if (WS2812BLEDInUse == 0) {
            WS2812BDMAStart(CUSTOM_RGB_NUM_LEDS);
        }
    }
}

void rgb_led_notify_keypress(uint8_t row, uint8_t col) {
    // 打鍵時に輝度をMAXにする
    reactive_intensity = 150;
}

#endif
