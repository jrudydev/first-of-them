#include <pebble.h>

static Window *window;
static TextLayer *text_layer;
static TextLayer *high_score_layer;
static TextLayer *msg_layer;
static Layer *square_layer;
static Layer *bullseye_layer;
static Layer *marker_layer;
static Layer *bullet_layer;
// Timers can be canceled with `app_timer_cancel()`
static AppTimer *timer;
static GPoint markerPos;
static GPoint bulletPos;
static bool direction = true;
static float speed = 5;
static float bulletSpeed = 10;
static int count = 0;
static int markerMin = -20;
static int markerMax = 20;
static Layer* arr[100];
static int zombieCount = 0;
static bool isShooting = false;
static bool isGameOver = false;

const int HIGH_SCORE_KEY = 1337;


static const GPathInfo SQUARE_POINTS = {
  4,
  (GPoint []) {
    {-73, 65},
    {-73,  75},
    { 72,  75},
    { 72, 65}
  }
};

static const GPathInfo BULLSEYE_POINTS = {
  4,
  (GPoint []) {
    {-20, 65},
    {-20, 75},
    {20, 75},
    {20, 65}
  }
};

static const GPathInfo 	MARKER_POINTS = {
  3,
  (GPoint []) {
    {-10, 55},
    {0,  65},
    { 10,  55}
  }
};

static const GPathInfo 	BULLET_POINTS = {
  4,
  (GPoint []) {
    {-1,  1},
    {-1, -1},
    { 1, -1},
    { 1,  1}
  }
};

static GPath *square_path;
static GPath *bullseye_path;
static GPath *marker_path;
static GPath *bullet_path;

static Layer *player_layer;
static GBitmap *player_image;

static Layer *zombie_layer;
static GBitmap *zombie_image;

static Layer *background_layer;
static GBitmap *background_image;

static void set_score() {
	int highScore = persist_read_int(HIGH_SCORE_KEY);

	if (count > highScore) {
		// Save persistent data
		persist_write_int(HIGH_SCORE_KEY, count);	
		highScore = count;
	}
	
	// Display high score in text_layer
	static char buf[32];
	snprintf(buf, 32, "High Score: %u", highScore);
	text_layer_set_text(high_score_layer, buf);
}

static void player_layer_update_callback(Layer *me, GContext* ctx) {
  // We make sure the dimensions of the GRect to draw into
  // are equal to the size of the bitmap--otherwise the image
  // will automatically tile. Which might be what *you* want.

  GRect bounds = player_image->bounds;

  graphics_draw_bitmap_in_rect(ctx, player_image, (GRect) { .origin = { 0, 0 }, .size = bounds.size });
}

static void zombie_layer_update_callback(Layer *me, GContext* ctx) {
  // We make sure the dimensions of the GRect to draw into
  // are equal to the size of the bitmap--otherwise the image
  // will automatically tile. Which might be what *you* want.
if (!isGameOver)
{
  GRect rect = layer_get_frame(me);
  rect.origin.x = rect.origin.x - 1;
  
  if (rect.origin.x < -100)
  {
	isGameOver = true;
	
	set_score();
	
	static char body_text[50];
	snprintf(body_text, sizeof(body_text), "Game Over");
	text_layer_set_text(msg_layer, body_text);
  }
  layer_set_frame(me, rect);   
}
  GRect bounds = zombie_image->bounds;

  graphics_context_set_compositing_mode(ctx,GCompOpAnd);
  graphics_draw_bitmap_in_rect(ctx, zombie_image, (GRect) { .origin = { 100, 2}, .size = bounds.size });

  //graphics_draw_bitmap_in_rect(ctx, image, (GRect) { .origin = { 80, 60 }, .size = bounds.size });
}


static void background_layer_update_callback(Layer *me, GContext* ctx) {
  // We make sure the dimensions of the GRect to draw into
  // are equal to the size of the bitmap--otherwise the image
  // will automatically tile. Which might be what *you* want.

  GRect bounds = background_image->bounds;

  graphics_draw_bitmap_in_rect(ctx, background_image, (GRect) { .origin = { 0, 18 }, .size = bounds.size });
}

static void update_square_layer(Layer *layer, GContext* ctx) {
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_fill_color(ctx, GColorWhite);
  gpath_draw_filled(ctx, square_path);
  gpath_draw_outline(ctx, square_path);
}

