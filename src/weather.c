#include <pebble.h>
 
Window* window;
TextLayer *date_layer, *location_layer, *temperature_layer, *time_layer;

char date_buffer[32], location_buffer[64], temperature_buffer[32], time_buffer[32];

enum {
	KEY_LOCATION = 0,
	KEY_TEMPERATURE = 1,
};

void process_tuple(Tuple *t)
{
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
		case KEY_TEMPERATURE:
			//Temperature received
			snprintf(temperature_buffer, sizeof("Temperature: XX \u00B0C"), "%d\u00B0F", value);
			text_layer_set_text(temperature_layer, (char*) &temperature_buffer);
			break;
	}
  
	//Set time this update came in
	time_t temp = time(NULL);	
	struct tm *tm = localtime(&temp);
	strftime(time_buffer, sizeof("Last updated: XX:XX"), "%I:%M", tm);
	text_layer_set_text(time_layer, (char*) &time_buffer);
}

static void in_received_handler(DictionaryIterator *iter, void *context) 
{
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

static TextLayer* init_text_layer(GRect location, GColor colour, GColor background, const char *res_id, GTextAlignment alignment)
{
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


void window_load(Window *window)
{
  date_layer = init_text_layer(GRect(0, 47, 144, 30), GColorBlack, GColorWhite, "FONT_KEY_GOTHIC_28_BOLD", GTextAlignmentLeft);
	text_layer_set_text(date_layer, "Date N/A");
  text_layer_set_font(date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(date_layer));
  text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
  
  location_layer = init_text_layer(GRect(0, 76, 144, 30), GColorBlack, GColorWhite, "FONT_KEY_GOTHIC_28_BOLD", GTextAlignmentLeft);
	text_layer_set_text(location_layer, "Location");
  text_layer_set_font(location_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(location_layer));
  text_layer_set_text_alignment(location_layer, GTextAlignmentCenter);

	temperature_layer = init_text_layer(GRect(0, 109, 144, 75), GColorWhite, GColorBlack, "FONT_KEY_GOTHIC_28_BOLD", GTextAlignmentLeft);
	text_layer_set_text(temperature_layer, "N/A");
  text_layer_set_font(temperature_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(temperature_layer));
  text_layer_set_text_alignment(temperature_layer, GTextAlignmentCenter);
}
 
void window_unload(Window *window)
{	
  text_layer_destroy(date_layer);
	text_layer_destroy(location_layer);
	text_layer_destroy(temperature_layer);
	text_layer_destroy(time_layer);
}

void send_int(uint8_t key, uint8_t cmd)
{
	DictionaryIterator *iter;
 	app_message_outbox_begin(&iter);
 	
 	Tuplet value = TupletInteger(key, cmd);
 	dict_write_tuplet(iter, &value);
 	
 	app_message_outbox_send();
}

void tick_callback(struct tm *tick_time, TimeUnits units_changed)
{
	//Every five minutes
	if(tick_time->tm_min % 5 == 0)
	{
		//Send an arbitrary message, the response will be handled by in_received_handler()
		send_int(5, 5);
	}
}
 
void init()
{
	window = window_create();
  window_set_background_color(window, GColorBlack);
	WindowHandlers handlers = {
		.load = window_load,
		.unload = window_unload
	};
	window_set_window_handlers(window, handlers);

	//Register AppMessage events
	app_message_register_inbox_received(in_received_handler);					 
	app_message_open(512, 512);		//Large input and output buffer sizes

	//Register to receive minutely updates
	tick_timer_service_subscribe(MINUTE_UNIT, tick_callback);

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
    handle_second_tick(current_time, SECOND_UNIT);
    tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);
    
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_layer));
}
 
void deinit()
{
	tick_timer_service_unsubscribe();

	window_destroy(window);
}
 
int main(void)
{
	init();
	app_event_loop();
	deinit();
}