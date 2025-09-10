/**
 * @file
 *
 * Declaration of the Bee class
 */

#ifndef _BEE_H
#define _BEE_H

#include "utils.h"

class Environment;

/**
 * The Bee class ...
 */
class Bee {

public:
    Bee(fPos pos, Environment* pEnv);
    ~Bee() {}

    void move();

    float x;
    float y;

private:
    Environment* m_pEnv;
};

#endif /* _BEE_H */
