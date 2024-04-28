//(C) benyuan 2024
//all rights reserved
#pragma once
#include <tuple>
#include <cassert>
#include <functional>

namespace es
{
    template <typename... Args>
    struct TupleTypeFromArgs
    {
        using TupleType = std::tuple<std::add_lvalue_reference_t<std::add_const_t<std::decay_t<Args>>>...>;
    };

    template <typename ...Args>
    struct ArgumentStatistic
    {
        static constexpr uint32_t size = (uint32_t)(... + sizeof(std::decay_t<Args>));
        static constexpr uint32_t pointerCount = (uint32_t)(... + std::is_pointer_v<std::decay_t<Args>>);
        static constexpr uint32_t classCount = (uint32_t)(... + std::is_class_v<std::decay_t<Args>>);
    };

    template <>
    struct ArgumentStatistic<>
    {
        static constexpr uint32_t size = 0;
        static constexpr uint32_t pointerCount = 0;
        static constexpr uint32_t classCount = 0;
    };

    template<typename T>
    struct FunctionTraits;

    template<typename ReturnType, typename... Args>
    struct FunctionTraits<ReturnType(Args...)>
    {
        using TupleType = typename TupleTypeFromArgs<Args...>::TupleType;
        static constexpr uint32_t size = ArgumentStatistic<Args...>::size;
        static constexpr uint32_t pointerCount = ArgumentStatistic<Args...>::pointerCount;
        static constexpr uint32_t classCount = ArgumentStatistic<Args...>::classCount;
        static constexpr uint32_t count = sizeof...(Args);
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
    

    class EventSystem
    {
    public:
        using CallBackHandle = uint32_t;
        struct CallBackParam
        {
            const void* p = nullptr;
            //ruff runtime checks
            uint32_t paramCount = 0;
            uint32_t paramSize = 0;
            uint32_t pointerParamCount = 0;
            uint32_t classCount = 0;

            bool IsTypeValid(uint32_t count, uint32_t size, uint32_t pointer, uint32_t klass) const
            {
                return paramCount == count && paramSize == size && pointerParamCount == pointer && classCount == klass;
            }
        };

    public:
        static inline [[nodiscard]]EventSystem& Inst()
        {
            static EventSystem es;
            return es;
        }

        template <typename EventType, typename ...Args>
        void Send(EventType evtID, const void* sender, Args... args) const
        {
            static_assert(std::is_convertible_v<EventType, int> || std::is_enum_v<EventType>);
            const typename TupleTypeFromArgs<Args...>::TupleType evt(std::forward<Args>(args)...);
            const EventSystem::CallBackParam cbp{ .p = &evt, 
                .paramCount = sizeof...(Args), 
                .paramSize = ArgumentStatistic<Args...>::size, 
                .pointerParamCount = ArgumentStatistic<Args...>::pointerCount,
                .classCount = ArgumentStatistic<Args...>::classCount
            };
            Call((int)evtID, sender, &cbp);
        }

        template <typename EventType, typename ...Args>
        void SendAll(EventType evtID, Args... args) const
        {
            static_assert(std::is_convertible_v<EventType, int> || std::is_enum_v<EventType>);
            const typename TupleTypeFromArgs<Args...>::TupleType evt(std::forward<Args>(args)...);
            const EventSystem::CallBackParam cbp{ .p = &evt,
                .paramCount = sizeof...(Args),
                .paramSize = ArgumentStatistic<Args...>::size,
                .pointerParamCount = ArgumentStatistic<Args...>::pointerCount,
                .classCount = ArgumentStatistic<Args...>::classCount
            };
            Call((int)evtID, nullptr, &cbp);
        }

        /// <summary>
        /// listen to object sender's sent event only
        /// </summary>
        /// <typeparam name="EventType">any enum</typeparam>
        /// <typeparam name="F">any callerable</typeparam>
        /// <param name="evtID">event id</param>
        /// <param name="sender">the target object to listen to. pass nullptr is any object's event is wanted</param>
        /// <param name="f">any non member function, callables</param>
        /// <returns>the id of the registration</returns>
        template <typename EventType, typename F>
        CallBackHandle Register(EventType evtID, const void* sender, F&& f)
        {
            static_assert(std::is_convertible_v<EventType, int> || std::is_enum_v<EventType>);
            FnCallBack callBack = MakeCBStorage(std::forward<F>(f));
            return Reg((int)evtID, sender, nullptr, std::move(callBack));
        }

        /// <summary>
        /// listen to object sent event, with member function as callback
        /// </summary>
        template <typename EventType, typename RC, typename F>
        CallBackHandle Register(EventType evtID, const void* sender, const RC* recver, F&& f)
        {
            static_assert(std::is_convertible_v<EventType, int> || std::is_enum_v<EventType>);
            static_assert(std::is_class_v<RC>);
            FnCallBack cb = MakeCBStorage(recver, std::forward<F>(f));
            return Reg((int)evtID, sender, recver, std::move(cb));
        }

        /// <summary>
        /// unregister any event with sender or recver that is obj
        /// </summary>
        void Unregister(const void* ob);

        /// <summary>
        /// unregister any event with id which is return by Register
        /// </summary>
        /// <param name="id"></param>
        void Unregister(CallBackHandle id);
        void Clear();

    private:
        EventSystem();

        using FnCallBack = std::function<void(const CallBackParam*)>;

        CallBackHandle Reg(int evt, const void* sd, const void* rc, FnCallBack&& cb);
        void Call(int evt, const void* sd, const EventSystem::CallBackParam* args) const;

        template <typename F>
        static auto MakeCBStorage(F&& f)
        {
            return [f = std::forward<F>(f)](const CallBackParam* p)
                {
                    using TupleType = typename FunctionTraits<F>::TupleType;
                    const TupleType* eventBody = reinterpret_cast<const TupleType*>(p->p);
                    const uint32_t paramCount = FunctionTraits<F>::count;
                    const uint32_t paramSize = FunctionTraits<F>::size;
                    const uint32_t pointerParamCount = FunctionTraits<F>::pointerCount;
                    const uint32_t classParamCount = FunctionTraits<F>::classCount;

                    assert(p->IsTypeValid(paramCount, paramSize, pointerParamCount, classParamCount));
                    std::apply(f, *eventBody);
                };
        }

        template <typename OBJ, typename F>
        static auto MakeCBStorage(const OBJ* obj, F&& f)
        {
            return [f = std::forward<F>(f), obj = const_cast<OBJ*>(obj)](const CallBackParam* p)
                {
                    using TupleType = typename FunctionTraits<F>::TupleType;
                    const TupleType* eventBody = reinterpret_cast<const TupleType*>(p->p);

                    const uint32_t paramCount = FunctionTraits<F>::count;
                    const uint32_t paramSize = FunctionTraits<F>::size;
                    const uint32_t pointerParamCount = FunctionTraits<F>::pointerCount;
                    const uint32_t classParamCount = FunctionTraits<F>::classCount;

                    assert(p->IsTypeValid(paramCount, paramSize, pointerParamCount, classParamCount));
                    std::apply(std::bind_front(f, obj), *eventBody);
                };
        }

    private:
        friend struct EventSystemImp;
        EventSystem(const EventSystem&) = delete;
        void operator=(const EventSystem&) = delete;
        struct EventSystemImp* _imp;
    };

    static inline [[nodiscard]] EventSystem& ESI()
    {
        return EventSystem::Inst();
    }
}

