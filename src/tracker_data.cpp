#include "tracker_data.hpp"
#include <numeric>

const int TrackingList::HEADER_SIZE = 11;

using namespace std;

char const* BaseTracking::getName() const {
	return name;
}

int BaseTracking::getTime() const {
	return time;
}

TrackingElement::TrackingElement(char* name, int priority) {
	this->name = new char[strlen(name) + 1];
	strcpy(this->name, name);
	this->priority = priority;
}

TrackingElement::~TrackingElement() {
	delete[] name;
}

int TrackingElement::getPriority() const {
	return priority;
}

int TrackingElement::getHeight() const {
	return 1;
}

TrackingPair::TrackingPair(char* name, BaseTracking* element1, BaseTracking* element2) {
	this->name = name;
	this->element1 = element1;
	this->element2 = element2;
	this->time = element1->getTime() + element2->getTime();
}

int TrackingPair::getPriority() const {
	return min(element1->getPriority(), element2->getPriority());
}

int TrackingPair::getHeight() const {
	return element1->getHeight() + element2->getHeight();
}

TrackingList::TrackingList(std::vector<BaseTracking*> elements, PairMap& pairs) : vector<BaseTracking*>(elements) {
	this->possiblePairs = pairs;
}

TrackingList::TrackingList(std::vector<BaseTracking*> elements, PairMap& pairs, int totalHours, int totalAccHours) : TrackingList(elements, pairs) {
	this->totalHours = totalHours;
	this->totalAccHours = totalAccHours;
}

TrackingList::~TrackingList() {
	for(int i = 0; i < size(); ++i) {
		while(at(i)->getHeight() > 1)
			breakPair(i);
	}
	for_each(this->begin(), this->end(), [](BaseTracking* e) {delete e;});
	for_each(possiblePairs.begin(), possiblePairs.end(), [](std::pair<char*, char*> e) {
		delete[] e.first;
		delete[] e.second;
	});
}

int TrackingList::getBinarySize() {
	return HEADER_SIZE + 5 * totalHeight();
}

schar* TrackingList::serialize() {
	schar* s = new schar[getBinarySize()]();
	s[0] = (schar)mode;
	s[1] = (schar)selectedIndex;
	s[2] = (schar)activeIndex1;
	*(int*)(s + 3) = lastTimeStamp;
	*(int*)(s + 7) = accumulatedTime;
	for(int i = 0; i < size(); ++i) {
		while(at(i)->getHeight() > 1) {
			s[HEADER_SIZE + (i + at(i)->getHeight()) * 5 - 1] = ')';
			breakPair(i);
		}
		*(int*)(s + HEADER_SIZE + i * 5) = at(i)->time;
		if (s[HEADER_SIZE + i * 5 + 4] != ')')
			s[HEADER_SIZE + i * 5 + 4] = ',';
	}
	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "%02x%02x%02x", s[0], s[1], s[2]);
	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "%d", *(int*)(s + 3));
	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "%d", *(int*)(s + 7));
	for(int i = 0; i < size(); ++i) {
		app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "%d%c", *(int*)(s + HEADER_SIZE + i * 5), s[HEADER_SIZE + i * 5 + 4]);
	}
	return s;
}

void TrackingList::deserialize(schar* s) {
	for(int i = 0, k = 0; k < size(); ++i) {
		at(k)->time = *(int*)(s + HEADER_SIZE + i * 5);
		if (s[HEADER_SIZE + i * 5 + 4] == ',')
			++k;
		else
			buildPair(k - 1, k);
	}
	mode = (TrackingListMode)s[0];
	selectedIndex = s[1];
	activeIndex1 = s[2];
	lastTimeStamp = *(int*)(s + 3);
	accumulatedTime = *(int*)(s + 7);
	updateTime();
}

void TrackingList::addTime(int value) {
	if (selectedIndex != NULL_V) {
		this->at(selectedIndex)->time = max(0, this->at(selectedIndex)->time + value);
		if (activeIndex1 != NULL_V && activeIndex1 != selectedIndex) {
			this->at(activeIndex1)->time = max(0, this->at(activeIndex1)->time - value);
		}
	}
	else {
		accumulatedTime = max(0, accumulatedTime + value);
	}
}

void TrackingList::subTime(int value) {
	addTime(-value);
}

void TrackingList::switchMode(TrackingListMode newMode) {
	updateTime();
	this->mode = newMode;
}

void TrackingList::resetIndex() {
	activeIndex2 = NULL_V;
	selectedIndex = NULL_V;
}

void TrackingList::restoreSelected() {
	if (activeIndex1 != NULL_V)
		selectedIndex = activeIndex1;
}

void TrackingList::restorePreviousActive() {
	activeIndex1 = activeIndex2;
	activeIndex2 = selectedIndex;
	selectedIndex = activeIndex1;
}

void TrackingList::incIndex() {
	incIndex(1);
}

void TrackingList::incIndex(int c) {
	activeIndex2 = NULL_V;
	for (int i = 0; i < c; ++i) {
		if (selectedIndex == NULL_V || selectedIndex == size() - 1)
			selectedIndex = 0;
		else
			++selectedIndex;
	}
}

void TrackingList::decIndex() {
	decIndex(1);
}

void TrackingList::decIndex(int c) {
	for (int i = 0; i < c; ++i) {
		if (selectedIndex == NULL_V || selectedIndex == 0)
			selectedIndex = size() - 1;
		else
			--selectedIndex;
	}
}

