#include "tracker_data.hpp"

using namespace std;

const int HEADER_HEIGHT = 18;
const int MAX_LIST_SIZE = 6;
const int LEFT_MARGIN = 4;
const int RIGHT_MARGIN = LEFT_MARGIN;
const int MAX_FREEZE_TIME = 60;
const int LONG_PRESS_STEP = 3;
const int RECEIVED_ELEMENTS_KEYMAP = -10;
const int RECEIVED_HOURS_KEYMAP = 1000;

static TrackingList* trackingList;

static Window* window;
static MenuLayer* menu_layer;
static GFont status_font;
static TextLayer* textLayers[MAX_LIST_SIZE][2];
static bool bluetoothLastState;

static time_t freezeTime;
static int changeTimePos;
static std::map<int, int> changeTimeAdds;

static const uint32_t tinyDuration[] = {100};
static VibePattern tinyVibe = { .durations = tinyDuration, .num_segments = 1 };
static const uint32_t longDurations[] = {400, 200, 500, 200, 500};
static VibePattern smallVibe = { .durations = longDurations, .num_segments = 1 };
static VibePattern longVibe = { .durations = longDurations, .num_segments = 3 };
static VibePattern veryLongVibe = { .durations = longDurations, .num_segments = 5 };

inline void serialize() {
	schar* buffer = trackingList->serialize();
	persist_write_data(0,  buffer, trackingList->getBinarySize());
	delete[] buffer;
}

inline void deserialize() {
	if (persist_exists(0)) {
		schar* buffer = new schar[trackingList->getBinarySize()];
		persist_read_data(0, buffer, trackingList->getBinarySize());
		trackingList->deserialize(buffer);
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
	snprintf(time[row], sizeof(time[row]), "%d:%02d", timeInSecs / (60 * 60), timeInSecs / 60 % 60);

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
	int selIndex = trackingList->getSelectedIndex();
	TrackingListMode mode = trackingList->getMode();
	time_t t = time(0L);
	struct tm* now = localtime(&t);
	static char time[6];
	snprintf(time, sizeof(time), "%d:%02d", now->tm_hour, now->tm_min);
	int timeInSecs = trackingList->totalTime(false);
	int accTimeInSecs = trackingList->totalTime();
	static char totalTime[13];
	if (timeInSecs != accTimeInSecs || mode == FREEZE_MODE)
		snprintf(totalTime, sizeof(totalTime), "%d:%02d/%d:%02d", timeInSecs / (60 * 60), timeInSecs / 60 % 60,
									  accTimeInSecs / (60 * 60), accTimeInSecs / 60 % 60);
	else
		snprintf(totalTime, sizeof(totalTime), "%d:%02d", timeInSecs / (60 * 60), timeInSecs / 60 % 60);

	GRect bounds = layer_get_bounds(cell_layer);
	GRect timeBounds = { LEFT_MARGIN, 0, bounds.size.w / 4 - LEFT_MARGIN, bounds.size.h };
	GRect totalTimeBounds = { bounds.size.w / 4, 0, 3 * bounds.size.w / 4 - RIGHT_MARGIN, bounds.size.h };

	if (mode == NORMAL_MODE || mode == FREEZE_MODE)
		graphics_context_set_fill_color(ctx, selIndex != NULL_V ? GColorJaegerGreen : GColorIslamicGreen);
	else if (mode == BUILD_BREAK_MODE)
		graphics_context_set_fill_color(ctx, selIndex != NULL_V ? GColorPictonBlue : GColorBlueMoon);
	graphics_fill_rect(ctx, bounds, 0, GCornersAll);
	graphics_draw_line(ctx, GPoint(0, bounds.size.h - 1), GPoint(bounds.size.w, bounds.size.h - 1));
	graphics_draw_text(ctx, time, status_font, timeBounds, GTextOverflowModeFill, GTextAlignmentLeft, NULL);

	if (mode == FREEZE_MODE && selIndex == NULL_V) {
		int digitShiftX = 83;
		if (changeTimePos == 1)
			digitShiftX = 93;
		else if (changeTimePos == 2)
			digitShiftX = 99;
		int digitShiftY = 15;
		int digitWidth = 4;
		graphics_context_set_stroke_color(ctx, GColorBlack);
		graphics_context_set_stroke_width(ctx, 1);
		graphics_context_set_antialiased(ctx, true);
		graphics_draw_line(ctx, GPoint(totalTimeBounds.origin.x + digitShiftX, digitShiftY),
					GPoint(totalTimeBounds.origin.x + digitShiftX + digitWidth, digitShiftY));
	}
	graphics_draw_text(ctx, totalTime, status_font, totalTimeBounds, GTextOverflowModeFill, GTextAlignmentRight, NULL);
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
				trackingList->restoreSelected();
			}
			break;
	}
	menu_layer_reload_data(menu_layer);
}

