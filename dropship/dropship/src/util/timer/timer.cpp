#include "pch.h"

#include "timer.h"


namespace util::timer {

	Timer::Timer(const char* name):
		_name(name),
		_stopped(false)
	{
		this->_start_time_point = std::chrono::high_resolution_clock::now();
	}

	Timer::~Timer() {
		if (!this->_stopped)
		{
			this->stop();
		}
	}

	void Timer::stop() {
		auto end_time_point = std::chrono::high_resolution_clock::now();

		this->_stopped = true;

		//auto duration =  duration_cast<std::chrono::milliseconds>(end_time_point - this->_start_time_point);
		auto duration = duration_cast<std::chrono::microseconds>(end_time_point - this->_start_time_point).count() * 0.001f;

		// web https://stackoverflow.com/a/54062826

		println("\x1B[33mduration \x1B[93m{}\x1B[33m: \x1B[93m{:.2f}ms\033[0m", this->_name, duration);
		
	}

}
