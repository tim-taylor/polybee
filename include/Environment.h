/**
 * @file
 *
 * Declaration of the Environment class
 */

#ifndef _ENVIRONMENT_H
#define _ENVIRONMENT_H

#include "Bee.h"
#include <vector>

/**
 * The Environment class ...
 */
class Environment {

public:
    Environment();
    ~Environment() {}

    void initialise(std::vector<Bee>* bees);
    void reset(); // reset environment to initial state

private:
    std::vector<Bee>* m_pBees;
};

#endif /* _ENVIRONMENT_H */
