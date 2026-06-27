#ifndef RT_H
#define RT_H

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <random>

// C++ std usings

using std::make_shared;
using std::shared_ptr;

// constants

const double infinity = std::numeric_limits<double>::infinity();
const double pi = 3.1415926535897932385;

// utility functions

inline double degrees_to_radians(double degrees) {
    return degrees * pi / 180.0;
}

inline double random_double() {
    // returns a random real in [0,1)
    return std::rand() / (RAND_MAX + 1.0);
}

inline double random_double(double min, double max) {
    // returns a random real in [min,max)
    return min + (max-min)*random_double();
}

inline double random_double(std::mt19937& rng) { 
    std::uniform_real_distribution<double> dist(0.0, 1.0); 
    return dist(rng); 
}

inline double random_double(std::mt19937& rng, double min, double max) {
    // returns a random real in [min,max)
    return min + (max-min)*random_double(rng);
}

// common headers

#include "color.h"
#include "interval.h"
#include "ray.h"
#include "vec3.h"

#endif