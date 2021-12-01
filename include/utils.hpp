#pragma once

class Random
{
    public:
        static float uniform(void) {
            return (static_cast<double>(rand()) / (RAND_MAX));
        }

        static float range(float min, float max) {
            return (uniform() * (max-min) + min);
        }
};

// static float min(float a, float b) {
//     return a < b ? a : b;
// }

// static float max(float a, float b) {
//     return a > b ? a : b;
// }