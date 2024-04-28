
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


template <typename F>
static auto MakeCBStorage(F&& f)
{
    return [f = std::forward<F>(f)](const void* e)
        {
            if constexpr (function_traits<F>::arg_count > 0)
            {
                using TupleType = typename function_traits<F>::tuple_type;
                const TupleType* eventBody = reinterpret_cast<const TupleType*>(e);
                std::apply(f, *eventBody);
            }
            else
            {
                f();
            }
        };
}

template <typename OBJ, typename F>
static auto MakeCBStorage(const OBJ* obj, F&& f)
{
    
    return [f = std::forward<F>(f), obj = const_cast<OBJ*>(obj)](const void* e)
        {
            if constexpr (function_traits<F>::arg_count > 0)
            {
                using TupleType = typename function_traits<F>::tuple_type;
                const TupleType* eventBody = reinterpret_cast<const TupleType*>(e);
                std::apply(std::bind_front(f, obj), *eventBody);
            }
            else
            {
                std::invoke(f, obj);
            }
        };
}

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
        Max
    };
    auto cb = MakeCBStorage([](int a) { std::cout << a << std::endl; });
    
    std::tuple<const int&> param(1);
    cb(&param);

    auto cb1 = MakeCBStorage(&r, &Svr::OnVoid);
    
    //EventSystem::Inst().Register(EventID::NewJob, &s, &r, &Svr::OnReady);
    //EventSystem::Inst().Register(EventID::OnVoid, &s, &r, &Svr::OnVoid);
    //EventSystem::Inst().Register(EventID::OnPointer, &s, &r, &Svr::OnPointer);
    //EventSystem::Inst().Register(EventID::OnConstMethod, &s, &r, &Svr::OnConstMethod);
    //EventSystem::Inst().Register(EventID::OnStaticMethod, &s, &Svr::OnStaticMethod);
    //EventSystem::Inst().Register(EventID::OnNoSender, nullptr, &r, &Svr::OnNoSender);
    ////EventSystem::Inst().Register(EventID::OnLambda, nullptr, [](int b) { });

    //
    //EventSystem::Inst().Send(EventID::NewJob, &s, 1, std::string("abc"));
    //EventSystem::Inst().Send(EventID::OnVoid, &s);
    //EventSystem::Inst().Send(EventID::OnPointer, &s, "pointer args");
    //EventSystem::Inst().Send(EventID::OnConstMethod, &s);
    //EventSystem::Inst().Send(EventID::OnStaticMethod, &s);
    //EventSystem::Inst().Send(EventID::OnNoSender, &s);
    //EventSystem::Inst().SendAll(EventID::OnNoSender);

    return 0;
}