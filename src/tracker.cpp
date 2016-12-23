#include "tracker_data.hpp"

using namespace std;

const int HEADER_HEIGHT = 18;
const int MAX_LIST_SIZE = 6;
const int LEFT_MARGIN = 4;
const int RIGHT_MARGIN = LEFT_MARGIN;
const int MINUTES_Q = 60;
const int MAX_FREEZE_TIME = 60;

static TrackingList* trackingList;

static Window* window;
static MenuLayer* menu_layer;
static GFont status_font;
static TextLayer* textLayers[MAX_LIST_SIZE][2];
static bool bluetoothLastState;

static time_t freezeTime;
static int changeTimePos;
static std::map<int, int> changeTimeAdds;

static const uint32_t shortDuration[] = {100};
static VibePattern tinyVibe = { .durations = shortDuration, .num_segments = 1 };

inline void serialize() {
	schar* buffer = trackingList->serialize();
	persist_write_data(0,  buffer, trackingList->getBinarySize());
	delete[] buffer;
}

inline void restore() {
	if (persist_exists(0)) {
		schar* buffer = new schar[trackingList->getBinarySize()];
		persist_read_data(0, buffer, trackingList->getBinarySize());
		trackingList->restore(buffer);
		delete[] buffer;
	}
}

inline GFont getFont(bool big, bool selected) {
	if (big)
		return selected ? fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD) : fonts_get_system_font(FONT_KEY_GOTHIC_24);
	return selected ? fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD) : fonts_get_system_font(FONT_KEY_GOTHIC_18);
}

uint16_t getNumRows(MenuLayer* menu_layer, uint16_t cell_index, void*) {
	return trackingList->size();
}

int16_t getCellHeight(MenuLayer* menu_layer, MenuIndex* cell_index, void*) {
	int16_t baseHeight = (layer_get_bounds(menu_layer_get_layer(menu_layer)).size.h - HEADER_HEIGHT) / trackingList->totalHeight();
	return trackingList->at(cell_index->row)->getHeight() * baseHeight;
}

int16_t getHeaderHeight(MenuLayer* menu_layer, uint16_t section_index, void*) {
	return HEADER_HEIGHT;
}

void drawRow(GContext* ctx, const Layer* cell_layer, MenuIndex* cell_index, void*) {
	int row = cell_index->row;
	int selIndex = trackingList->getSelectedIndex();
	bool isBig = trackingList->at(row)->getHeight() > 1;
	TrackingListMode mode = trackingList->getMode();
	char const* name = trackingList->at(row)->getName();
	bool isActive = trackingList->getActiveIndex() == row;
	int timeInSecs = trackingList->at(row)->getTime();
	static char time[MAX_LIST_SIZE][6];
	snprintf(time[row], sizeof(time[row]), "%d:%02d", timeInSecs / (60 * MINUTES_Q), timeInSecs / MINUTES_Q % 60);

	GRect bounds = layer_get_bounds(cell_layer);
	GRect nameBounds = { LEFT_MARGIN, 0, bounds.size.w * 2 / 3 - LEFT_MARGIN, bounds.size.h };
	GRect timeBounds = { bounds.size.w * 2 / 3, 0, bounds.size.w / 3 - RIGHT_MARGIN, bounds.size.h };

	if (selIndex == row)
		graphics_context_set_fill_color(ctx, mode == BUILD_BREAK_MODE ? GColorPictonBlue : GColorJaegerGreen);
	else
		graphics_context_set_fill_color(ctx, row % 2 == 1 ? GColorRajah : GColorPastelYellow);
	graphics_fill_rect(ctx, bounds, 0, GCornersAll);

	graphics_context_set_text_color(ctx, GColorBlack);
	GFont nameFont = getFont(isBig, isActive);
	graphics_draw_text(ctx, name, nameFont, nameBounds, GTextOverflowModeFill, GTextAlignmentLeft, NULL);

	GFont timeFont;
	if (mode == FREEZE_MODE && selIndex == row) {
		timeFont = getFont(isBig, true);
		int digitShiftX = isBig ? 11 : 17;
		if (changeTimePos == 1)
			digitShiftX = isBig ? 25 : 29;
		else if (changeTimePos == 2)
			digitShiftX = isBig ? 35 : 37;
		int digitShiftY = isBig ? 26 : 20;
		int digitWidth = isBig ? 7 : 5;
		graphics_context_set_stroke_color(ctx, GColorBlack);
		graphics_context_set_stroke_width(ctx, 2);
		graphics_context_set_antialiased(ctx, true);
		graphics_draw_line(ctx, GPoint(timeBounds.origin.x + digitShiftX, digitShiftY), 
					GPoint(timeBounds.origin.x + digitShiftX + digitWidth, digitShiftY));
	}
	else {
		timeFont = nameFont;
	}
	graphics_draw_text(ctx, time[row], timeFont, timeBounds, GTextOverflowModeFill, GTextAlignmentRight, NULL);
}