static void update_bullseye_layer(Layer *layer, GContext* ctx) {

  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_fill_color(ctx, GColorBlack);
  gpath_draw_filled(ctx, bullseye_path);
  gpath_draw_outline(ctx, bullseye_path);
}

static void update_marker_layer(Layer *layer, GContext* ctx) {
	if (!isGameOver)
	{
	  Layer *window_layer = window_get_root_layer(window);
	  GRect bounds = layer_get_bounds(window_layer);

	  int posX = (direction) ? markerPos.x + speed:  markerPos.x - speed;
  
	  // check if new position is out of bounce if so bring back
	  // and go other way
	  if (posX < 0)
	  {
		posX = 0;
		direction = true;
	
		// add the player images
		zombie_layer = layer_create(bounds);
		layer_set_update_proc(zombie_layer, zombie_layer_update_callback);
		layer_add_child(window_layer, zombie_layer);

		arr[zombieCount] = zombie_layer;
		zombieCount++;
	  }
	  else if (posX >= 144)
	  {
		posX = 144;
		direction = false;
	
		// add the player images
		zombie_layer = layer_create(bounds);
		layer_set_update_proc(zombie_layer, zombie_layer_update_callback);
		layer_add_child(window_layer, zombie_layer);

		arr[zombieCount] = zombie_layer;
		zombieCount++;
	  }// end if

	  // save the current poistion of the marker
	  markerPos = GPoint (posX, markerPos.y);
	  gpath_move_to(marker_path, markerPos);
  
	  // set the speed limit
	  if (speed < 15)
	    speed += 0.01;
	}
	  graphics_context_set_stroke_color(ctx, GColorBlack);
	  graphics_context_set_fill_color(ctx, GColorBlack);
	  gpath_draw_filled(ctx, marker_path);
	  gpath_draw_outline(ctx, marker_path);
}

static void update_bullet_layer(Layer *layer, GContext* ctx) {
if (!isGameOver)
{  
  if (isShooting)
  {
    int posX = bulletPos.x + bulletSpeed;

    // save the current poistion of the marker
    bulletPos = GPoint (posX, bulletPos.y);
    gpath_move_to(bullet_path, bulletPos);

    if (arr[0] && zombieCount > 0)
    {
		Layer *layer = arr[0];
	    GRect bounds = layer_get_bounds(layer);
		GPoint center = grect_center_point(&bounds);
		
		if (posX > center.x)
		{
			// kill zombie
			isShooting = false;
			bulletPos = GPoint (10, 9);
			
			// move arry elements down
			for (int i = 1; i < zombieCount; i++)
			{
				arr[i-1] = arr[i];
			}
			
			zombieCount--;
			
			layer_remove_from_parent(layer);
			
			count++;
			
			static char body_text[50];
			snprintf(body_text, sizeof(body_text), "Dead Dead %u", count);
			text_layer_set_text(text_layer, body_text);
		}
    }
  }
  else 
  {
	// save the current poistion of the marker
    bulletPos = GPoint (bulletPos.x, bulletPos.y);
    gpath_move_to(bullet_path, bulletPos);
  }
}
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_fill_color(ctx, GColorBlack);
  gpath_draw_filled(ctx, bullet_path);
  gpath_draw_outline(ctx, bullet_path);
}