void TrackingList::switchIndex() {
	if (selectedIndex != NULL_V) {
		switch(mode) {
			case NORMAL_MODE:
				updateTime();
				if (activeIndex1 == NULL_V || activeIndex1 != selectedIndex) {
					activeIndex2 = activeIndex1;
					activeIndex1 = selectedIndex;
				}
				else {
					activeIndex2 = NULL_V;
					activeIndex1 = NULL_V;
				}
				break;
			case BUILD_BREAK_MODE:
				if (activeIndex1 == selectedIndex) {
					activeIndex1 = NULL_V;
				}
				else if (activeIndex1 == NULL_V || abs(selectedIndex - activeIndex1) != 1 || !buildPair(selectedIndex)) {
					activeIndex1 = selectedIndex;
				}
				break;
		}
	}
}

bool TrackingList::buildPair() {
	if (activeIndex1 == NULL_V) {
		for(int i = 0; i < size() - 1; ++i)
			buildPair(i, i + 1);
		activeIndex1 = NULL_V;
		return true;
	}

	bool b = false;
	if (activeIndex1 > 0)
		b = buildPair(activeIndex1, activeIndex1 - 1);
	if (!b && activeIndex1 < size() - 1)
		b = buildPair(activeIndex1, activeIndex1 + 1);
	return b;
}

bool TrackingList::buildPair(int activeIndex2) {
	return buildPair(this->activeIndex1, activeIndex2);
}

bool TrackingList::buildPair(int activeIndex1, int activeIndex2) {
	if (activeIndex2 < activeIndex1)
		std::swap(activeIndex1, activeIndex2);
	char pairNameKey[25];
	strcpy(pairNameKey, this->at(activeIndex1)->name);
	strcat(pairNameKey, this->at(activeIndex2)->name);

	bool result = false;
	if (possiblePairs.find(pairNameKey) != possiblePairs.end()) {
		char* pairName = possiblePairs[pairNameKey];
		BaseTracking* newPair = new TrackingPair(pairName, this->at(activeIndex1), this->at(activeIndex2));
		erase(this->begin() + activeIndex1, this->begin() + activeIndex2 + 1);
		insert(this->begin() + activeIndex1, newPair);
		this->activeIndex1 = activeIndex1;
		result = true;
	}

	return result;
}

bool TrackingList::buildAll() {
	for(int i = 0; i < size() - 1; ++i) {
		while(buildPair(i, i + 1));
	}
	activeIndex1 = NULL_V;
	return true;
}

bool TrackingList::breakPair() {
	if (selectedIndex != NULL_V)
		return breakPair(selectedIndex);
	else if (activeIndex1 != NULL_V)
		return breakPair(activeIndex1);
	for(int i = 0; i < size(); ++i) {
		if (breakPair(i))
			++i;
	}
	return true;
}

bool TrackingList::breakPair(int index) {
	if (this->at(index)->getHeight() == 1)
		return false;
	TrackingPair* pair = static_cast<TrackingPair*>(this->at(index));

	BaseTracking* element1 = pair->element1;
	BaseTracking* element2 = pair->element2;
	int timeDiff = pair->time - element1->time - element2->time;
	if (element1->getPriority() == element2->getPriority()) {
		if (element1->time != element2->time) {
			BaseTracking* outRunning;
			BaseTracking* lagging;
			if (element1->time < element2->time == timeDiff > 0) {
				lagging = element1;
				outRunning = element2;
			}
			else {
				lagging = element2;
				outRunning = element1;
			}
			int overTakeTimeDiff = outRunning->time - lagging->time;
			if (abs(timeDiff) <= abs(overTakeTimeDiff)) {
				lagging->time += timeDiff;
				timeDiff = 0;
			}
			else {
				lagging->time += overTakeTimeDiff;
				timeDiff -= overTakeTimeDiff;
			}
		}
		element1->time += timeDiff / 2 + timeDiff % 2;
		element2->time += timeDiff / 2;
	}
	else if (timeDiff > 0 == element1->getPriority() < element2->getPriority()) {
		element1->time += timeDiff;
		if (element1->time < 0) {
			element2->time += element1->time;
			element1->time = 0;
		}
	}
	else {
		element2->time += timeDiff;
		if (element2->time < 0) {
			element1->time += element2->time;
			element2->time = 0;
		}
	}

	erase(this->begin() + index);
	delete pair;
	insert(this->begin() + index, element2);
	insert(this->begin() + index, element1);

	return true;
}

bool TrackingList::breakAll() {
	for(int i = 0; i < size(); ++i) {
		while(at(i)->getHeight() > 1) {
			breakPair(i);
		}
	}
	return true;
}

int TrackingList::updateTime() {
	time_t currentTime = time(0L);
	int newTime = NULL_V;
	if (mode == NORMAL_MODE && lastTimeStamp != NULL_V && activeIndex1 != NULL_V) {
		this->at(activeIndex1)->time += currentTime - lastTimeStamp;
		newTime = this->at(activeIndex1)->time;
	}
	lastTimeStamp = currentTime;
	return newTime;
}

void TrackingList::resetSelectedTime() {
	if (selectedIndex != NULL_V)
		this->at(selectedIndex)->time = 0;
	else
		accumulatedTime = 0;
}

void TrackingList::resetTime(bool resetAccumulated) {
	if (resetAccumulated)
		accumulatedTime = 0;
	else
		accumulatedTime += totalTime(false);
	for_each(this->begin(), this->end(), [](BaseTracking* a) { a->time = 0; });
}

int TrackingList::totalHeight() const {
	return accumulate(begin(), end(), 0, [](int a, BaseTracking* b) { return a + b->getHeight(); });
}

int TrackingList::totalTime() const {
	return totalTime(true);
}

int TrackingList::totalTime(bool accumulated) const {
	int totalTime = accumulate(begin(), end(), 0, [](int a, BaseTracking* b) { return a + b->time; });
	return accumulated ? totalTime + accumulatedTime : totalTime;
}
