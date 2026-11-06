#include "player.h"
#include "audio_cs43l22.h"
#include "spi3_bus.h"
#include "sd_spi.h"
#include "ff.h"
#include "mp3dec.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MP3_BUF_SIZE     (36U * 1024U)

static FATFS s_fs;
static FIL s_file;
static HMP3Decoder s_dec;
static player_state_t s_state = PLAYER_ERROR;
static uint8_t s_volume = 70;
static uint32_t s_file_size;
static uint32_t s_file_pos;
static int s_rate;
static int s_chans;
static char s_status[48];
static bool s_mounted;
static bool s_file_open;
static bool s_eof;
static volatile bool s_spi_busy;
static char s_tracks[PLAYER_MAX_TRACKS][PLAYER_PATH_MAX];
static uint8_t s_track_count;
static uint8_t s_selected;

static uint8_t s_mp3[MP3_BUF_SIZE];
static int s_bytes_left;
static uint8_t *s_read_ptr;

static int16_t s_pcm[AUDIO_PCM_HALVES][AUDIO_PCM_FRAME_SAMPLES];
static int16_t s_frame[AUDIO_PCM_FRAME_SAMPLES];

static void set_status(const char *msg)
{
    strncpy(s_status, msg, sizeof(s_status) - 1U);
    s_status[sizeof(s_status) - 1U] = '\0';
}

static void skip_id3(void)
{
    uint8_t h[10];
    UINT br = 0;

    if (f_read(&s_file, h, 10, &br) != FR_OK || br != 10U) {
        (void)f_lseek(&s_file, 0);
        return;
    }
    if (h[0] == 'I' && h[1] == 'D' && h[2] == '3') {
        uint32_t size = ((uint32_t)(h[6] & 0x7FU) << 21) |
                        ((uint32_t)(h[7] & 0x7FU) << 14) |
                        ((uint32_t)(h[8] & 0x7FU) << 7) |
                        (uint32_t)(h[9] & 0x7FU);
        (void)f_lseek(&s_file, 10U + size);
    } else {
        (void)f_lseek(&s_file, 0);
    }
}

static int refill_mp3(void)
{
    UINT br = 0;
    FRESULT fr;

    if (s_bytes_left > 0 && s_read_ptr != s_mp3) {
        memmove(s_mp3, s_read_ptr, (size_t)s_bytes_left);
    }
    s_read_ptr = s_mp3;

    if (s_eof) {
        return s_bytes_left;
    }

    fr = f_read(&s_file, s_mp3 + s_bytes_left,
                MP3_BUF_SIZE - (UINT)s_bytes_left, &br);
    if (fr != FR_OK) {
        set_status("SD read error");
        s_state = PLAYER_ERROR;
        return -1;
    }
    s_bytes_left += (int)br;
    s_file_pos = (uint32_t)f_tell(&s_file);
    if (br == 0U) {
        s_eof = true;
    }
    return s_bytes_left;
}

static int decode_one_frame(int16_t *dst)
{
    int filled = 0;

    memset(dst, 0, AUDIO_PCM_FRAME_SAMPLES * sizeof(int16_t));

    while (filled < (int)AUDIO_PCM_FRAME_SAMPLES) {
        int err;
        int offset;
        int n;
        int copy;
        MP3FrameInfo info;

        if (s_bytes_left < 16) {
            break;
        }

        offset = MP3FindSyncWord(s_read_ptr, s_bytes_left);
        if (offset < 0) {
            s_bytes_left = 0;
            s_read_ptr = s_mp3;
            break;
        }
        s_read_ptr += offset;
        s_bytes_left -= offset;

        err = MP3Decode(s_dec, &s_read_ptr, &s_bytes_left, s_frame, 0);
        if (err == ERR_MP3_INDATA_UNDERFLOW || err == ERR_MP3_MAINDATA_UNDERFLOW) {
            break;
        }
        if (err != ERR_MP3_NONE) {
            if (s_bytes_left > 1) {
                s_read_ptr++;
                s_bytes_left--;
            }
            continue;
        }

        MP3GetLastFrameInfo(s_dec, &info);
        s_rate = info.samprate;
        s_chans = info.nChans;
        n = (info.nChans >= 2) ? info.outputSamps : (info.outputSamps * 2);
        if (n > (int)AUDIO_PCM_FRAME_SAMPLES) {
            n = (int)AUDIO_PCM_FRAME_SAMPLES;
        }

        copy = (int)AUDIO_PCM_FRAME_SAMPLES - filled;
        if (copy > n) {
            copy = n;
        }

        if (info.nChans >= 2) {
            memcpy(dst + filled, s_frame, (size_t)copy * sizeof(int16_t));
        } else {
            int i;
            int mono = copy / 2;
            for (i = 0; i < mono; i++) {
                dst[filled + 2 * i] = s_frame[i];
                dst[filled + 2 * i + 1] = s_frame[i];
            }
        }
        filled += copy;
        if (filled >= (int)AUDIO_PCM_FRAME_SAMPLES) {
            break;
        }
    }

    return filled > 0;
}