void drawHeader(GContext* ctx, const Layer* cell_layer, uint16_t, void*) {
	TrackingListMode mode = trackingList->getMode();
	time_t t = time(0L);
	struct tm* now = localtime(&t);
	static char time[6];
	snprintf(time, sizeof(time), "%d:%02d", now->tm_hour, now->tm_min);
	int timeInSecs = trackingList->totalTime();
	static char totalTime[6];
	snprintf(totalTime, sizeof(totalTime), "%d:%02d", timeInSecs / (MINUTES_Q * 60), timeInSecs / MINUTES_Q % 60);

	GRect bounds = layer_get_bounds(cell_layer);
	GRect nameBounds = { LEFT_MARGIN, 0, bounds.size.w * 3 / 4 - LEFT_MARGIN, bounds.size.h };
	GRect timeBounds = { bounds.size.w * 3 / 4, 0, bounds.size.w / 4 - RIGHT_MARGIN, bounds.size.h };

	if (mode == NORMAL_MODE || mode == FREEZE_MODE)
		graphics_context_set_fill_color(ctx, trackingList->getSelectedIndex() != NULL_V ? GColorJaegerGreen : GColorIslamicGreen);
	else if (mode == BUILD_BREAK_MODE)
		graphics_context_set_fill_color(ctx, trackingList->getSelectedIndex() != NULL_V ? GColorPictonBlue : GColorBlueMoon);
	graphics_fill_rect(ctx, bounds, 0, GCornersAll);
	graphics_draw_line(ctx, GPoint(0, bounds.size.h - 1), GPoint(bounds.size.w, bounds.size.h - 1));
	graphics_draw_text(ctx, time, status_font, nameBounds, GTextOverflowModeFill, GTextAlignmentLeft, NULL);
	graphics_draw_text(ctx, totalTime, status_font, timeBounds, GTextOverflowModeFill, GTextAlignmentRight, NULL);
}

void backClick(ClickRecognizerRef, void*) {
	int selIndex = trackingList->getSelectedIndex();
	switch(trackingList->getMode()) {
		case FREEZE_MODE:
			if (changeTimePos > 0) {
				freezeTime = time(0L);
				--changeTimePos;
			}
			else {
				trackingList->switchMode(NORMAL_MODE);
			}
			break;
		case NORMAL_MODE:
			if (selIndex != NULL_V) {
				trackingList->resetIndex();
				break;
			}
			else {
				window_stack_pop_all(true);
				return;
			}
		case BUILD_BREAK_MODE:
			if (selIndex != NULL_V) {
				trackingList->resetIndex();
			}
			else {
				if (trackingList->getActiveIndex() != NULL_V)
					trackingList->switchMode(NORMAL_MODE);
				trackingList->buildPair();
				trackingList->restoreIndex();
			}
			break;
	}
	menu_layer_reload_data(menu_layer);
}

void selectClick(ClickRecognizerRef c, void*) {
	TrackingListMode mode = trackingList->getMode();
	int selIndex = trackingList->getSelectedIndex();
	if (selIndex == NULL_V) {
		trackingList->switchMode(mode == NORMAL_MODE ? BUILD_BREAK_MODE : NORMAL_MODE);
	}
	else {
		switch(mode) {
			case NORMAL_MODE:
				if (selIndex == trackingList->getActiveIndex()) {
					vibes_enqueue_custom_pattern(tinyVibe);
				}
			case BUILD_BREAK_MODE:
				trackingList->switchIndex();
				break;
			case FREEZE_MODE:
				if (changeTimePos < 2) {
					freezeTime = time(0L);
					++changeTimePos;
				}
				else {
					trackingList->switchMode(NORMAL_MODE);
				}
				break;
		}
	}
	menu_layer_reload_data(menu_layer);
}

