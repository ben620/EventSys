
#include <iostream>
#include "EventSystem.hpp"




class Svr
{
public:
    void OnReady(int code, const std::string& b)
    {
        std::cout << "read " << code << " " << b << std::endl;
    }

    void OnVoid()
    {
        std::cout << "OnVoid" << std::endl;
    }

    void OnPointer(const char* ch)
    {
        std::cout << ch << std::endl;
    }

    void OnConstMethod() const
    {
        std::cout << "OnConstMethod" << std::endl;
    }

    static void OnStaticMethod()
    {
        std::cout << "OnStaticMethod" << std::endl;
    }
};

int main()
{
    Svr s;
    Svr r;
    
    enum class EventID
    {
        NewJob,
        OnVoid,
        OnPointer,
        OnConstMethod,
        OnStaticMethod,
        Max
    };


    EventSystem::Inst().Register(EventID::NewJob, &s, &r, &Svr::OnReady);
    EventSystem::Inst().Register(EventID::OnVoid, &s, &r, &Svr::OnVoid);
    EventSystem::Inst().Register(EventID::OnPointer, &s, &r, &Svr::OnPointer);
    EventSystem::Inst().Register(EventID::OnConstMethod, &s, &r, &Svr::OnConstMethod);
    EventSystem::Inst().Register(EventID::OnStaticMethod, &s, &Svr::OnStaticMethod);

    EventSystem::Inst().Send(EventID::NewJob, &s, 1, std::string("abc"));
    EventSystem::Inst().Send(EventID::OnVoid, &s);
    EventSystem::Inst().Send(EventID::OnPointer, &s, "pointer args");
    EventSystem::Inst().Send(EventID::OnConstMethod, &s);
    EventSystem::Inst().Send(EventID::OnStaticMethod, &s);
    

    return 0;
}