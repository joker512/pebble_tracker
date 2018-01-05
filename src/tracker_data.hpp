#include "pebble.hpp"

#include <map>
#include <vector>
#include <algorithm>

#define NULL_V -1

typedef signed char schar;

class BaseTracking {
public:
	virtual ~BaseTracking() {};

	char const* getName() const;
	int getTime() const;
	virtual int getPriority() const {} // = 0;
	virtual int getHeight() const {} // = 0;

	friend class TrackingList;

protected:
	char* name;
	int time = 0;
};

class TrackingElement : public BaseTracking {
public:
	TrackingElement(char*, int);
	~TrackingElement();

	int getPriority() const;
	int getHeight() const;
private:
	int priority;
};

class TrackingPair : public BaseTracking {
public:
	TrackingPair(char*, BaseTracking*, BaseTracking*);

	int getPriority() const;
	int getHeight() const;

	BaseTracking* element1;
	BaseTracking* element2;
};

enum TrackingListMode { NORMAL_MODE, BUILD_BREAK_MODE, FREEZE_MODE };

struct CharComparator {
	bool operator()(char const *a, char const *b) {
		return strcmp(a, b) < 0;
	}
};

class PairMap : private std::map<char*, char*, CharComparator> {
public:
	using map::empty;

	std::pair<iterator, bool> insert(std::pair<char*, char*> pair) {
		char* first = new char[strlen(pair.first) + 1];
		strcpy(first, pair.first);
		char* second = new char[strlen(pair.second) + 1];
		strcpy(second, pair.second);
		return std::map<char*, char*, CharComparator>::insert(std::pair<char*, char*>(first, second));
	}

	friend class TrackingList;
};

class TrackingList : private std::vector<BaseTracking*> {
public:
	TrackingList(std::vector<BaseTracking*>, PairMap&);
	TrackingList(std::vector<BaseTracking*>, PairMap&, int, int);
	~TrackingList();

	using vector::size;
	using vector::at;

	TrackingListMode getMode() const {
		return mode;
	}
	int getSelectedIndex() const {
		return selectedIndex;
	}
	int getActiveIndex() const {
		return activeIndex1;
	}
	int getPreviousActiveIndex() const {
		return activeIndex2;
	}
	int getTotalHours() const {
		return totalHours;
	}
	int getTotalAccHours() const {
		return totalAccHours;
	}

	int getBinarySize();
	schar* serialize();
	void deserialize(schar*);

	void switchMode(TrackingListMode);
	void resetIndex();
	void restoreSelected();
	void restorePreviousActive();
	void incIndex();
	void incIndex(int);
	void decIndex();
	void decIndex(int);
	void switchIndex();

	bool buildPair();
	bool buildPair(int);
	bool buildPair(int, int);
	bool buildAll();
	bool breakPair();
	bool breakPair(int);
	bool breakAll();

	int updateTime();
	void addTime(int);
	void subTime(int);
	void resetSelectedTime();
	void resetTime(bool);

	int totalHeight() const;
	int totalTime() const;
	int totalTime(bool) const;

private:
	static const int HEADER_SIZE;
	PairMap possiblePairs;
	TrackingListMode mode = NORMAL_MODE;
	int selectedIndex = NULL_V;
	int activeIndex1 = NULL_V;
	int activeIndex2 = NULL_V;
	int lastTimeStamp = NULL_V;
	int accumulatedTime = 0;
	int totalHours = 8;
	int totalAccHours = 40;
};
