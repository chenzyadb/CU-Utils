#include <iostream>
#include "CuTimer.h"

int main()
{
	CU::Timer::SingleShot(8000, []() {
		std::cout << "Single Shot!" << std::endl;
	});

	{
		CU::Timer timer{};
		timer.setTimeOutCallback([]() {
			std::cout << "thread id: " << std::this_thread::get_id() << std::endl;
		});
		timer.setInterval(1000);
		timer.start();

		std::this_thread::sleep_for(std::chrono::seconds(5));
		timer.stop();
		timer.start();

		std::this_thread::sleep_for(std::chrono::seconds(5));
		timer.pauseTimer();

		std::this_thread::sleep_for(std::chrono::seconds(5));
		timer.continueTimer();

		std::this_thread::sleep_for(std::chrono::seconds(5));
	}

	{
		CU::Timer timer{};
		timer.setLoop([&]() {
			timer.setInterval(500);
			auto printVal = "Timer Loop!";
			TIMER_LOOP(timer) {
				std::cout << printVal << std::endl;
			}
		});
		timer.start();

		std::this_thread::sleep_for(std::chrono::seconds(5));
		timer.pauseTimer();

		std::this_thread::sleep_for(std::chrono::seconds(5));
		timer.continueTimer();

		std::this_thread::sleep_for(std::chrono::seconds(5));
	}
	
	return 0;
}