void longSelectClick(ClickRecognizerRef, void*) {
	int selIndex = trackingList->getSelectedIndex();
	switch(trackingList->getMode()) {
		case NORMAL_MODE:
			if (selIndex == NULL_V) {
				if (trackingList->totalTime() != 0) {
					serialize();
					trackingList->resetTime(true);
				}
				else {
					restore();
				}
			}
			else {
				trackingList->switchMode(FREEZE_MODE);
				freezeTime = time(0L);
				changeTimePos = 0;
			}
			break;
		case BUILD_BREAK_MODE:
			if (selIndex == NULL_V && trackingList->getActiveIndex() != NULL_V) {
				trackingList->switchMode(NORMAL_MODE);
				trackingList->restoreIndex();
			}
			trackingList->breakPair();
			break;
		case FREEZE_MODE:
			trackingList->resetTime(false);
			trackingList->switchMode(NORMAL_MODE);
			break;
	}
	menu_layer_reload_data(menu_layer);
}

static void longUpClick(ClickRecognizerRef, void*) {
	if (trackingList->getMode() != FREEZE_MODE) {
		trackingList->decIndex(3);
	}
	else {
		freezeTime = time(0L);
		trackingList->decIndex();
	}
	menu_layer_set_selected_index(menu_layer, MenuIndex(0, trackingList->getSelectedIndex()), MenuRowAlignNone, true);
}

static void upClick(ClickRecognizerRef, void*) {
	if (trackingList->getMode() != FREEZE_MODE) {
		trackingList->decIndex();
		menu_layer_set_selected_index(menu_layer, MenuIndex(0, trackingList->getSelectedIndex()), MenuRowAlignNone, true);
	}
	else {
		trackingList->addTime(changeTimeAdds[changeTimePos]);
		freezeTime = time(0L);
		menu_layer_reload_data(menu_layer);
	}
}

static void longDownClick(ClickRecognizerRef, void*) {
	if (trackingList->getMode() != FREEZE_MODE) {
		trackingList->incIndex(3);
	}
	else {
		freezeTime = time(0L);
		trackingList->incIndex();
	}
	menu_layer_set_selected_index(menu_layer, MenuIndex(0, trackingList->getSelectedIndex()), MenuRowAlignNone, true);
}

static void downClick(ClickRecognizerRef, void*) {
	if (trackingList->getMode() != FREEZE_MODE) {
		trackingList->incIndex();
		menu_layer_set_selected_index(menu_layer, MenuIndex(0, trackingList->getSelectedIndex()), MenuRowAlignNone, true);
	}
	else {
		trackingList->subTime(changeTimeAdds[changeTimePos]);
		freezeTime = time(0L);
		menu_layer_reload_data(menu_layer);
	}
}

void handleBluetooth(bool connected) {
	if (connected != bluetoothLastState)
		vibes_short_pulse();
	bluetoothLastState = connected;
}

void handleTick(tm* tickTime, TimeUnits) {
	int elementTime = trackingList->updateTime();
	if (elementTime >= 60 && elementTime / 60 % 60 == 0)
		vibes_short_pulse();
	if (trackingList->getMode() == FREEZE_MODE && mktime(tickTime) - freezeTime >= MAX_FREEZE_TIME)
		trackingList->switchMode(NORMAL_MODE);
	menu_layer_reload_data(menu_layer);
}

static void window_load(Window* window) {
	Layer* window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);

	menu_layer = menu_layer_create((GRect) {
		.origin = GPointZero,
		.size = bounds.size
	});

	static MenuLayerCallbacks menuLayerCallbacks;
	menuLayerCallbacks.get_num_rows = getNumRows;
	menuLayerCallbacks.get_cell_height = getCellHeight;
	menuLayerCallbacks.get_header_height = getHeaderHeight;
	menuLayerCallbacks.draw_row = drawRow;
	menuLayerCallbacks.draw_header = drawHeader;
	menu_layer_set_callbacks(menu_layer, trackingList, menuLayerCallbacks);

	layer_add_child(window_layer, menu_layer_get_layer(menu_layer));
}

static void window_unload(Window* window) {
	menu_layer_destroy(menu_layer);	
}

static void click_config_provider(void*) {
	window_single_click_subscribe(BUTTON_ID_BACK, backClick);
	window_single_click_subscribe(BUTTON_ID_SELECT, selectClick);
	window_long_click_subscribe(BUTTON_ID_SELECT, 500, longSelectClick, NULL);
	window_single_click_subscribe(BUTTON_ID_UP, upClick);
	window_long_click_subscribe(BUTTON_ID_UP, 300, longUpClick, NULL);
	window_single_click_subscribe(BUTTON_ID_DOWN, downClick);
	window_long_click_subscribe(BUTTON_ID_DOWN, 300, longDownClick, NULL);
}