static void timer_callback(void *context) {
  layer_mark_dirty(square_layer);
  layer_mark_dirty(bullseye_layer);
  layer_mark_dirty(marker_layer);
  layer_mark_dirty(bullet_layer);
  layer_mark_dirty(player_layer);
  layer_mark_dirty(zombie_layer);
  layer_mark_dirty(background_layer);

  const uint32_t timeout_ms = 50;
  timer = app_timer_register(timeout_ms, timer_callback, NULL);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {

	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	  
	if (!isGameOver)
	{ 

	  GPoint center = grect_center_point(&bounds);

	  if (markerPos.x >= center.x + markerMin && markerPos.x <= center.x + markerMax)
	  {
		bulletPos = GPoint (10, 9);
		vibes_short_pulse();
		isShooting = true;
	  }
	}
	
	else {
		// Reset the game
		isGameOver = false;
		text_layer_set_text(msg_layer, "Press Select Button");

		// Reset variables
		markerPos = GPoint (75, 75);
		bulletPos = GPoint (10, 9);

		direction = true;
		speed = 5;
		bulletSpeed = 10;
		count = 0;
		
		static char body_text[50];
		snprintf(body_text, sizeof(body_text), "Dead Dead %u", count);

		// Clear the zombie array and amount of zombies
		for (int i = 0; i < zombieCount; i++) {
			layer_destroy(arr[i]);
			arr[i] = NULL;
		}

		zombieCount = 0;
		zombie_layer = layer_create(bounds);
	  	layer_set_update_proc(zombie_layer, zombie_layer_update_callback);
	  	layer_add_child(window_layer, zombie_layer);

	  	arr[zombieCount] = zombie_layer;
	  	zombieCount++;
	}
	

}
/*
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Up");
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Down");
}
*/
static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  //window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  //window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void init(void) {
  int highScore = persist_read_int(HIGH_SCORE_KEY);
	
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // add the player images
  background_layer = layer_create(bounds);
  layer_set_update_proc(background_layer, background_layer_update_callback);
  layer_add_child(window_layer, background_layer);

  background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
  
  // add the player images
  player_layer = layer_create(bounds);
  layer_set_update_proc(player_layer, player_layer_update_callback);
  layer_add_child(window_layer, player_layer);

  player_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DUDE);

  // add the player images
  zombie_layer = layer_create(bounds);
  layer_set_update_proc(zombie_layer, zombie_layer_update_callback);
  layer_add_child(window_layer, zombie_layer);
  
  arr[zombieCount] = zombie_layer;
  zombieCount++;

  zombie_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ZOMBIE);

  square_layer = layer_create(bounds);
  layer_set_update_proc(square_layer, update_square_layer);
  layer_add_child(window_layer, square_layer);

  square_path = gpath_create(&SQUARE_POINTS);
  gpath_move_to(square_path, grect_center_point(&bounds));

  bullseye_layer = layer_create(bounds);
  layer_set_update_proc(bullseye_layer, update_bullseye_layer);
  layer_add_child(window_layer, bullseye_layer);

  bullseye_path = gpath_create(&BULLSEYE_POINTS);
  gpath_move_to(bullseye_path, grect_center_point(&bounds));

  marker_layer = layer_create(bounds);
  layer_set_update_proc(marker_layer, update_marker_layer);
  layer_add_child(window_layer, marker_layer);

  marker_path = gpath_create(&MARKER_POINTS);
  gpath_move_to(marker_path, grect_center_point(&bounds));

  bullet_layer = layer_create(bounds);
  layer_set_update_proc(bullet_layer, update_bullet_layer);
  layer_add_child(window_layer, bullet_layer);

  bullet_path = gpath_create(&BULLET_POINTS);
  gpath_move_to(bullet_path, grect_center_point(&bounds));

  text_layer = text_layer_create((GRect) { .origin = { 0, 62 }, .size = { bounds.size.w, 20 } });
  text_layer_set_text(text_layer, "Dead Dead");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));

  msg_layer = text_layer_create((GRect) { .origin = { 0, 85 }, .size = { bounds.size.w, 20 } });
  text_layer_set_text(msg_layer, "Press Select Button");
  text_layer_set_text_alignment(msg_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(msg_layer));

  high_score_layer = text_layer_create((GRect) { .origin = { 0, 109 }, .size = { bounds.size.w, 20 } });
  	// Display high score in text_layer
	static char buf[32];
	snprintf(buf, 32, "High Score: %u", highScore);
	text_layer_set_text(high_score_layer, buf);
  text_layer_set_text_alignment(high_score_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(high_score_layer));

  const bool animated = true;
  window_stack_push(window, animated);

  const uint32_t timeout_ms = 50;
  timer = app_timer_register(timeout_ms, timer_callback, NULL);

  markerPos = GPoint (75, 75);
  bulletPos = GPoint (10, 9);
}

static void deinit(void) {
  gpath_destroy(square_path);
  gpath_destroy(bullseye_path);
  gpath_destroy(marker_path);
  gpath_destroy(bullet_path);

  layer_destroy(square_layer);
  layer_destroy(bullseye_layer);
  layer_destroy(marker_layer);
  layer_destroy(bullet_layer);

  layer_destroy(player_layer);
  layer_destroy(zombie_layer);
  layer_destroy(background_layer);

  text_layer_destroy(text_layer);

  gbitmap_destroy(player_image);
  
  window_destroy(window);
}

int main(void) {
  init();

  app_event_loop();

  deinit();
}