#include "pch.h"

#include "Pinger.h"

namespace util::ping {

	Pinger::Pinger(std::string ip, std::unique_ptr<std::optional<int>> const& ping) :
		io_context(std::make_unique<asio::io_context>()),
		pinger(std::make_unique<AsioPinger>(*io_context.get(), ip.c_str(), ping)
		) {
		/* this->ping() */
	}

	Pinger::~Pinger() {
	}


	void Pinger::start() {
		try {
			(*io_context).run();
		}
		catch (std::exception& e)
		{
			std::cerr << "Exception: " << e.what() << std::endl;
		}
	}

	void Pinger::stop() {
		try {
			(*io_context).stop();
		}
		catch (std::exception& e)
		{
			std::cerr << "Exception: " << e.what() << std::endl;
		}
	}

	void Pinger::ping() {
		try {
			(*io_context).run_one();
		}
		catch (std::exception& e)
		{
			std::cerr << "Exception: " << e.what() << std::endl;
		}
	}
}
