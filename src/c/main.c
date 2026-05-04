#include <pebble.h>

// 8 vivid colors matching the original watchface palette
#define NUM_COLORS 8
static const uint8_t s_palette[NUM_COLORS] = {
  0xDC,  // Green  #55FF00
  0xCF,  // Cyan   #00FFFF
  0xFC,  // Yellow #FFFF00
  0xF8,  // Orange #FFAA00
  0xF4,  // Red    #FF5500
  0xD7,  // Blue   #5555FF
  0xFF,  // White  #FFFFFF
  0xF7,  // Pink   #FF55FF
};

// Persistent storage keys
#define PKEY_COLOR       1
#define PKEY_BATTERY     2
#define PKEY_STEPS       3
#define PKEY_DATE        4
#define PKEY_DAY         5
#define PKEY_24H         6
#define PKEY_AUTO        7
#define PKEY_RAINBOW     8
#define PKEY_COLOR_IDX   9

static Window    *s_window;
static Layer     *s_layer;
static struct tm  s_now;

static GColor s_accent;
static int    s_color_idx;
static bool   s_auto_color;
static bool   s_rainbow;
static bool   s_show_battery;
static bool   s_show_date;
static bool   s_show_day;
static bool   s_show_steps;
static bool   s_use_24h;

static GColor color_at(int idx) {
  return (GColor){.argb = s_palette[idx % NUM_COLORS]};
}

static void advance_color(void) {
  s_color_idx = (s_color_idx + 1) % NUM_COLORS;
  s_accent = color_at(s_color_idx);
  persist_write_int(PKEY_COLOR_IDX, s_color_idx);
}

static void load_settings(void) {
  s_color_idx   = persist_read_int(PKEY_COLOR_IDX);
  s_accent      = color_at(s_color_idx);
  s_show_battery = persist_exists(PKEY_BATTERY) ? persist_read_bool(PKEY_BATTERY) : true;
  s_show_steps   = persist_exists(PKEY_STEPS)   ? persist_read_bool(PKEY_STEPS)   : true;
  s_show_date    = persist_exists(PKEY_DATE)     ? persist_read_bool(PKEY_DATE)    : true;
  s_show_day     = persist_exists(PKEY_DAY)      ? persist_read_bool(PKEY_DAY)     : true;
  s_use_24h      = persist_exists(PKEY_24H)      ? persist_read_bool(PKEY_24H)     : false;
  s_auto_color   = persist_exists(PKEY_AUTO)     ? persist_read_bool(PKEY_AUTO)    : false;
  s_rainbow      = persist_exists(PKEY_RAINBOW)  ? persist_read_bool(PKEY_RAINBOW) : false;
  // Apply fixed color choice when not in auto/rainbow mode
  if (!s_auto_color && !s_rainbow && persist_exists(PKEY_COLOR)) {
    int c = persist_read_int(PKEY_COLOR);
    if (c >= 0 && c < NUM_COLORS) {
      s_color_idx = c;
      s_accent = color_at(c);
    }
  }
}

