#pragma once

#include <chrono>

namespace lib
{
class StopWatch
{
    private:
        bool _is_running = false;
        std::chrono::time_point<std::chrono::system_clock> _start, _end = std::chrono::system_clock::now();
    public:
        StopWatch(void) = default;
        ~StopWatch(void) = default;
        void start(void) {
            _is_running = true;
            _start = std::chrono::system_clock::now();
        };
        void stop(void) {
            _is_running = false;
            _end = std::chrono::system_clock::now();
        }
        void restart(void) {
            start();
        }
        bool is_running(void) const {
            return this->_is_running;
        };
        double getElapsedTime(void) const {
            if (this->_is_running) {
                return (std::chrono::duration<double> (std::chrono::system_clock::now() - _start)).count();
            } else {
                return (std::chrono::duration<double> (_end - _start)).count();
            }
        };
};
}