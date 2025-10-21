/**
 * @file
 *
 * Declaration of the Hive class
 */

#ifndef _HIVE_H
#define _HIVE_H

/**
 * The Hive class ...
 */
class Hive {

public:
    Hive() : x(0.0f), y(0.0f), direction(4) {}
    Hive(float x, float y, int direction) : x(x), y(y), direction(direction) {}

private:
    float x; // position of hive in enrironment coordinates
    float y; // position of hive in enrironment coordinates
    int direction; // 0=North, 1=East, 2=South, 3=West, 4=Random
};

#endif /* _HIVE_H */
