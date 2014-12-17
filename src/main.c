#include "pebble.h"

#define BUFFER_SIZE 86

static struct CommonData {
    TextLayer *label1, *label2, *label3, *labelc, *labeld;
    Window *window;
    char buffer1[BUFFER_SIZE], buffer2[BUFFER_SIZE], buffer3[BUFFER_SIZE], bufferc[BUFFER_SIZE], bufferd[BUFFER_SIZE];
} s_data;

static struct SizeData {
    int16_t half_height_bold, half_height_light;
    GRect frame;
} s_sizes;

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
#define DATE_CORNER_SIZE 20

static const char* const NUMWORDS[] = {
    NULL,
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

static void itoa(int num, char* buf) {
    if (num <= 0) {
        buf[0] = '0';
        buf[1] = 0;
        return;
    }

    int numcpy = num;
    int length = 0;

    while (numcpy) {
        length += 1;
        numcpy /= 10;
    }

    numcpy = num;
    for (int i = 0; i < length; ++i) {
        buf[length - i - 1] = '0' + (numcpy % 10);
        numcpy /= 10;
    }

    buf[length] = 0;
}

static int16_t get_text_size(const char* text, int font_id) {
    GFont font = fonts_load_custom_font(resource_get_handle(font_id));

    GSize size = graphics_text_layout_get_content_size(
        text,
        font,
        GRect(0,0, s_sizes.frame.size.w, s_sizes.frame.size.h),
        GTextOverflowModeWordWrap,
        GTextAlignmentCenter);

    return size.h;
}

static void update_visibility(bool show_group_of_three) {
    layer_set_hidden(text_layer_get_layer(s_data.label1), !show_group_of_three);
    layer_set_hidden(text_layer_get_layer(s_data.label2), !show_group_of_three);
    layer_set_hidden(text_layer_get_layer(s_data.label3), !show_group_of_three);
    layer_set_hidden(text_layer_get_layer(s_data.labelc), show_group_of_three);
}

static void set_three_text() {
    text_layer_set_text(s_data.label1, s_data.buffer1);
    text_layer_set_text(s_data.label2, s_data.buffer2);
    text_layer_set_text(s_data.label3, s_data.buffer3);
    update_visibility(true);
}

static void set_central_text() {
    GRect frame = s_sizes.frame;
    int16_t start = (frame.size.h - get_text_size(s_data.bufferc, FONT_LARGE)) / 2 + TEXT_Y_OFFSET;
    layer_set_frame(text_layer_get_layer(s_data.labelc), GRect(0, start, frame.size.w, frame.size.h));

    text_layer_set_text(s_data.labelc, s_data.bufferc);
    update_visibility(false);
}

static void clear_buffers() {
    s_data.bufferc[0] = 0;
    s_data.buffer1[0] = 0;
    s_data.buffer2[0] = 0;
    s_data.buffer3[0] = 0;
}

int time_to_text(int hours, int minutes)
{
    if (minutes > 15) hours += 1;

    if (hours == 0) hours = 12;
    else if (hours > 24) hours -= 24;
    else if (hours > 12) hours -= 12;

    clear_buffers();

    if (minutes == 0) {
        strcat(s_data.bufferc, NUMWORDS[hours]);
        strcat(s_data.bufferc, " " STR_UUR);
        return 1;
    }
    else if (minutes == 30) {
        strcat(s_data.bufferc, STR_HALF " ");
        strcat(s_data.bufferc, NUMWORDS[hours]);
        return 1;
    }
    else if (minutes <= 15) {
        strcat(s_data.buffer1, NUMWORDS[minutes]);
        strcat(s_data.buffer2, STR_OVER);
        strcat(s_data.buffer3, NUMWORDS[hours]);
        return 3;
    }
    else if (minutes < 30) {
        strcat(s_data.buffer1, NUMWORDS[30 - minutes]);
        strcat(s_data.buffer2, STR_VOORHALF);
        strcat(s_data.buffer3, NUMWORDS[hours]);
        return 3;
    }
    else if (minutes < 45) {
        strcat(s_data.buffer1, NUMWORDS[minutes - 30]);
        strcat(s_data.buffer2, STR_OVERHALF);
        strcat(s_data.buffer3, NUMWORDS[hours]);
        return 3;
    }
    else { // if minutes between 45 and 60
        strcat(s_data.buffer1, NUMWORDS[60 - minutes]);
        strcat(s_data.buffer2, STR_VOOR);
        strcat(s_data.buffer3, NUMWORDS[hours]);
        return 3;
    }
}

static void update_date(int mday) {
    itoa(mday, s_data.bufferd);
    text_layer_set_text(s_data.labeld, s_data.bufferd);
}

static void update_time(struct tm* t) {
    int lines_of_text = time_to_text(t->tm_hour, t->tm_min);

    switch (lines_of_text) {
    case 1:
        set_central_text();
        break;
    case 3:
        set_three_text();
        break;
    }
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
    update_time(tick_time);
    if (units_changed & DAY_UNIT) update_date(tick_time->tm_mday);
}

static void update_time_with_now() {
    time_t time_t_now = time(NULL);
    struct tm *tm_now = localtime(&time_t_now);
    update_time(tm_now);
    update_date(tm_now->tm_mday);
}

static TextLayer* build_label(int16_t starty, int16_t height, int fontid) {
    GFont font = fonts_load_custom_font(resource_get_handle(fontid));

    TextLayer *layer = text_layer_create(GRect(0, starty, s_sizes.frame.size.w, height));
    text_layer_set_background_color(layer, GColorClear);
    text_layer_set_text_color(layer, GColorWhite);
    text_layer_set_font(layer, font);
    text_layer_set_text_alignment(layer, GTextAlignmentCenter);
    return layer;
}

static TextLayer* build_datelabel() {
    GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_14);

    TextLayer *layer = text_layer_create(GRect(s_sizes.frame.size.w - DATE_CORNER_SIZE, s_sizes.frame.size.h - DATE_CORNER_SIZE, DATE_CORNER_SIZE, DATE_CORNER_SIZE));
    text_layer_set_background_color(layer, GColorClear);
    text_layer_set_text_color(layer, GColorWhite);
    text_layer_set_font(layer, font);
    text_layer_set_text_alignment(layer, GTextAlignmentCenter);

    return layer;
}

