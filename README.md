# Pebble tracker
It is an application for convenient time tracking. Split your workday on time slots, choose your current one and that's it. App shows the total time (for example, day working hours) and total accumulated time (week hours) after last reset. It vibrates twice when total time is multiple of the value indicated on the settings page; or thrice in a similar case for the total accumulated time. You can merge your time slots into more general ones. It makes time control more flexible. Look at the possible actions below:

| states / buttons        | down      | up        | select                           | back                                  | long down                  | long up                  | long select                           |
|-----------------------|-----------|-----------|----------------------------------|---------------------------------------|----------------------------|--------------------------|---------------------------------------|
| normal / in list      | go down   | go up     | (de)activate time slot           | go to header                          | go 3 down / go to previous | go 3 up / go to previous | switch to time editing                |
| normal / header       | go down   | go up     | switch to merge-split            | exit                                  | switch to time editing     | full reset / restore     | soft reset / restore                  |
| time editing          | digit - 1 | digit + 1 | next digit / end edit            | previous digit / end edit             | go down and edit / -10;3;5 | go up and edit / +10;3;5 | reset time slot                       |
| merge-split / in list | go down   | go up     | (de)activate / merge if possible | go to header                          | go 3 down                  | go 3 up                  | split current if possible             |
| merge-split / header  | go down   | go up     | switch to normal                 | merge active and return / merge level | merge all                  | split all                | split active and return / split level |

In general, it's enough, it's easy to understand all features just by playing with the app. But the curious can read further.

## Some comments

### Soft reset and full reset

* Soft reset splits all list elements and zeroes them, and it saves the total time to the total accumulated time.
* Full reset does the same, but also zeroes the total accumulated time. It can be useful to control day and week time separately.
* If you reset time unintentionally, don't panic, just **repeat long select or long up click** and the app will restore the last state.

### Time editing hidden features

1. You can switch between different time slots while editing. Just use long up or long down. For the total accumulated time editing it works as ±10 hours / ±30 minutes / ±5 minutes. I came to conclusion that it is more convenient here.
2. If you haven't edited time for **1 minute** program will automatically switch to the normal mode. It's necessary for cases when you press the select button unintentionally.
3. If you are adding time to slot 2 and time slot 1 is active, time will **flow from 1 to 2**. It's useful in case when you have forgotten to switch time slot.
4. Surely, you can edit the total accumulated time (long down on header in normal mode).

Keep in mind that time goes **only in normal mode with activated time slot**.

### How to merge time slots easily

_This section is mostly for advanced users. It helps to improve your performance and can be tricky at the beginning._

Let's imagine that the selected active time slot is 1 and you need to merge it with 2 and return.

* Slow way: back - select - down - down - select - back - select - down (8).
* Fast way: back - select - back (3).

That's what "_merge active and return_" means.

Now let's imagine the opposite. Your selected time slot is 1 and you need to split it.

* Slow way: back - select - down - long select - back - select - down (7).
* Fast way: back - select - long select (3).

That's what "_split active and return_" means.

Also you can merge/split the whole list by long up, long down, back and long select (the latter 2 imply no active time slots otherwise it will do _merge/split active and return_) in merge-split / header mode.

### Go back and forth

_Also for advanced users._

Sometimes you need to jump between 2 time slots many times. Let's imagine that you were on time slot 2, switched to time slot 4 and want back.

* Slow way: up - up - select (3).
* Fast way: long down (1).

The opposite, you were on time slot 4, switched to time slot 2 and want back.

* Slow way: down - down - select (3).
* Fast way: long up (1).

Just make long press in the direction of the closest list side. It works only if you haven't changed the selection.

### How is time divided between time slots after split

_I think it is not so necessary for everyday usage. It is here just for completeness._

Time division depends on priorities which you can assign on settings page.

1. The most priority time slot (which has the lowest value) gets all accumulated time (passed after merge operation).
2. If priorities are the same and the accumulated time is less than the difference between time slots values, the poorest time slot gets all.
3. If not and accumulated time is more than the difference, then:
 1. The lowest time slot value becomes equal to the biggest.
 2. The rest of the accumulated time is shared equally between time slots.
4. Merged time slot has the priority of the highest inner time slot.

### Other points

_For the most curious._

* Time values have been updated once a minute if you don't do any actions. So, don't worry about the battery life.
* Total accumulated time is shown only if it is not equal to the total time or in the time editing mode.
* Vibrations happen if:
 * Pebble has lost/found bluetooth connection.
 * Time slot value is multiple of an hour.
 * Total or total accumulated time is multiple of value indicated on the settings page.
 * You have deactivated the current time slot. It's necessary since deactivation happens unintentionally sometimes.
* It's assumed that time slots values is less than **100 hours** and total accumulated time is less than **1000 hours**, that's why you can edit only hours, 10-minutes and minutes in time editing. You can overcome these restrictions somehow, but don't blame me whether it looks bad.


## About application settings

There are some restrictions preventing the app looking ugly:

 1. No more than 6 leaves in a tree.
 2. No less than 2 nodes on top level of a tree.
 3. No more than 12 symbols for each leaf.
 4. No more than 10 symbols for each inner node.

Also keep in mind that if you press "**Confirm**" all your time slots values will be lost **forever**. Even if you haven't changed anything in tree.

## Questions, comments and suggestions

You're welcome. Use github comments or send to kotsursv@gmail.com.