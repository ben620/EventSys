#pragma once
#include <tuple>
#include <functional>

namespace es
{

    template<typename T>
    struct function_traits;

    template<typename ReturnType, typename... Args>
    struct function_traits<ReturnType(Args...)>
    {
        using tuple_type = std::tuple<std::add_lvalue_reference_t<std::add_const_t<std::decay_t<Args>>>...>;
        static constexpr std::size_t arg_count = sizeof ...(Args);
    };

    template<typename ReturnType, typename... Args>
    struct function_traits<ReturnType(*)(Args...)> : function_traits<ReturnType(Args...)> {};

    template<typename ReturnType, typename... Args>
    struct function_traits<std::function<ReturnType(Args...)>> : function_traits<ReturnType(Args...)> {};

#define FUNCTION_TRAITS(...)\
template <typename ReturnType, typename ClassType, typename... Args>\
struct function_traits<ReturnType(ClassType::*)(Args...) __VA_ARGS__> : function_traits<ReturnType(Args...)>{};\

    FUNCTION_TRAITS()
        FUNCTION_TRAITS(const)
        FUNCTION_TRAITS(volatile)
        FUNCTION_TRAITS(const volatile)
        FUNCTION_TRAITS(const&&)
#undef FUNCTION_TRAITS

        template<typename Callable>
    struct function_traits : function_traits<decltype(&Callable::operator())> {};

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

    class EventSystem
    {
    public:
        static inline EventSystem& Inst()
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

        template <typename EventType, typename ...Args>
        void SendAll(EventType evtID, Args... args) const
        {
            const std::tuple<std::add_lvalue_reference_t<std::add_const_t<std::decay_t<Args>>>...> evt(args...);
            Call((int)evtID, nullptr, &evt);
        }

        template <typename EventType, typename F>
        CallBackHandle Register(EventType evtID, const void* sender, F&& f)
        {
            FnCallBack callBack = MakeCBStorage(std::forward<F>(f));
            return Reg((int)evtID, sender, nullptr, std::move(callBack));
        }

        template <typename EventType, typename RC, typename F>
        CallBackHandle Register(EventType evtID, const void* sender, const RC* recver, F&& f)
        {
            FnCallBack cb = MakeCBStorage(recver, std::forward<F>(f));
            return Reg((int)evtID, sender, recver, std::move(cb));
        }


        void Unregister(const void* ob);
        void Unregister(CallBackHandle id);
        void Clear();

    private:
        EventSystem();

        using FnCallBack = std::function<void(const void*)>;

        CallBackHandle Reg(int evt, const void* sd, const void* rc, FnCallBack&& cb);
        void Call(int evt, const void* sd, const void* args) const;

    private:
        friend struct EventSystemImp;
        struct EventSystemImp* _imp;
    };

    static inline EventSystem& ESI()
    {
        return EventSystem::Inst();
    }
}

