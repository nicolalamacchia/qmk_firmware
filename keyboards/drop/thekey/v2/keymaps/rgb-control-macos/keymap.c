#include QMK_KEYBOARD_H

enum custom_keycodes {
    LAYER_SWITCH = SAFE_RANGE,
};

// RGB Configuration
#define LAYER0_HUE 213
#define LAYER0_SAT 255
#define LAYER0_VAL 64

#define LAYER1_HUE 128
#define LAYER1_SAT 255
#define LAYER1_VAL 64

#define LAYER2_HUE 50
#define LAYER2_SAT 255
#define LAYER2_VAL_MIN 10 // Starting brightness
#define LAYER2_VAL_MAX 64 // Maximum brightness

#define LAYER2_TIMEOUT 1000 // 1 second timeout
#define MIN_BLINK_INTERVAL 20
#define MAX_BLINK_INTERVAL 100

static uint8_t  current_layer      = 0;
static uint32_t layer2_timer       = 0;
static bool     layer2_key_pressed = false;
static bool     layer2_active      = false;
static bool     blink_state        = false;
static uint32_t last_blink_toggle  = 0;

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {[0] = LAYOUT(LAYER_SWITCH, KC_VOLD, KC_VOLU), [1] = LAYOUT(LAYER_SWITCH, KC_MPRV, KC_MNXT), [2] = LAYOUT(LAYER_SWITCH, KC_MPLY, KC_MSTP)};

void update_layer_rgb(uint8_t layer) {
    switch (layer) {
        case 0:
            rgblight_sethsv_noeeprom(LAYER0_HUE, LAYER0_SAT, LAYER0_VAL);
            break;
        case 1:
            rgblight_sethsv_noeeprom(LAYER1_HUE, LAYER1_SAT, LAYER1_VAL);
            break;
        case 2:
            // Base color will be managed by the effect system
            break;
    }
}

void matrix_scan_user(void) {
    if (layer2_active) {
        const uint32_t now     = timer_read32();
        const uint32_t elapsed = now - layer2_timer;

        if (elapsed >= LAYER2_TIMEOUT || layer2_key_pressed) {
            // Timeout reached - solid maximum brightness
            layer2_active = false;
            rgblight_sethsv_noeeprom(LAYER2_HUE, LAYER2_SAT, LAYER2_VAL_MAX);
            return;
        }

        // Calculate current brightness
        uint8_t brightness = LAYER2_VAL_MIN + (LAYER2_VAL_MAX - LAYER2_VAL_MIN) * elapsed / LAYER2_TIMEOUT;
        brightness         = brightness > LAYER2_VAL_MAX ? LAYER2_VAL_MAX : brightness;

        // Calculate dynamic blink interval
        uint16_t interval = MAX_BLINK_INTERVAL - (elapsed * (MAX_BLINK_INTERVAL - MIN_BLINK_INTERVAL) / LAYER2_TIMEOUT);
        interval          = interval < MIN_BLINK_INTERVAL ? MIN_BLINK_INTERVAL : interval;

        // Handle blinking
        if ((now - last_blink_toggle) >= interval) {
            blink_state       = !blink_state;
            last_blink_toggle = now;
        }

        // Update LEDs
        if (blink_state) {
            // rgblight_sethsv_noeeprom(LAYER2_HUE, LAYER2_SAT, brightness);
            rgblight_sethsv_noeeprom(LAYER2_HUE, LAYER2_SAT, LAYER2_VAL_MAX);
        } else {
            // rgblight_sethsv_noeeprom(LAYER1_HUE, LAYER1_SAT, 0);
            rgblight_sethsv_noeeprom(LAYER1_HUE, LAYER1_SAT, LAYER1_VAL);
        }
    }
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (current_layer == 2 && keycode != LAYER_SWITCH) {
        if (record->event.pressed) {
            layer2_key_pressed = true;
        }
        return true;
    }

    switch (keycode) {
        case LAYER_SWITCH:
            if (record->event.pressed) {
                if (current_layer == 1) {
                    // Activate Layer 2 with combined effect
                    layer_on(2);
                    current_layer      = 2;
                    layer2_timer       = timer_read32();
                    layer2_active      = true;
                    layer2_key_pressed = false;
                    blink_state        = true;
                    last_blink_toggle  = layer2_timer;
                    rgblight_sethsv_noeeprom(LAYER2_HUE, LAYER2_SAT, LAYER2_VAL_MIN);
                }
            } else {
                if (current_layer == 2) {
                    layer_off(2);
                    const uint32_t held_duration = timer_elapsed32(layer2_timer);

                    if (!layer2_key_pressed && held_duration < LAYER2_TIMEOUT) {
                        // Short hold: return to Layer 0
                        layer_off(1);
                        current_layer = 0;
                        layer_on(0);
                    } else {
                        // Long hold or keypress: return to Layer 1
                        current_layer = 1;
                        layer_on(1);
                    }
                    layer2_active = false;
                    update_layer_rgb(current_layer);
                } else {
                    // Normal layer toggle
                    layer_off(current_layer);
                    current_layer = (current_layer + 1) % 2;
                    layer_on(current_layer);
                    update_layer_rgb(current_layer);
                }
            }
            return false;
    }
    return true;
}

void keyboard_post_init_user(void) {
    rgblight_enable_noeeprom();
    rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT);
    update_layer_rgb(0);
}