void selectClick(ClickRecognizerRef c, void*) {
	TrackingListMode mode = trackingList->getMode();
	int selIndex = trackingList->getSelectedIndex();
	switch(mode) {
		case NORMAL_MODE:
			if (selIndex == NULL_V) {
				trackingList->switchMode(BUILD_BREAK_MODE);
			}
			else {
				if (selIndex == trackingList->getActiveIndex())
					vibes_enqueue_custom_pattern(tinyVibe);
				trackingList->switchIndex();
			}
			break;
		case BUILD_BREAK_MODE:
			if (selIndex == NULL_V)
				trackingList->switchMode(NORMAL_MODE);
			else
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
	menu_layer_reload_data(menu_layer);
}

void longSelectClick(ClickRecognizerRef, void*) {
	int selIndex = trackingList->getSelectedIndex();
	switch(trackingList->getMode()) {
		case NORMAL_MODE:
			if (selIndex == NULL_V) {
				if (trackingList->totalTime(false) != 0) {
					serialize();
					trackingList->resetTime(false);
				}
				else {
					deserialize();
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
				trackingList->restoreSelected();
			}
			trackingList->breakPair();
			break;
		case FREEZE_MODE:
			trackingList->resetSelectedTime();
			trackingList->switchMode(NORMAL_MODE);
			break;
	}
	menu_layer_reload_data(menu_layer);
}

static void longUpClick(ClickRecognizerRef, void*) {
	int selIndex = trackingList->getSelectedIndex();
	switch(trackingList->getMode()) {
		case NORMAL_MODE:
			if (selIndex == NULL_V) {
				if (trackingList->totalTime() != 0) {
					if(trackingList->totalTime(false) != 0)
						serialize();
					trackingList->resetTime(true);
				}
				else {
					deserialize();
				}
			}
			else if (trackingList->getPreviousActiveIndex() != NULL_V && selIndex < LONG_PRESS_STEP) {
				trackingList->restorePreviousActive();
			}
			else {
				trackingList->decIndex(LONG_PRESS_STEP);
			}
			break;
		case BUILD_BREAK_MODE:
			if (selIndex == NULL_V)
				trackingList->buildAll();
			else
				trackingList->decIndex(LONG_PRESS_STEP);
			break;
		case FREEZE_MODE:
			freezeTime = time(0L);
			if (selIndex == NULL_V) {
				switch (changeTimePos) {
					case 0:
						trackingList->addTime(changeTimeAdds[changeTimePos] * 10);
						break;
					case 1:
						trackingList->addTime(changeTimeAdds[changeTimePos] * 3);
						break;
					case 2:
						trackingList->addTime(changeTimeAdds[changeTimePos] * 5);
						break;
				}
				menu_layer_reload_data(menu_layer);
				return;
			}
			trackingList->decIndex();
			break;

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
	int selIndex = trackingList->getSelectedIndex();
	TrackingListMode mode = trackingList->getMode();
	switch(trackingList->getMode()) {
		case NORMAL_MODE:
			if (selIndex == NULL_V) {
				trackingList->switchMode(FREEZE_MODE);
				freezeTime = time(0L);
				changeTimePos = 0;
			}
			else if (trackingList->getPreviousActiveIndex() != NULL_V && selIndex >= trackingList->size() - LONG_PRESS_STEP) {
				trackingList->restorePreviousActive();
			}
			else {
				trackingList->incIndex(LONG_PRESS_STEP);
			}
			break;
		case BUILD_BREAK_MODE:
			if (selIndex == NULL_V)
				trackingList->breakAll();
			else
				trackingList->incIndex(LONG_PRESS_STEP);
			break;
		case FREEZE_MODE:
			freezeTime = time(0L);
			if (selIndex == NULL_V) {
				switch (changeTimePos) {
					case 0:
						trackingList->subTime(changeTimeAdds[changeTimePos] * 10);
						break;
					case 1:
						trackingList->subTime(changeTimeAdds[changeTimePos] * 3);
						break;
					case 2:
						trackingList->subTime(changeTimeAdds[changeTimePos] * 5);
						break;
				}
				menu_layer_reload_data(menu_layer);
				return;
			}
			trackingList->incIndex();
			break;
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

void handleTick(tm* tickTime, TimeUnits units) {
	int elementTime = trackingList->updateTime();
	bool elementMinuteChanged = elementTime % 60 == 0 && elementTime > 0;
	if (units & MINUTE_UNIT || elementMinuteChanged) {
		if (elementMinuteChanged) {
			int accTimeInSecs = trackingList->totalTime();
			int timeInSecs = trackingList->totalTime(false);
			if (accTimeInSecs > 60 && accTimeInSecs / 60 % (trackingList->getTotalAccHours() * 60) == 0)
				vibes_enqueue_custom_pattern(veryLongVibe);
			else if (timeInSecs > 60 && (timeInSecs / 60 % (trackingList->getTotalHours() * 60) == 0 ||
						     accTimeInSecs / 60 % (trackingList->getTotalHours() * 60) == 0))
				vibes_enqueue_custom_pattern(longVibe);
			else if (elementTime % (60 * 60) == 0)
				vibes_enqueue_custom_pattern(smallVibe);
		}

		if (trackingList->getMode() == FREEZE_MODE && mktime(tickTime) - freezeTime >= MAX_FREEZE_TIME)
			trackingList->switchMode(NORMAL_MODE);
		menu_layer_reload_data(menu_layer);
	}
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
	for(auto i = -1; (tuple = dict_find(received, RECEIVED_ELEMENTS_KEYMAP * (2 * i + 1))) != NULL || persist_exists(2 * i + 1); --i) {
		if (tuple) {
			char* title = (char*)tuple->value;
			int* priority = (int*)dict_find(received, RECEIVED_ELEMENTS_KEYMAP * 2 * i)->value;
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

	int* totalHours = (int*)dict_find(received, RECEIVED_HOURS_KEYMAP * 1)->value;
	persist_write_int(RECEIVED_HOURS_KEYMAP * 1, *totalHours);
	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "total hours: %d", *totalHours);
	int* accTotalHours = (int*)dict_find(received, RECEIVED_HOURS_KEYMAP * 2)->value;
	persist_write_int(RECEIVED_HOURS_KEYMAP * 2, *accTotalHours);
	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "total accumulated hours: %d", *accTotalHours);

	delete trackingList;
	trackingList = new TrackingList(elements, pairs, *totalHours, *accTotalHours);
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
	changeTimeAdds = { { 0, 60 * 60 }, { 1, 10 * 60 }, { 2, 1 * 60 } };

	PairMap pairs = getPairs();
	vector<BaseTracking*> elements = getElements();
	if (persist_exists(RECEIVED_HOURS_KEYMAP)) {
		int totalHours = persist_read_int(RECEIVED_HOURS_KEYMAP * 1);
		int accTotalHours = persist_read_int(RECEIVED_HOURS_KEYMAP * 2);
		trackingList = new TrackingList(elements, pairs, totalHours, accTotalHours);
	}
	else {
		trackingList = new TrackingList(elements, pairs);
	}

	status_font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
	window = window_create();
	window_set_click_config_provider(window, click_config_provider);

	static WindowHandlers windowHandlers;
	windowHandlers.load = window_load;
	windowHandlers.unload = window_unload;
	window_set_window_handlers(window, windowHandlers);

	deserialize();
	window_stack_push(window, true);

	app_message_register_inbox_received(handle_msg_received);
	app_message_open(app_message_inbox_size_maximum(), APP_MESSAGE_OUTBOX_SIZE_MINIMUM);

	bluetoothLastState = bluetooth_connection_service_peek();
	bluetooth_connection_service_subscribe(handleBluetooth);
	tick_timer_service_subscribe(SECOND_UNIT, handleTick);
}

static void deinit(void) {
	tick_timer_service_unsubscribe();
	serialize();
	window_destroy(window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
	return 0;
}
