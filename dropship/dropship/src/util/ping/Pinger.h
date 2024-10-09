#pragma once

#include "asio/asio.h"

namespace util::ping {

	class Pinger
	{
		public:

			Pinger(std::string ip, std::unique_ptr<std::optional<int>> const& ping);
			~Pinger();

			void start();
			void stop();

			void ping();

		private:
			std::unique_ptr<asio::io_context> io_context;
			std::unique_ptr<AsioPinger> pinger;
	};
}