static void create_window() {
    s_data.window = window_create();

    window_stack_push(s_data.window, true);
    window_set_background_color(s_data.window, GColorBlack);
}

static void subscribe_timer() {
    tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick);
}

static void calculate_sizes() {
    Layer *root_layer = window_get_root_layer(s_data.window);

    s_sizes.frame = layer_get_frame(root_layer);
    s_sizes.half_height_bold = get_text_size("hgft", FONT_BOLD) / 2;
    s_sizes.half_height_light = get_text_size("hgft", FONT_LIGHT) / 2;
}

static void create_labels() {
    Layer *root_layer = window_get_root_layer(s_data.window);
    int16_t h = s_sizes.frame.size.h;

    s_data.label1 = build_label(h * 0.25 - s_sizes.half_height_bold + TEXT_Y_OFFSET, h / 3, FONT_BOLD);
    s_data.label2 = build_label(h * 0.5 - s_sizes.half_height_bold + TEXT_Y_OFFSET, h / 3, FONT_LIGHT);
    s_data.label3 = build_label(h * 0.75 - s_sizes.half_height_bold + TEXT_Y_OFFSET, h / 3, FONT_BOLD);
    s_data.labelc = build_label(0, h, FONT_LARGE);
    s_data.labeld = build_datelabel();

    layer_add_child(root_layer, text_layer_get_layer(s_data.label1));
    layer_add_child(root_layer, text_layer_get_layer(s_data.label2));
    layer_add_child(root_layer, text_layer_get_layer(s_data.label3));
    layer_add_child(root_layer, text_layer_get_layer(s_data.labelc));
    layer_add_child(root_layer, text_layer_get_layer(s_data.labeld));
}

static void do_init(void) {
    create_window();
    calculate_sizes();
    create_labels();
    update_time_with_now();
    subscribe_timer();
}

static void do_deinit(void) {
    window_destroy(s_data.window);
    text_layer_destroy(s_data.label1);
    text_layer_destroy(s_data.label2);
    text_layer_destroy(s_data.label3);
    text_layer_destroy(s_data.labelc);
    text_layer_destroy(s_data.labeld);
}

int main(void) {
    do_init();
    app_event_loop();
    do_deinit();
}
