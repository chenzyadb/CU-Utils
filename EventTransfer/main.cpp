#include <iostream>
#include "CuEventTransfer.h"

int main()
{
    auto subscriber = [](const CU::EventTransfer::TransData &data) {
        auto msg = CU::EventTransfer::GetData<std::string>(data);
        std::cout << msg << std::endl;
    };
    CU::EventTransfer::Post<std::string>("MyEvent", "hello, world!");
    CU::EventTransfer::Subscribe("MyEvent", subscriber);
    CU::EventTransfer::Post<std::string>("MyEvent", "hello, world!");
    CU::EventTransfer::Unsubscribe(subscriber);
    CU::EventTransfer::Post<std::string>("MyEvent", "hello, world!");
}
