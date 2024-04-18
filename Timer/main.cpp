#include <iostream>
#include "CuTimer.h"

int main()
{
	CU::Timer::SingleShot(8000, []() {
		printf("Single Shot!\n");
	});

	{
		CU::Timer timer{};
		timer.setTimeOutCallback([]() {
			printf("Hello World!\n");
		});
		timer.setInterval(1000);
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
			TIMER_LOOP(timer) {
				printf("Timer Loop!\n");
				timer.setInterval(500);
			}
		});
		timer.setInterval(1000);
		timer.start();

		std::this_thread::sleep_for(std::chrono::seconds(5));
		timer.pauseTimer();

		std::this_thread::sleep_for(std::chrono::seconds(5));
		timer.continueTimer();

		std::this_thread::sleep_for(std::chrono::seconds(5));
	}
	
	return 0;
}
