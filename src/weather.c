#include <pebble.h>

Window *window, *temp;
TextLayer *date_layer, *location_layer, *temp_layer, *time_layer, *battery_layer;

char date_buffer[32], location_buffer[64], temp_buffer[32], time_buffer[32];

enum {
	KEY_LOCATION = 0,
	temperature = 3,
};

void process_tuple(Tuple *t) {
	//Get key
	int key = t->key;

	//Get integer value, if present
	int value = t->value->int32;

	//Get string value, if present
	char string_value[32];
	strcpy(string_value, t->value->cstring);

	//Decide what to do
	switch(key) {
		case KEY_LOCATION:
			//Location received
			snprintf(location_buffer, sizeof("Location: couldbereallylongname"), "%s", string_value);
			text_layer_set_text(location_layer, (char*) &location_buffer);
			break;
		case temperature:
			//Temperature received
			snprintf(temp_buffer, sizeof("Temperature: XX \u00B0C"), "%d\u00B0F", value);
			text_layer_set_text(temp_layer, (char*) &temp_buffer);
			break;
	}

	//Set time this update came in
	time_t temp = time(NULL);
	struct tm *tm = localtime(&temp);
	strftime(time_buffer, sizeof("Last updated: XX:XX"), "%I:%M", tm);
	text_layer_set_text(time_layer, (char*) &time_buffer);
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
	(void) context;

	//Get data
	Tuple *t = dict_read_first(iter);
	if(t)
	{
		process_tuple(t);
	}

	//Get next
	while(t != NULL)
	{
		t = dict_read_next(iter);
		if(t)
		{
			process_tuple(t);
		}
	}
}

static TextLayer* init_text_layer(GRect location, GColor colour, GColor background, const char *res_id, GTextAlignment alignment) {
	TextLayer *layer = text_layer_create(location);
	text_layer_set_text_color(layer, colour);
	text_layer_set_background_color(layer, background);
	text_layer_set_font(layer, fonts_get_system_font(res_id));
	text_layer_set_text_alignment(layer, alignment);

	return layer;
}

static void handle_second_tick(struct tm* tick_time, TimeUnits units_changed) {

    static char time_text[] = "00:00";
    static char date_text[] = "Mon, Jan 31";

    strftime(time_text, sizeof(time_text), "%r", tick_time);
    text_layer_set_text(time_layer, time_text);

    strftime(date_text, sizeof(date_text), "%a, %b %d", tick_time);
    text_layer_set_text(date_layer, date_text);
}

static void handle_battery(BatteryChargeState charge_state) {
  static char battery_text[15];

  if (charge_state.is_charging) {
    snprintf(battery_text, sizeof(battery_text), "charging");
  } else {
    snprintf(battery_text, sizeof(battery_text), "Batt: %d%%", charge_state.charge_percent);
  }
  text_layer_set_text(battery_layer, battery_text);
}

void window_load(Window *window) {
  date_layer = init_text_layer(GRect(0, 47, 144, 30), GColorBlack, GColorWhite, "FONT_KEY_GOTHIC_28_BOLD", GTextAlignmentLeft);
	text_layer_set_text(date_layer, "Date N/A");
  text_layer_set_font(date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(date_layer));
  text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);

  battery_layer = text_layer_create(GRect(0, 76, 144, 30));
  text_layer_set_text(battery_layer, "Batt: ");
  text_layer_set_text_color(battery_layer, GColorBlack);
  text_layer_set_background_color(battery_layer, GColorWhite);
  text_layer_set_font(battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(battery_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(battery_layer));

  temp_layer = text_layer_create(GRect(0, 111, 144, 50));
  text_layer_set_text(temp_layer, "N/A");
  text_layer_set_text_color(temp_layer, GColorWhite);
  text_layer_set_background_color(temp_layer, GColorBlack);
  text_layer_set_font(temp_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(temp_layer));
  text_layer_set_text_alignment(temp_layer, GTextAlignmentCenter);
}

void window_unload(Window *window) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();

  text_layer_destroy(date_layer);
	text_layer_destroy(temp_layer);
  text_layer_destroy(location_layer);
}

void send_int(uint8_t key, uint8_t cmd) {
	DictionaryIterator *iter;
 	app_message_outbox_begin(&iter);

 	Tuplet value = TupletInteger(key, cmd);
 	dict_write_tuplet(iter, &value);

 	app_message_outbox_send();
}

void tick_callback(struct tm *tick_time, TimeUnits units_changed) {
	//Every five minutes
	if(tick_time->tm_min % 1 == 0)
	{
		//Send an arbitrary message, the response will be handled by in_received_handler()
		send_int(1, 1);
	}
}

void init() {
	window = window_create();
  window_set_background_color(window, GColorBlack);
	WindowHandlers handlers = {
		.load = window_load,
		.unload = window_unload
	};
	window_set_window_handlers(window, handlers);
  window_set_fullscreen(window, true);

	//Register AppMessage events
	app_message_register_inbox_received(in_received_handler);
	app_message_open(512, 512);		//Large input and output buffer sizes

	window_stack_push(window, true);

  // Init the text layer used to show the time
    time_layer = text_layer_create(GRect(0, 0, 144 /* width */, 50 /* height */));
    text_layer_set_text_color(time_layer, GColorBlack);
    text_layer_set_background_color(time_layer, GColorWhite);
    text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
    text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);

    // Ensures time is displayed immediately (will break if NULL tick event accessed).
    // (This is why it's a good idea to have a separate routine to do the update itself.)
    time_t now = time(NULL);
    struct tm *current_time = localtime(&now);
    handle_battery(battery_state_service_peek());
    handle_second_tick(current_time, SECOND_UNIT);
    tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);
    battery_state_service_subscribe(&handle_battery);

    layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_layer));
}

void deinit() {
  text_layer_destroy(battery_layer);
  text_layer_destroy(time_layer);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
