#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>
#include <stdbool.h>

#define PLAYER_MAX_TRACKS  24
#define PLAYER_PATH_MAX    96

typedef enum {
    PLAYER_STOPPED = 0,
    PLAYER_PLAYING,
    PLAYER_PAUSED,
    PLAYER_ERROR
} player_state_t;

int player_init(void);
void player_poll(void);

void player_play(void);
void player_pause(void);
void player_stop(void);
void player_toggle(void);
void player_select(uint8_t index);
void player_play_index(uint8_t index);
void player_set_volume(uint8_t percent);
void player_touch_while_i2s(void);
bool player_spi_busy(void);

player_state_t player_get_state(void);
uint8_t player_get_volume(void);
uint32_t player_get_position(void);
uint32_t player_get_size(void);
int player_get_rate(void);
int player_get_channels(void);
const char *player_get_status_text(void);
const char *player_get_track_path(void);
const char *player_get_track_name(void);
uint8_t player_get_track_count(void);
uint8_t player_get_selected(void);
const char *player_get_track_path_at(uint8_t index);
const char *player_get_track_name_at(uint8_t index);

#endif
