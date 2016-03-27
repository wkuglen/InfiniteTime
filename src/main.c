#include <pebble.h>
static Window *s_main_window;
static TextLayer *s_time_layer, *s_date_layer;

static Layer *s_canvas_layer;

static GPoint s_center;
static GPoint s_left_top;
static GPoint s_left_center;
static GPoint s_right_top;
static GPoint s_right_center;
static int s_radius;

//static int s_debug = 60*6;

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
  
  // Copy date into buffer from tm structure
  static char date_buffer[16];
  strftime(date_buffer, sizeof(date_buffer), "%a %d %b", tick_time);

  // Show the date
  text_layer_set_text(s_date_layer, date_buffer);

}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  
  // Redraw
  if(s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }
}

static void update_proc(Layer *layer, GContext *ctx) {
  int32_t angle_start, angle_end;// = DEG_TO_TRIGANGLE(15);
  GRect rect_bounds;
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  int hour = (tick_time->tm_hour);
  float timeFraction = hour + ((tick_time->tm_min)/60);
  if (hour < 12) {
    //Morning
    if (timeFraction <= 6) {
      angle_start = DEG_TO_TRIGANGLE(90 - (180 - (30*timeFraction)));
      angle_end = DEG_TO_TRIGANGLE(90 - (162 - (30 * timeFraction)));
    } else {
      angle_start = DEG_TO_TRIGANGLE(90 - (540 - (30*timeFraction)));
      angle_end = DEG_TO_TRIGANGLE(90 - (522 - (30*timeFraction)));
    }
    rect_bounds = GRect(s_right_top.x, s_right_top.y, 2*s_radius, 2*s_radius);
  } else {
    //Afternoon
    rect_bounds = GRect(s_left_top.x, s_left_top.y, 2*s_radius, 2*s_radius);
    angle_start = DEG_TO_TRIGANGLE(90 - ((timeFraction-12) * 30));
    angle_end = DEG_TO_TRIGANGLE(90 - ((timeFraction-12) * 30 - 18));
  }
  printf("%i %i", (int)angle_start, (int)angle_end);
  
  /*  // Cyan Circles
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, s_left_center, s_radius);
  graphics_fill_circle(ctx, s_right_center, s_radius);  
  */
  //DRAW CIRCLES
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 4);
  // Draw outline
  graphics_draw_circle(ctx, s_left_center, s_radius);
  graphics_draw_circle(ctx, s_right_center, s_radius);
 
  // Draw an arc
  // Set the fill color
  graphics_context_set_fill_color(ctx, GColorOrange);
  //graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_fill_radial(ctx, rect_bounds, GOvalScaleModeFitCircle, 15, angle_start, angle_end);
  

  
}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_center = grect_center_point(&bounds);
  s_left_center = GPoint(2*s_center.x / 3, 3*s_center.y/4);
  s_right_center = GPoint(s_center.x + (s_center.x / 3), 3*s_center.y/4);
  
  s_radius = s_center.x / 3;
  s_left_top = GPoint(s_left_center.x - s_radius, s_left_center.y - s_radius);
  s_right_top = GPoint(s_center.x, s_right_center.y - s_radius);

  s_canvas_layer = layer_create(bounds);
  
  layer_set_update_proc(s_canvas_layer, update_proc);
  layer_add_child(window_layer, s_canvas_layer);
  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(
      GRect(0, bounds.size.h - 80, bounds.size.w, 50));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  
  // Create date TextLayer
  s_date_layer = text_layer_create(GRect(0, bounds.size.h - 30, 144, 30));
  text_layer_set_text_color(s_date_layer, GColorBlack);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

  // Add to Window
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));

}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  
  // Destroy Drawing
  layer_destroy(s_canvas_layer);
}

static void init() {
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Create main Window element and assign to pointer
  s_main_window = window_create();
  
  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Make sure the time is displayed from the start
  update_time();

}

static void deinit() {
  window_destroy(s_main_window);
  
}


int main(void) {
  init();
  app_event_loop();
  deinit();
}


