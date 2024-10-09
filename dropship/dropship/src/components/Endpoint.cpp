#include "pch.h"

#include "Endpoint.h"

Endpoint2::Endpoint2(
	std::string title,
	std::string description,
	std::string ip_ping,
	bool blocked
):

	title(title),
	description(description),
	ip_ping(ip_ping.empty() ? std::nullopt : std::optional<std::string>{ ip_ping }),
	ping(std::make_unique<std::optional<int>>(std::nullopt)),
	ping_ms_display(-1),

	blocked(blocked),
	blocked_desired(blocked)
{
	// ...

	// TODO if config
	// TODO no ping, ping once, keep pinging
	if (this->ip_ping) {
		this->start_pinging();
	}
}

Endpoint2::~Endpoint2() {
	/*  */
	this->stop_pinging();
}


void Endpoint2::start_pinging() {
	try {
		/* endpoint has no ip */
		if (!this->ip_ping) return;
		if (this->pinger.get() != nullptr) return;

		/* ping */
		this->pinger = std::make_unique<util::ping::Pinger>(this->ip_ping.value(), ping);
		this->_ping_future = std::async(std::launch::async, [&]
		{
			try {
				(*(this->pinger)).start();
			}
			catch (const std::exception& ex) {
				// ...
				std::println("{}", ex.what());
			}
			catch (...)
			{
				printf("error caught!");
			}
		});
	}
	catch (const std::exception& ex) {
		// ...
		std::println("{}", ex.what());
	}
	catch (...)
	{
		printf("error caught!");
	}
}

void Endpoint2::stop_pinging() {
	if (this->pinger.get() != nullptr) {
		(*(this->pinger.get())).stop();
		this->_ping_future.get();
		(this->pinger).reset();
	}
}

std::string Endpoint2::getTitle() {
	return this->title;
}

bool Endpoint2::getBlockDesired() {
	return this->blocked_desired;
}

void Endpoint2::setBlockDesired(bool b) {
	this->blocked_desired = b;;
}

/*void Endpoint2::confirm() {
	this->blocked = this->blocked_desired;
}*/

bool Endpoint2::_getBlockedState() {
	return this->blocked;
}

void Endpoint2::_setBlockedState(bool b) {
	this->blocked = b;
}