static void layer_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int w = bounds.size.w, h = bounds.size.h;

  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Battery bar (4px strip at top)
  if (s_show_battery) {
    BatteryChargeState batt = battery_state_service_peek();
    int fill_w = w * batt.charge_percent / 100;
    graphics_context_set_fill_color(ctx, GColorDarkGray);
    graphics_fill_rect(ctx, GRect(0, 0, w, 4), 0, GCornerNone);
#ifdef PBL_COLOR
    graphics_context_set_fill_color(ctx, batt.charge_percent > 20 ? s_accent : GColorRed);
#else
    graphics_context_set_fill_color(ctx, GColorWhite);
#endif
    graphics_fill_rect(ctx, GRect(0, 0, fill_w, 4), 0, GCornerNone);
  }

  int y = 10;

  // Day of week
  if (s_show_day) {
    char buf[12];
    strftime(buf, sizeof(buf), "%A", &s_now);
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, buf,
      fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
      GRect(0, y, w, 22),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    y += 23;
  }

  // Date
  if (s_show_date) {
    char buf[16];
    strftime(buf, sizeof(buf), "%b %d", &s_now);
    graphics_context_set_text_color(ctx, GColorLightGray);
    graphics_draw_text(ctx, buf,
      fonts_get_system_font(FONT_KEY_GOTHIC_18),
      GRect(0, y, w, 22),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    y += 23;
  }

  // Time — vertically centered in remaining space above steps
  char time_buf[8];
  if (s_use_24h) {
    strftime(time_buf, sizeof(time_buf), "%H:%M", &s_now);
  } else {
    strftime(time_buf, sizeof(time_buf), "%I:%M", &s_now);
    if (time_buf[0] == '0') memmove(time_buf, time_buf + 1, strlen(time_buf));
  }
  int time_h = 52;
  int bottom_reserve = s_show_steps ? 22 : 4;
  int time_y = y + (h - bottom_reserve - y - time_h) / 2;
  if (time_y < y) time_y = y;

#ifdef PBL_COLOR
  graphics_context_set_text_color(ctx, s_accent);
#else
  graphics_context_set_text_color(ctx, GColorWhite);
#endif
  graphics_draw_text(ctx, time_buf,
    fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD),
    GRect(0, time_y, w, time_h + 4),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  // Step count
  if (s_show_steps) {
#ifdef PBL_HEALTH
    HealthServiceAccessibilityMask mask = health_service_metric_accessible(
      HealthMetricStepCount, time(NULL) - 86400, time(NULL));
    if (mask & HealthServiceAccessibilityMaskAvailable) {
      int steps = (int)health_service_sum_today(HealthMetricStepCount);
      char buf[20];
      snprintf(buf, sizeof(buf), "%d steps", steps);
      graphics_context_set_text_color(ctx, GColorLightGray);
      graphics_draw_text(ctx, buf,
        fonts_get_system_font(FONT_KEY_GOTHIC_14),
        GRect(0, h - 20, w, 16),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    }
#endif
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  s_now = *tick_time;
  if (s_rainbow) {
    advance_color();
  } else if (s_auto_color && (units_changed & HOUR_UNIT)) {
    advance_color();
  }
  layer_mark_dirty(s_layer);
}

static void inbox_received(DictionaryIterator *iter, void *context) {
  Tuple *t;

  t = dict_find(iter, MESSAGE_KEY_COLOR);
  if (t) {
    int idx = (int)t->value->int32;
    if (idx >= 0 && idx < NUM_COLORS) {
      s_color_idx = idx;
      s_accent = color_at(idx);
      persist_write_int(PKEY_COLOR, idx);
      persist_write_int(PKEY_COLOR_IDX, idx);
    }
  }

  t = dict_find(iter, MESSAGE_KEY_SHOW_BATTERY);
  if (t) { s_show_battery = t->value->int32; persist_write_bool(PKEY_BATTERY, s_show_battery); }

  t = dict_find(iter, MESSAGE_KEY_SHOW_STEPS);
  if (t) { s_show_steps = t->value->int32; persist_write_bool(PKEY_STEPS, s_show_steps); }

  t = dict_find(iter, MESSAGE_KEY_SHOW_DATE);
  if (t) { s_show_date = t->value->int32; persist_write_bool(PKEY_DATE, s_show_date); }

  t = dict_find(iter, MESSAGE_KEY_SHOW_DAY);
  if (t) { s_show_day = t->value->int32; persist_write_bool(PKEY_DAY, s_show_day); }

  t = dict_find(iter, MESSAGE_KEY_USE_24H);
  if (t) { s_use_24h = t->value->int32; persist_write_bool(PKEY_24H, s_use_24h); }

  t = dict_find(iter, MESSAGE_KEY_AUTO_COLOR);
  if (t) { s_auto_color = t->value->int32; persist_write_bool(PKEY_AUTO, s_auto_color); }

  t = dict_find(iter, MESSAGE_KEY_RAINBOW);
  if (t) { s_rainbow = t->value->int32; persist_write_bool(PKEY_RAINBOW, s_rainbow); }

  layer_mark_dirty(s_layer);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);
  s_layer = layer_create(bounds);
  layer_set_update_proc(s_layer, layer_update);
  layer_add_child(root, s_layer);
}

static void window_unload(Window *window) {
  layer_destroy(s_layer);
}

static void init(void) {
  load_settings();
  time_t now = time(NULL);
  s_now = *localtime(&now);

  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load   = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  app_message_register_inbox_received(inbox_received);
  app_message_open(256, 64);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
