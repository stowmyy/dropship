#pragma once

// web https://youtu.be/YbYV8rRo9_A?t=606

namespace util::timer {

	/*struct ProfileResult {
		const char* name;
		float duration_ms;
	};*/

	class Timer {

		public:
			Timer(const char* name);
			~Timer();

			void stop();

		private:
			const char* _name;
			std::chrono::time_point<std::chrono::steady_clock> _start_time_point;
			bool _stopped;
	};

}
