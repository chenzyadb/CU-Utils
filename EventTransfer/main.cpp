#include <iostream>
#include "CuEventTransfer.h"

class Test
{   public:
        Test()
        {
            CU::EventTransfer::Post<std::string>("TestCreated", "Test created!");
            CU::EventTransfer::Subscribe("MyEvent", this, std::bind(&Test::Subscriber, this, std::placeholders::_1));
        }

        ~Test()
        {
            CU::EventTransfer::Unsubscribe("MyEvent", this);
        }

        void Subscriber(const CU::EventTransfer::TransData &data)
        {
            auto msg = CU::EventTransfer::GetData<std::string>(data);
            std::cout << "class Test: " << msg << std::endl;
        }
};

void Subscriber(const CU::EventTransfer::TransData &data)
{
    auto msg = CU::EventTransfer::GetData<std::string>(data);
    std::cout << msg << std::endl;
}

int main()
{
    CU::EventTransfer::Subscribe("MyEvent", nullptr, &Subscriber);
    CU::EventTransfer::Subscribe("TestCreated", nullptr, &Subscriber);
    {
        Test test{};
        CU::EventTransfer::Post<std::string>("MyEvent", "hello, world!");
        CU::EventTransfer::Unsubscribe("MyEvent", nullptr);
    }
    CU::EventTransfer::Subscribe("MyEvent", nullptr, [](CU::EventTransfer::TransData data) {
        auto msg = CU::EventTransfer::GetData<std::string>(data);
        std::cout << "lambda: " << msg << std::endl;
    });
    CU::EventTransfer::Post<std::string>("MyEvent", "hello, world!");
}