static void close_file(void)
{
    if (s_file_open) {
        (void)f_close(&s_file);
        s_file_open = false;
    }
}

static int is_mp3_name(const char *name)
{
    size_t n;

    if (name == NULL) {
        return 0;
    }
    n = strlen(name);
    if (n < 4U) {
        return 0;
    }
    return (name[n - 4U] == '.' &&
            tolower((unsigned char)name[n - 3U]) == 'm' &&
            tolower((unsigned char)name[n - 2U]) == 'p' &&
            name[n - 1U] == '3');
}

static const char *filename_of(const char *path)
{
    const char *slash = strrchr(path, '/');

    return (slash != NULL) ? (slash + 1) : path;
}

static void add_track(const char *dir, const char *name)
{
    if (s_track_count >= PLAYER_MAX_TRACKS || name == NULL || name[0] == '\0') {
        return;
    }
    if (dir != NULL && dir[0] != '\0') {
        (void)snprintf(s_tracks[s_track_count], PLAYER_PATH_MAX, "%s/%s", dir, name);
    } else {
        (void)snprintf(s_tracks[s_track_count], PLAYER_PATH_MAX, "%s", name);
    }
    s_track_count++;
}

static void scan_dir(const char *dir)
{
    DIR d;
    FILINFO fno;
    FRESULT fr;

    fr = f_opendir(&d, (dir != NULL && dir[0] != '\0') ? dir : "/");
    if (fr != FR_OK) {
        return;
    }
    for (;;) {
        fr = f_readdir(&d, &fno);
        if (fr != FR_OK || fno.fname[0] == '\0') {
            break;
        }
        if ((fno.fattrib & AM_DIR) != 0U) {
            continue;
        }
        if (is_mp3_name(fno.fname)) {
            add_track(dir, fno.fname);
        }
    }
    (void)f_closedir(&d);
}

static void scan_tracks(void)
{
    s_track_count = 0;
    s_selected = 0;
    scan_dir("music");
    if (s_track_count == 0U) {
        scan_dir("");
    }
}

static int open_track(void)
{
    close_file();
    if (s_track_count == 0U || s_selected >= s_track_count) {
        set_status("No MP3 files found");
        s_state = PLAYER_ERROR;
        return -1;
    }
    if (f_open(&s_file, s_tracks[s_selected], FA_READ) != FR_OK) {
        set_status("Open MP3 failed");
        s_state = PLAYER_ERROR;
        return -1;
    }
    s_file_open = true;
    s_file_size = (uint32_t)f_size(&s_file);
    s_file_pos = 0;
    s_eof = false;
    s_bytes_left = 0;
    s_read_ptr = s_mp3;
    skip_id3();
    if (refill_mp3() < 0) {
        close_file();
        return -1;
    }
    return 0;
}

int player_init(void)
{
    set_status("No SD card");
    s_state = PLAYER_ERROR;
    s_dec = MP3InitDecoder();
    if (s_dec == 0) {
        set_status("MP3 decoder failed");
        return -1;
    }

    spi3_bus_init_spi();
    if (f_mount(&s_fs, "", 1) != FR_OK) {
        set_status("SD mount failed");
        return -1;
    }
    s_mounted = true;

    scan_tracks();
    if (s_track_count == 0U) {
        set_status("No MP3 in music/ or /");
        s_state = PLAYER_ERROR;
        return -1;
    }

    s_state = PLAYER_STOPPED;
    set_status("Ready");
    return 0;
}

static void stop_audio_keep_file(bool close)
{
    audio_codec_mute(true);
    audio_i2s_stop();
    spi3_bus_enter_spi();
    s_spi_busy = false;
    if (close) {
        close_file();
        s_state = PLAYER_STOPPED;
        set_status("Stopped");
    }
}

