#pragma once

#include <winuser.h> // IsWindow()

namespace util::watcher::window {

	/* functional */
	std::optional<HWND> findWindow(std::string& window_name);
	bool isWindowOpen();

	/* class */
	class WindowWatcher
	{
		/* consts */
		private:
			static constexpr auto watcher_delay = 900ms;

		public:
			WindowWatcher(std::string window_name);
			~WindowWatcher();
			bool isActive();

		private:
			void start();
			void stop();

		private:
			std::string _window_name;

			bool _window_open { false };
			bool _watching;

			/* worker */
			std::future<void> _watcher_future;
			std::mutex __watcher_future_condition_variable_mutex;
			std::condition_variable __watcher_future_condition_variable;
	};
}