static void handle_msg_received(DictionaryIterator *received, void*) {
	Tuple* tuple;
	PairMap pairs;
	for(auto i = 1; (tuple = dict_find(received, 2 * i - 1)) != NULL || persist_exists(2 * i - 1); ++i) {
		if (tuple) {
			char* key = (char*)tuple->value;
			char* title = (char*)dict_find(received, 2 * i)->value;
			app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "%s: %s", key, title);
			pairs.insert(pair<char*, char*>(key, title));

			persist_write_string(2 * i - 1, key);
			persist_write_string(2 * i, title);
		}
		else {
			persist_delete(2 * i - 1);
			persist_delete(2 * i);
		}
	}

	vector<BaseTracking*> elements;
	for(auto i = -1; (tuple = dict_find(received, 2 * i + 1)) != NULL || persist_exists(2 * i + 1); --i) {
		if (tuple) {
			char* title = (char*)tuple->value;
			int* priority = (int*)dict_find(received, 2 * i)->value;
			app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "%s: %d", title, *priority);
			elements.push_back(new TrackingElement(title, *priority));

			persist_write_string(2 * i + 1, title);
			persist_write_int(2 * i, *priority);
		}
		else {
			persist_delete(2 * i + 1);
			persist_delete(2 * i);
		}
	}
	delete trackingList;
	trackingList = new TrackingList(elements, pairs);
	persist_delete(0);

	menu_layer_reload_data(menu_layer);
}

inline PairMap getPairs(void) {
	PairMap pairs;
	for(auto i = 1; persist_exists(2 * i - 1); ++i) {
		int key_size = persist_get_size(2 * i - 1);
		int value_size = persist_get_size(2 * i);
		char* key = new char[key_size];
		char* value = new char[value_size];
		persist_read_string(2 * i - 1, key, key_size);
		persist_read_string(2 * i, value, value_size);
		pairs.insert(pair<char*, char*>(key, value));
		delete[] key;
		delete[] value;
	}

	if (pairs.empty()) {
		pairs.insert(pair<char*, char*>("hardsimple", "work"));
		pairs.insert(pair<char*, char*>("overviewoptimization", "additional"));
		pairs.insert(pair<char*, char*>("workeducation", "main"));
		pairs.insert(pair<char*, char*>("additionaldistractions", "secondary"));
	}
	return pairs;
}

inline vector<BaseTracking*> getElements(void) {
	vector<BaseTracking*> elements;
	for(auto i = -1; persist_exists(2 * i + 1); --i) {
		int title_size = persist_get_size(2 * i + 1);
		char* title = new char[title_size];
		persist_read_string(2 * i + 1, title, title_size);
		int priority = persist_read_int(2 * i);
		elements.push_back(new TrackingElement(title, priority));
		delete[] title;
	}
	if (elements.empty()) {
		elements = vector<BaseTracking*> {
			new TrackingElement("hard", 0),
			new TrackingElement("simple", 0),
			new TrackingElement("education", 1),
			new TrackingElement("overview", 2),
			new TrackingElement("optimization", 2),
			new TrackingElement("distractions", 3)
		};
	}
	return elements;
}

static void init(void) {
	changeTimeAdds = { { 0, 60 * MINUTES_Q }, { 1, 10 * MINUTES_Q }, { 2, 1 * MINUTES_Q } };

	PairMap pairs = getPairs();
	vector<BaseTracking*> elements = getElements();
	trackingList = new TrackingList(elements, pairs);

	status_font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
	window = window_create();
	window_set_click_config_provider(window, click_config_provider);

	static WindowHandlers windowHandlers;
	windowHandlers.load = window_load;
	windowHandlers.unload = window_unload;
	window_set_window_handlers(window, windowHandlers);

	restore();
	window_stack_push(window, true);

	app_message_register_inbox_received(handle_msg_received);
	app_message_open(app_message_inbox_size_maximum(), APP_MESSAGE_OUTBOX_SIZE_MINIMUM);

	bluetoothLastState = bluetooth_connection_service_peek();
	bluetooth_connection_service_subscribe(handleBluetooth);
	tick_timer_service_subscribe(MINUTE_UNIT, handleTick);
}

static void deinit(void) {
	serialize();
	window_destroy(window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
	return 0;
}
