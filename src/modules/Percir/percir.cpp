#include <Arduino.h>
#include "driver/rmt.h"
// We removed the hal/hal.h line!

#define RMT_TX_CHANNEL RMT_CHANNEL_0
#define RMT_CLK_DIV 80 
#define PULSE_HIGH_US 2
#define ZERO_LOW_US   4
#define ONE_LOW_US    8

void blast_payload(const uint8_t* payload, size_t len) {
    // Hardcode the T-Embed IR pin directly to bypass the include error
    gpio_num_t ir_pin = GPIO_NUM_4;

    // Setup the RMT hardware
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(ir_pin, RMT_TX_CHANNEL);
    config.tx_config.carrier_en = false; 
    config.clk_div = RMT_CLK_DIV;
    
    ESP_ERROR_CHECK_WITHOUT_ABORT(rmt_config(&config));
    ESP_ERROR_CHECK_WITHOUT_ABORT(rmt_driver_install(config.channel, 0, 0));

    // Prepare the data
    size_t num_items = len * 8;
    rmt_item32_t* items = (rmt_item32_t*) malloc(num_items * sizeof(rmt_item32_t));
    if (!items) return;

    size_t item_idx = 0;
    for (size_t i = 0; i < len; i++) {
        uint8_t current_byte = payload[i];
        for (int bit = 7; bit >= 0; bit--) {
            bool is_one = (current_byte & (1 << bit)) != 0;
            items[item_idx].duration0 = PULSE_HIGH_US; 
            items[item_idx].level0 = 1; 
            items[item_idx].duration1 = is_one ? ONE_LOW_US : ZERO_LOW_US; 
            items[item_idx].level1 = 0;
            item_idx++;
        }
    }

    // Fire the laser!
    rmt_write_items(RMT_TX_CHANNEL, items, num_items, true);
    rmt_wait_tx_done(RMT_TX_CHANNEL, portMAX_DELAY);
    
    // Clean up
    free(items);
    rmt_driver_uninstall(RMT_TX_CHANNEL);
}
