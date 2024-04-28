
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
    
    void OnNoSender() const
    {
        std::cout << "OnNoSender" << std::endl;
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
        OnNoSender,
        OnLambda,
        OnStdFunction,
        Max
    };
    
    
    EventSystem::Inst().Register(EventID::NewJob, &s, &r, &Svr::OnReady);
    EventSystem::Inst().Register(EventID::NewJob, &s, &s, &Svr::OnReady);
    EventSystem::Inst().Register(EventID::OnVoid, &s, &r, &Svr::OnVoid);
    EventSystem::Inst().Register(EventID::OnPointer, &s, &r, &Svr::OnPointer);
    EventSystem::Inst().Register(EventID::OnConstMethod, &s, &r, &Svr::OnConstMethod);
    EventSystem::Inst().Register(EventID::OnStaticMethod, &s, &Svr::OnStaticMethod);
    EventSystem::Inst().Register(EventID::OnNoSender, nullptr, &r, &Svr::OnNoSender);
    EventSystem::Inst().Register(EventID::OnLambda, nullptr, [](int b) { std::cout << "OnLambda " << b << std::endl; });
    EventSystem::Inst().Register(EventID::OnStdFunction, nullptr, std::function([](int b) { std::cout << "OnStdFunction " << b << std::endl; }));

    EventSystem::Inst().Send(EventID::NewJob, &s, 1, std::string("abc"));
    std::cout << "change sender" << std::endl;
    EventSystem::Inst().Send(EventID::NewJob, &r, 1, std::string("abc"));
    EventSystem::Inst().Send(EventID::OnVoid, &s);
    EventSystem::Inst().Send(EventID::OnPointer, &s, "pointer args");
    EventSystem::Inst().Send(EventID::OnConstMethod, &s);
    EventSystem::Inst().Send(EventID::OnStaticMethod, &s);
    EventSystem::Inst().Send(EventID::OnNoSender, &s);
    EventSystem::Inst().SendAll(EventID::OnNoSender);
    EventSystem::Inst().SendAll(EventID::OnLambda, 1);
    EventSystem::Inst().SendAll(EventID::OnStdFunction, 1);

    return 0;
}