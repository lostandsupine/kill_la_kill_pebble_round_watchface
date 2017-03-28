#include <pebble.h>

static Window *s_window;
static TextLayer *s_text_layer;
static TextLayer *s_text_layer_2;
static Layer *s_canvas_layer;
static GBitmap *s_bitmap;
static int show_battery;
static time_t showing_battery_start_time;

static void change_tick_time(TimeUnits tick_units);

static void update_time() {
  // set time text layer
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);
  text_layer_set_text(s_text_layer, s_buffer);

  // if showing the battery, timeout after 3 seconds
  if (show_battery == 1){
    if ((time(NULL) - showing_battery_start_time) >= 3){
      show_battery = 0;
      text_layer_set_text(s_text_layer_2, "");
      change_tick_time(MINUTE_UNIT);
    }
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void change_tick_time(TimeUnits tick_units){
  tick_timer_service_unsubscribe();
  tick_timer_service_subscribe(tick_units, tick_handler);
}

// on acceleration event start showing the battery and start the timer for timeout
static void accel_tap_handler(AccelAxisType axis, int32_t direction) {
  static char s_buffer_2[3];
  snprintf(s_buffer_2, sizeof(s_buffer_2), "%d", (int)battery_state_service_peek().charge_percent);
  text_layer_set_text(s_text_layer_2, s_buffer_2);

  change_tick_time(SECOND_UNIT);
  show_battery = 1;
  showing_battery_start_time = time(NULL);
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bitmap_bounds = gbitmap_get_bounds(s_bitmap);
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, s_bitmap, bitmap_bounds);
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // image layer
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_get_root_layer(window), s_canvas_layer);

  // time text layer
  s_text_layer = text_layer_create(GRect(0, 140, bounds.size.w, 50));
  text_layer_set_background_color(s_text_layer, GColorFromRGB (170, 170, 170));
  text_layer_set_text_color(s_text_layer, GColorBlack);
  text_layer_set_text(s_text_layer, "00:00");
  text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM));
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_text_layer));
  
  // battery text layer 2
  s_text_layer_2 = text_layer_create(GRect(140, 70, 40, 25));
  text_layer_set_background_color(s_text_layer_2, GColorFromRGB (170, 170, 170));
  text_layer_set_text_color(s_text_layer_2, GColorBlack);
  text_layer_set_text(s_text_layer_2, "");
  text_layer_set_font(s_text_layer_2, fonts_get_system_font(FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM));
  text_layer_set_text_alignment(s_text_layer_2, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_text_layer_2));

}

static void prv_window_unload(Window *window) {
  text_layer_destroy(s_text_layer);
  text_layer_destroy(s_text_layer_2);
  layer_destroy(s_canvas_layer);
}

static void prv_init(void) {
  show_battery = 0;

  s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_kill_la_kill);
  accel_tap_service_subscribe(accel_tap_handler);

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  const bool animated = true;
  window_stack_push(s_window, animated);

  update_time();

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void prv_deinit(void) {
  window_destroy(s_window);
  gbitmap_destroy(s_bitmap);
  accel_tap_service_unsubscribe();
}

int main(void) {
  prv_init();

  app_event_loop();
  prv_deinit();
}
