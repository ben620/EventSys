#pragma once
#include <tuple>
#include <functional>

namespace es
{
    template <typename... Args>
    struct TupleTypeFromArgs
    {
        using TupleType = std::tuple<std::add_lvalue_reference_t<std::add_const_t<std::decay_t<Args>>>...>;
    };
    template<typename T>
    struct FunctionTraits;

    template<typename ReturnType, typename... Args>
    struct FunctionTraits<ReturnType(Args...)>
    {
        using TupleType = typename TupleTypeFromArgs<Args...>::TupleType;
        static constexpr std::size_t arg_count = sizeof ...(Args);
    };

    template<typename ReturnType, typename... Args>
    struct FunctionTraits<ReturnType(*)(Args...)> : FunctionTraits<ReturnType(Args...)> {};

    template<typename ReturnType, typename... Args>
    struct FunctionTraits<std::function<ReturnType(Args...)>> : FunctionTraits<ReturnType(Args...)> {};

#define FUNCTION_TRAITS(...)\
template <typename ReturnType, typename ClassType, typename... Args>\
struct FunctionTraits<ReturnType(ClassType::*)(Args...) __VA_ARGS__> : FunctionTraits<ReturnType(Args...)>{};

    FUNCTION_TRAITS()
    FUNCTION_TRAITS(const)
    FUNCTION_TRAITS(volatile)
    FUNCTION_TRAITS(const volatile)
    FUNCTION_TRAITS(const&&)
#undef FUNCTION_TRAITS

    template<typename Callable>
    struct FunctionTraits : FunctionTraits<decltype(&Callable::operator())> {};

    template <typename F>
    static auto MakeCBStorage(F&& f)
    {
        return [f = std::forward<F>(f)](const void* e)
            {
                using TupleType = typename FunctionTraits<F>::TupleType;
                const TupleType* eventBody = reinterpret_cast<const TupleType*>(e);
                std::apply(f, *eventBody);
            };
    }

    template <typename OBJ, typename F>
    static auto MakeCBStorage(const OBJ* obj, F&& f)
    {
        return [f = std::forward<F>(f), obj = const_cast<OBJ*>(obj)](const void* e)
            {
                using TupleType = typename FunctionTraits<F>::TupleType;
                const TupleType* eventBody = reinterpret_cast<const TupleType*>(e);
                std::apply(std::bind_front(f, obj), *eventBody);
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
            const typename TupleTypeFromArgs<Args...>::TupleType evt(std::forward<Args>(args)...);
            Call((int)evtID, sender, &evt);
        }

        template <typename EventType, typename ...Args>
        void SendAll(EventType evtID, Args... args) const
        {
            const typename TupleTypeFromArgs<Args...>::TupleType evt(std::forward<Args>(args)...);
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

