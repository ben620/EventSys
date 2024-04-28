
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
    
    using namespace es;
    
    ESI().Register(EventID::NewJob, &s, &r, &Svr::OnReady);
    ESI().Register(EventID::NewJob, &s, &s, &Svr::OnReady);
    ESI().Register(EventID::OnVoid, &s, &r, &Svr::OnVoid);
    ESI().Register(EventID::OnPointer, &s, &r, &Svr::OnPointer);
    ESI().Register(EventID::OnConstMethod, &s, &r, &Svr::OnConstMethod);
    ESI().Register(EventID::OnStaticMethod, &s, &Svr::OnStaticMethod);
    ESI().Register(EventID::OnNoSender, nullptr, &r, &Svr::OnNoSender);
    ESI().Register(EventID::OnLambda, nullptr, [](int b) { std::cout << "OnLambda " << b << std::endl; });
    ESI().Register(EventID::OnStdFunction, nullptr, std::function([](int b) { std::cout << "OnStdFunction " << b << std::endl; }));

    ESI().Send(EventID::NewJob, &s, 1, std::string("abc"));
    std::cout << "change sender" << std::endl;
    ESI().Send(EventID::NewJob, &r, 1, std::string("abc"));
    ESI().Send(EventID::OnVoid, &s);
    ESI().Send(EventID::OnPointer, &s, "pointer args");
    ESI().Send(EventID::OnConstMethod, &s);
    ESI().Send(EventID::OnStaticMethod, &s);
    ESI().Send(EventID::OnNoSender, &s);
    ESI().SendAll(EventID::OnNoSender);
    ESI().SendAll(EventID::OnLambda, 1);
    ESI().SendAll(EventID::OnStdFunction, 1);
    const char *name = typeid(int).name();

    return 0;
}