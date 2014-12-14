#include "pebble.h"

#define BUFFER_SIZE 86

static struct CommonData {
    TextLayer *label1, *label2, *label3, *labelc;
    Window *window;
    char buffer1[BUFFER_SIZE], buffer2[BUFFER_SIZE], buffer3[BUFFER_SIZE], bufferc[BUFFER_SIZE];
} s_data;

#define STR_OVER "over"
#define STR_VOOR "voor"
#define STR_HALF "half"
#define STR_UUR "uur"
#define STR_OVERHALF STR_OVER " " STR_HALF
#define STR_VOORHALF STR_VOOR " " STR_HALF

#define FONT_BOLD RESOURCE_ID_FONT_ROBOTO_BOLD_35
#define FONT_LIGHT RESOURCE_ID_FONT_ROBOTO_LIGHT_35
#define FONT_LARGE RESOURCE_ID_FONT_ROBOTO_BOLD_42

#define TEXT_Y_OFFSET -7

static const char* const NUMWORDS[] = {
    "",
    "een",
    "twee",
    "drie",
    "vier",
    "vijf",
    "zes",
    "zeven",
    "acht",
    "negen",
    "tien",
    "elf",
    "twaalf",
    "dertien",
    "veertien",
    "kwart",
};

static void set_three_text() {
    text_layer_set_text(s_data.label1, s_data.buffer1);
    text_layer_set_text(s_data.label2, s_data.buffer2);
    text_layer_set_text(s_data.label3, s_data.buffer3);
    layer_set_hidden((Layer*)s_data.label1, false);
    layer_set_hidden((Layer*)s_data.label2, false);
    layer_set_hidden((Layer*)s_data.label3, false);
    layer_set_hidden((Layer*)s_data.labelc, true);
}

static int16_t get_text_size(const char* text, int font_id, GRect frame) {
    GFont font = fonts_load_custom_font(resource_get_handle(font_id));

    GSize size = graphics_text_layout_get_content_size(
        text,
        font,
        GRect(0,0, frame.size.w, frame.size.h),
        GTextOverflowModeWordWrap,
        GTextAlignmentCenter);

    return size.h;
}

static void set_central_text() {
    Layer *root_layer = window_get_root_layer(s_data.window);
    GRect frame = layer_get_frame(root_layer);
    int16_t start = (frame.size.h - get_text_size(s_data.bufferc, FONT_LARGE, frame)) / 2 + TEXT_Y_OFFSET;
    layer_set_frame((Layer*)s_data.labelc, GRect(0, start, frame.size.w, frame.size.h));

    text_layer_set_text(s_data.labelc, s_data.bufferc);
    layer_set_hidden((Layer*)s_data.label1, true);
    layer_set_hidden((Layer*)s_data.label2, true);
    layer_set_hidden((Layer*)s_data.label3, true);
    layer_set_hidden((Layer*)s_data.labelc, false);
}

void time_to_text(int hours, int minutes)
{
    if (minutes > 15) hours += 1;

    if (hours == 0) hours = 12;
    else if (hours > 24) hours -= 24;
    else if (hours > 12) hours -= 12;

    s_data.bufferc[0] = 0;
    s_data.buffer1[0] = 0;
    s_data.buffer2[0] = 0;
    s_data.buffer3[0] = 0;

    if (minutes == 0) {
        strcat(s_data.bufferc, NUMWORDS[hours]);
        strcat(s_data.bufferc, " " STR_UUR);
        set_central_text();
    }
    else if (minutes == 30) {
        strcat(s_data.bufferc, STR_HALF " ");
        strcat(s_data.bufferc, NUMWORDS[hours]);
        set_central_text();
    }
    else if (minutes <= 15) {
        strcat(s_data.buffer1, NUMWORDS[minutes]);
        strcat(s_data.buffer2, STR_OVER);
        strcat(s_data.buffer3, NUMWORDS[hours]);
        set_three_text();
    }
    else if (minutes < 30) {
        strcat(s_data.buffer1, NUMWORDS[30 - minutes]);
        strcat(s_data.buffer2, STR_VOORHALF);
        strcat(s_data.buffer3, NUMWORDS[hours]);
        set_three_text();
    }
    else if (minutes < 45) {
        strcat(s_data.buffer1, NUMWORDS[minutes - 30]);
        strcat(s_data.buffer2, STR_OVERHALF);
        strcat(s_data.buffer3, NUMWORDS[hours]);
        set_three_text();
    }
    else {
        strcat(s_data.buffer1, NUMWORDS[60 - minutes]);
        strcat(s_data.buffer2, STR_VOOR);
        strcat(s_data.buffer3, NUMWORDS[hours]);
        set_three_text();
    }
}

static void update_time(struct tm* t) {
    time_to_text(t->tm_hour, t->tm_min);
    text_layer_set_text(s_data.labelc, s_data.bufferc);
}
static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
    update_time(tick_time);
}

static void setup_label(Layer *root_layer, TextLayer **layer, int8_t num) {
    GRect frame = layer_get_frame(root_layer);

    int fontid;
    switch (num) {
    case -1:
        fontid = FONT_LARGE;
        break;
    default:
    case 0:
    case 2:
        fontid = FONT_BOLD;
        break;
    case 1:
        fontid = FONT_LIGHT;
        break;
    }

    GFont font = fonts_load_custom_font(resource_get_handle(fontid));

    int16_t height = frame.size.h / (num == -1 ? 1 : 3);

    int16_t halftsize = get_text_size("hgft", FONT_BOLD, frame) / 2;
    int16_t halftsize2 = get_text_size("hgft", FONT_LIGHT, frame) / 2;
    int16_t start;
    switch (num) {
    default:
        start = 0;
        break;
    case 0:
        start = frame.size.h * 0.25 - halftsize + TEXT_Y_OFFSET;
        break;
    case 1:
        start = frame.size.h * 0.5 - halftsize2 + TEXT_Y_OFFSET;
        break;
    case 2:
        start = frame.size.h * 0.75 - halftsize + TEXT_Y_OFFSET;
        break;
    }

    *layer = text_layer_create(GRect(0, start, frame.size.w, height));
    text_layer_set_background_color(*layer, GColorBlack);
    text_layer_set_text_color(*layer, GColorWhite);
    text_layer_set_font(*layer, font);
    text_layer_set_text_alignment(*layer, GTextAlignmentCenter);
    layer_add_child(root_layer, text_layer_get_layer(*layer));
}

static void do_init(void) {
    s_data.window = window_create();

    const bool animated = true;
    window_stack_push(s_data.window, animated);
    window_set_background_color(s_data.window, GColorBlack);

    Layer *root_layer = window_get_root_layer(s_data.window);

    setup_label(root_layer, &s_data.label1, 0);
    setup_label(root_layer, &s_data.label2, 1);
    setup_label(root_layer, &s_data.label3, 2);
    setup_label(root_layer, &s_data.labelc, -1);

    layer_set_hidden((Layer*)s_data.label1, true);
    layer_set_hidden((Layer*)s_data.label2, true);
    layer_set_hidden((Layer*)s_data.label3, true);
    layer_set_hidden((Layer*)s_data.labelc, true);

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    update_time(t);

    tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick);
}

static void do_deinit(void) {
    window_destroy(s_data.window);
    text_layer_destroy(s_data.label1);
    text_layer_destroy(s_data.label2);
    text_layer_destroy(s_data.label3);
    text_layer_destroy(s_data.labelc);
}

int main(void) {
    do_init();
    app_event_loop();
    do_deinit();
}