static int start_i2s_stream(void)
{
    uint32_t hz = (s_rate > 0) ? (uint32_t)s_rate : 44100U;

    sd_spi_cs_release_all();
    spi3_bus_enter_i2s(hz);
    if (audio_i2s_start(&s_pcm[0][0], AUDIO_PCM_FRAME_SAMPLES * AUDIO_PCM_HALVES, hz) != 0) {
        spi3_bus_enter_spi();
        set_status("I2S start failed");
        s_state = PLAYER_ERROR;
        return -1;
    }
    audio_codec_mute(false);
    return 0;
}

static int refill_from_sd(void)
{
    int rc;

    s_spi_busy = true;
    audio_codec_mute(true);
    audio_i2s_stop();
    spi3_bus_enter_spi();
    rc = refill_mp3();
    if (rc < 0) {
        s_spi_busy = false;
        close_file();
        return -1;
    }
    (void)decode_one_frame(s_pcm[0]);
    (void)decode_one_frame(s_pcm[1]);
    s_spi_busy = false;
    if (s_state != PLAYER_PLAYING) {
        return 0;
    }
    return start_i2s_stream();
}

void player_play(void)
{
    if (!s_mounted) {
        return;
    }
    if (s_state == PLAYER_PLAYING) {
        return;
    }

    s_spi_busy = true;
    spi3_bus_enter_spi();
    if (s_state != PLAYER_PAUSED || !s_file_open) {
        if (open_track() != 0) {
            s_spi_busy = false;
            return;
        }
        (void)decode_one_frame(s_pcm[0]);
        (void)decode_one_frame(s_pcm[1]);
    }
    s_spi_busy = false;

    if (start_i2s_stream() != 0) {
        return;
    }
    s_state = PLAYER_PLAYING;
    set_status("Playing");
}

void player_pause(void)
{
    if (s_state != PLAYER_PLAYING) {
        return;
    }
    audio_codec_pause();
    audio_i2s_stop();
    spi3_bus_enter_spi();
    s_state = PLAYER_PAUSED;
    set_status("Paused");
}

void player_stop(void)
{
    stop_audio_keep_file(true);
}

void player_toggle(void)
{
    if (s_state == PLAYER_PLAYING) {
        player_pause();
    } else {
        player_play();
    }
}

void player_select(uint8_t index)
{
    if (index >= s_track_count) {
        return;
    }
    if (s_state == PLAYER_PLAYING || s_state == PLAYER_PAUSED) {
        player_stop();
    }
    s_selected = index;
}

void player_play_index(uint8_t index)
{
    player_select(index);
    player_play();
}

void player_set_volume(uint8_t percent)
{
    s_volume = percent;
    audio_codec_set_volume(percent);
}

void player_touch_while_i2s(void)
{
    if (s_state == PLAYER_PLAYING) {
        player_pause();
    }
}

bool player_spi_busy(void)
{
    return s_spi_busy;
}

void player_poll(void)
{
    uint8_t half;

    if (s_state != PLAYER_PLAYING) {
        return;
    }

    half = audio_pcm_take_half();
    if (half != 1U && half != 2U) {
        return;
    }

    if (decode_one_frame(s_pcm[half - 1U])) {
        return;
    }

    if (s_eof) {
        stop_audio_keep_file(true);
        set_status("Finished");
        return;
    }

    /* SPI3 is also I2S3 to the headphone jack — SD refill must steal the bus. */
    (void)refill_from_sd();
}

player_state_t player_get_state(void) { return s_state; }
uint8_t player_get_volume(void) { return s_volume; }
uint32_t player_get_position(void) { return s_file_pos; }
uint32_t player_get_size(void) { return s_file_size; }
int player_get_rate(void) { return s_rate; }
int player_get_channels(void) { return s_chans; }
const char *player_get_status_text(void) { return s_status; }
const char *player_get_track_path(void)
{
    if (s_selected >= s_track_count) {
        return "";
    }
    return s_tracks[s_selected];
}
const char *player_get_track_name(void)
{
    return filename_of(player_get_track_path());
}
uint8_t player_get_track_count(void) { return s_track_count; }
uint8_t player_get_selected(void) { return s_selected; }
const char *player_get_track_path_at(uint8_t index)
{
    if (index >= s_track_count) {
        return "";
    }
    return s_tracks[index];
}
const char *player_get_track_name_at(uint8_t index)
{
    return filename_of(player_get_track_path_at(index));
}
