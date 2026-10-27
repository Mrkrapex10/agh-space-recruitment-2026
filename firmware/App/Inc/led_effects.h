#ifndef LED_EFFECTS_H
#define LED_EFFECTS_H

#include <stdint.h>

void led_effects_init(void);

/* Call when the button is pressed: starts LED_BLINK_COUNT blinks of the blue channel. */
void led_effects_trigger_blink(uint32_t now_ms);

/* Pure function: colour for time `now_ms`. Exposed for unit tests. */
void led_effects_compute(uint32_t now_ms, uint8_t *r, uint8_t *g, uint8_t *b);

/* Call as often as possible from the main loop. */
void led_effects_update(uint32_t now_ms);

#endif /* LED_EFFECTS_H */
