#include "ui_player.h"
#include "player.h"
#include "lvgl.h"

static lv_obj_t *s_status;
static lv_obj_t *s_info;
static lv_obj_t *s_bar;
static lv_obj_t *s_play_btn;
static lv_obj_t *s_vol;
static lv_obj_t *s_list;
static lv_obj_t *s_btns[PLAYER_MAX_TRACKS];

static void refresh_play_label(void)
{
        if (player_get_state() == PLAYER_PLAYING) {
                lv_label_set_text(lv_obj_get_child(s_play_btn, 0), "Pause");
        } else {
                lv_label_set_text(lv_obj_get_child(s_play_btn, 0), "Play");
        }
}

static void highlight_selected(void)
{
        uint8_t i;
        uint8_t n = player_get_track_count();
        uint8_t sel = player_get_selected();

        for (i = 0; i < n; i++) {
                if (s_btns[i] == NULL) {
                        continue;
                }
                if (i == sel) {
                        lv_obj_set_style_bg_color(s_btns[i], lv_color_hex(0x1B4F72), 0);
                } else {
                        lv_obj_set_style_bg_color(s_btns[i], lv_color_hex(0x1C2833), 0);
                }
        }
}

static void on_play(lv_event_t *e)
{
        (void)e;
        player_toggle();
        ui_player_refresh();
}

static void on_stop(lv_event_t *e)
{
        (void)e;
        player_stop();
        ui_player_refresh();
}

static void on_vol(lv_event_t *e)
{
        int32_t v = lv_slider_get_value(lv_event_get_target(e));
        player_set_volume((uint8_t)v);
}

static void on_track(lv_event_t *e)
{
        uint8_t idx = (uint8_t)(uintptr_t)lv_event_get_user_data(e);

        player_play_index(idx);
        ui_player_refresh();
}

static void fill_track_list(void)
{
        uint8_t i;
        uint8_t n = player_get_track_count();

        lv_obj_clean(s_list);
        for (i = 0; i < PLAYER_MAX_TRACKS; i++) {
                s_btns[i] = NULL;
        }

        if (n == 0U) {
                lv_obj_t *empty = lv_label_create(s_list);
                lv_label_set_text(empty, "No .mp3 in music/");
                lv_obj_set_style_text_color(empty, lv_color_hex(0xFF6B6B), 0);
                return;
        }

        for (i = 0; i < n; i++) {
                lv_obj_t *btn = lv_list_add_button(s_list, NULL, player_get_track_name_at(i));
                s_btns[i] = btn;
                lv_obj_set_style_text_font(btn, &lv_font_montserrat_12, 0);
                lv_obj_add_event_cb(btn, on_track, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        }
        highlight_selected();
}

void ui_player_create(void)
{
        lv_obj_t *scr = lv_scr_act();
        lv_obj_t *title;
        lv_obj_t *row;
        lv_obj_t *stop;
        lv_obj_t *vol_lbl;

        lv_obj_set_style_bg_color(scr, lv_color_hex(0x101820), 0);
        lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

        title = lv_label_create(scr);
        lv_label_set_text(title, "Music Player");
        lv_obj_set_style_text_color(title, lv_color_hex(0xF0F0F0), 0);
        lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

        s_status = lv_label_create(scr);
        lv_label_set_text(s_status, "Starting...");
        lv_obj_set_style_text_color(s_status, lv_color_hex(0xFFDC00), 0);
        lv_obj_set_width(s_status, 220);
        lv_obj_set_style_text_align(s_status, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(s_status, &lv_font_montserrat_12, 0);
        lv_obj_align(s_status, LV_ALIGN_TOP_MID, 0, 26);

        s_list = lv_list_create(scr);
        lv_obj_set_size(s_list, 224, 148);
        lv_obj_align(s_list, LV_ALIGN_TOP_MID, 0, 46);
        lv_obj_set_style_bg_color(s_list, lv_color_hex(0x0D1B2A), 0);
        lv_obj_set_style_border_color(s_list, lv_color_hex(0x415A77), 0);
        lv_obj_set_style_pad_row(s_list, 2, 0);
        fill_track_list();

        s_info = lv_label_create(scr);
        lv_label_set_text(s_info, "");
        lv_obj_set_style_text_color(s_info, lv_color_hex(0xCCCCCC), 0);
        lv_obj_set_style_text_font(s_info, &lv_font_montserrat_12, 0);
        lv_obj_set_width(s_info, 220);
        lv_obj_set_style_text_align(s_info, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(s_info, LV_ALIGN_TOP_MID, 0, 198);

        s_bar = lv_bar_create(scr);
        lv_obj_set_size(s_bar, 200, 8);
        lv_obj_align(s_bar, LV_ALIGN_TOP_MID, 0, 216);
        lv_bar_set_range(s_bar, 0, 1000);
        lv_bar_set_value(s_bar, 0, LV_ANIM_OFF);

        row = lv_obj_create(scr);
        lv_obj_remove_style_all(row);
        lv_obj_set_size(row, 220, 40);
        lv_obj_align(row, LV_ALIGN_TOP_MID, 0, 228);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        s_play_btn = lv_btn_create(row);
        lv_obj_set_size(s_play_btn, 90, 34);
        lv_obj_add_event_cb(s_play_btn, on_play, LV_EVENT_CLICKED, NULL);
        lv_label_set_text(lv_label_create(s_play_btn), "Play");
        lv_obj_center(lv_obj_get_child(s_play_btn, 0));

        stop = lv_btn_create(row);
        lv_obj_set_size(stop, 90, 34);
        lv_obj_add_event_cb(stop, on_stop, LV_EVENT_CLICKED, NULL);
        lv_label_set_text(lv_label_create(stop), "Stop");
        lv_obj_center(lv_obj_get_child(stop, 0));

        vol_lbl = lv_label_create(scr);
        lv_label_set_text(vol_lbl, "Volume");
        lv_obj_set_style_text_color(vol_lbl, lv_color_hex(0xAAAAAA), 0);
        lv_obj_set_style_text_font(vol_lbl, &lv_font_montserrat_12, 0);
        lv_obj_align(vol_lbl, LV_ALIGN_TOP_MID, 0, 272);

        s_vol = lv_slider_create(scr);
        lv_obj_set_width(s_vol, 180);
        lv_obj_set_height(s_vol, 10);
        lv_slider_set_range(s_vol, 0, 100);
        lv_slider_set_value(s_vol, player_get_volume(), LV_ANIM_OFF);
        lv_obj_align(s_vol, LV_ALIGN_TOP_MID, 0, 290);
        lv_obj_add_event_cb(s_vol, on_vol, LV_EVENT_VALUE_CHANGED, NULL);
}

void ui_player_refresh(void)
{
        char line[64];
        uint32_t size = player_get_size();
        uint32_t pos = player_get_position();
        int32_t bar = 0;

        lv_label_set_text(s_status, player_get_status_text());
        refresh_play_label();
        highlight_selected();

        lv_snprintf(line, sizeof(line), "%s  %dHz",
                                player_get_track_name(),
                                player_get_rate() > 0 ? player_get_rate() : 0);
        lv_label_set_text(s_info, line);

        if (size > 0U) {
                bar = (int32_t)((pos * 1000U) / size);
                if (bar > 1000) {
                        bar = 1000;
                }
        }
        lv_bar_set_value(s_bar, bar, LV_ANIM_OFF);
}
