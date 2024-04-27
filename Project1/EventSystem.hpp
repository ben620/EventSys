#pragma once
#include <tuple>
#include <functional>

class EventSystem
{
public:
    static EventSystem& Inst()
    {
        static EventSystem es;
        return es;
    }

    EventSystem(const EventSystem&) = delete;
    void operator=(const EventSystem&) = delete;

    using CallBackHandle = uint32_t;

    template <typename EventType, typename ...Args>
    void Send(EventType evtID, const void* sender, Args... args) const
    {
        const std::tuple<std::add_lvalue_reference_t<std::add_const_t<std::decay_t<Args>>>...> evt(args...);
        Call((int)evtID, sender, &evt);
    }

    template <typename EventType, typename C, typename ...Args>
    CallBackHandle Register(EventType evtID, const void* sender, C* recver, void (C::* f)(Args...))
    {
        FnCallBack callBack = MakeCallBack(recver, f);
        return Reg((int)evtID, sender, recver, std::move(callBack));
    }


    template <typename EventType, typename C, typename ...Args>
    CallBackHandle Register(EventType evtID, const void* sender, C* recver, void (C::* f)(Args...) const)
    {
        FnCallBack callBack = MakeCallBack(recver, f);
        return Reg((int)evtID, sender, recver, std::move(callBack));
    }

    //static and free function as callback
    template <typename EventType, typename ...Args>
    CallBackHandle Register(EventType evtID, const void* sender, void (*f)(Args...))
    {
        FnCallBack callBack = MakeCallBack(f);
        return Reg((int)evtID, sender, nullptr, std::move(callBack));
    }

    //listen to event regardless of sender, for method
    template <typename EventType, typename C, typename ...Args>
    CallBackHandle Register(EventType evtID, const C *recver, void (C::*f)(Args...))
    {
        FnCallBack callBack = MakeCallBack(f);
        return Reg((int)evtID, nullptr, recver, std::move(callBack));
    }

    void Unregister(const void* ob);
    void Unregister(CallBackHandle id);
    void Clear();

private:
    template <typename C, typename... Args>
    static auto MakeCallBack(C* obj, void (C::* f)(Args...))
    {
        using FnType = void (C::*)(Args... args);
        return [f = std::forward<FnType>(f), obj](const void* e)
            {
                if constexpr (sizeof...(Args) > 0)
                {
                    using TupleType = std::tuple<std::add_lvalue_reference_t<std::add_const_t<std::decay_t<Args>>>...>;
                    const TupleType* eventBody = reinterpret_cast<const TupleType*>(e);
                    std::apply(std::bind_front(f, obj), *eventBody);
                }
                else
                {
                    std::invoke(f, obj);
                }
            };
    }

    template <typename C, typename... Args>
    static auto MakeCallBack(const C* obj, void (C::* f)(Args...) const)
    {
        using FnType = void (C::*)(Args... args) const;
        return [f = std::forward<FnType>(f), obj](const void* e)
            {
                if constexpr (sizeof...(Args) > 0)
                {
                    using TupleType = std::tuple<std::add_lvalue_reference_t<std::add_const_t<std::decay_t<Args>>>...>;
                    const TupleType* eventBody = reinterpret_cast<const TupleType*>(e);
                    std::apply(std::bind_front(f, obj), *eventBody);
                }
                else
                {
                    std::invoke(f, obj);
                }
            };
    }

    template <typename... Args>
    static auto MakeCallBack(void (*f)(Args...))
    {
        using FnType = void (*)(Args...);
        return [f = std::forward<FnType>(f)](const void* e)
            {
                if constexpr (sizeof...(Args) > 0)
                {
                    using TupleType = std::tuple<std::add_lvalue_reference_t<std::add_const_t<std::decay_t<Args>>>...>;
                    const TupleType* eventBody = reinterpret_cast<const TupleType*>(e);
                    std::apply(f, *eventBody);
                }
                else
                {
                    f();
                }
            };
    }

    EventSystem();

    using FnCallBack = std::function<void(const void*)>;

    CallBackHandle Reg(int evt, const void *sd, const void *rc, FnCallBack &&cb);
    void Call(int evt, const void* sd, const void* args) const;

private:
    friend struct EventSystemImp;
    struct EventSystemImp* _imp;
};
