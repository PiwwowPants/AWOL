#include "pebble.h"

// Used for drawing hands
#include "main.h"
// Allows the use of colors such as "GColorMidnightGreen"
#ifdef PBL_PLATFORM_BASALT
  #include "gcolor_definitions.h"
#endif

// Main window
static Window *s_main_window;

// Background and hand layers
static Layer *s_solid_bg_layer, *s_hands_layer;
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

// Path for hands
static GPath *s_minute_arrow, *s_hour_arrow;

// Update background when called
static void bg_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

// Update hands when called
static void hands_update_proc(Layer *layer, GContext *ctx) {
  
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  
  // Hour hand
  graphics_context_set_fill_color(ctx, GColorWhite);
  // Define the path that the hour hand will follow
  gpath_rotate_to(s_hour_arrow, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, s_hour_arrow);
  // Draw outline if black and white

  // Minute hand
  #ifdef PBL_COLOR
    graphics_context_set_fill_color(ctx, GColorRed);
  #else
    graphics_context_set_fill_color(ctx, GColorWhite);
  #endif
  // Define the path that the minute hand will follow
  gpath_rotate_to(s_minute_arrow, TRIG_MAX_ANGLE * t->tm_min / 60);
  gpath_draw_filled(ctx, s_minute_arrow);
}

// Update hands every second when called
static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(s_main_window));
}

// Loads the layers onto the main window
static void window_load(Window *s_main_window) {
  
  // Creates window_layer as root and sets its bounds
  Layer *window_layer = window_get_root_layer(s_main_window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create the simple single color backgroud behind the face layer. Then apply it to the window layer
  s_solid_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_solid_bg_layer, bg_update_proc);
  layer_add_child(window_layer, s_solid_bg_layer);

  // Create the face on the background layer above the solid color. Then apply it to the window layer
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BACKGROUND);
  s_background_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  #ifdef PBL_PLATFORM_BASALT
    bitmap_layer_set_compositing_mode(s_background_layer, GCompOpSet);
  #endif
  #ifdef PBL_PLATFORM_APLITE
    layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));
  #endif

  // Draws the hands. Then apply it the window layer
  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(window_layer, s_hands_layer);
  
  #ifdef PBL_PLATFORM_BASALT
  layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));
  #endif
}

// Unload the layers from the main window
static void window_unload(Window *s_main_window) {
  
  // Destroy the background color
  layer_destroy(s_solid_bg_layer);
  
  // Destroy the watch face
  gbitmap_destroy(s_background_bitmap);
  bitmap_layer_destroy(s_background_layer);

  // Destroy the hands
  layer_destroy(s_hands_layer);
}

// Initialize the main window
static void init() {
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_main_window, true);

  // Initialize the paths of the hands
  s_minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
  s_hour_arrow = gpath_create(&HOUR_HAND_POINTS);

  // Set the position of the hands to the center
  Layer *window_layer = window_get_root_layer(s_main_window);
  GRect bounds = layer_get_bounds(window_layer);
  GPoint center = grect_center_point(&bounds);
  gpath_move_to(s_minute_arrow, center);
  gpath_move_to(s_hour_arrow, center);

  // Call to the handle_second_tick to update the watch every second
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
}

// Deinitialize the main window
static void deinit() {
  
  // Destroy the hand paths
  gpath_destroy(s_minute_arrow);
  gpath_destroy(s_hour_arrow);

  // Unsubscribe from the tick timer
  tick_timer_service_unsubscribe();
  
  // Destroy the main window
  window_destroy(s_main_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}