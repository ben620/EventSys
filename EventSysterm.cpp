//(C) benyuan 2024
//all rights reserved

module;

#include <map>
#include <forward_list>
#include <tuple>
#include <cassert>
#include <functional>


export module EventSystem;

export namespace es
{
    class EventSystem;
    [[nodiscard]] EventSystem& ESI();
}

namespace es
{
    using SizeType = unsigned int;
    template <typename... Args>
    struct TupleTypeFromArgs
    {
        using TupleType = std::tuple<std::add_lvalue_reference_t<std::add_const_t<std::decay_t<Args>>>...>;
    };

    template <typename ...Args>
    struct ArgumentStatistic
    {
        static constexpr SizeType size = (SizeType)(... + sizeof(std::decay_t<Args>));
        static constexpr SizeType pointerCount = (SizeType)(... + std::is_pointer_v<std::decay_t<Args>>);
        static constexpr SizeType classCount = (SizeType)(... + std::is_class_v<std::decay_t<Args>>);
    };

    template <>
    struct ArgumentStatistic<>
    {
        static constexpr SizeType size = 0;
        static constexpr SizeType pointerCount = 0;
        static constexpr SizeType classCount = 0;
    };

    template<typename T>
    struct FunctionTraits;

    template<typename ReturnType, typename... Args>
    struct FunctionTraits<ReturnType(Args...)>
    {
        using TupleType = typename TupleTypeFromArgs<Args...>::TupleType;
        static constexpr SizeType size = ArgumentStatistic<Args...>::size;
        static constexpr SizeType pointerCount = ArgumentStatistic<Args...>::pointerCount;
        static constexpr SizeType classCount = ArgumentStatistic<Args...>::classCount;
        static constexpr SizeType count = sizeof...(Args);
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
        using CallBackHandle = SizeType;
        struct CallBackParam
        {
            const void* p = nullptr;
            //ruff runtime checks
            SizeType paramCount = 0;
            SizeType paramSize = 0;
            SizeType pointerParamCount = 0;
            SizeType classCount = 0;

            bool IsTypeValid(SizeType count, SizeType size, SizeType pointer, SizeType klass) const
            {
                return paramCount == count && paramSize == size && pointerParamCount == pointer && classCount == klass;
            }
        };

    public:
        [[nodiscard]] static inline EventSystem& Inst()
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
        EventSystem() = default;

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
                    const SizeType paramCount = FunctionTraits<F>::count;
                    const SizeType paramSize = FunctionTraits<F>::size;
                    const SizeType pointerParamCount = FunctionTraits<F>::pointerCount;
                    const SizeType classParamCount = FunctionTraits<F>::classCount;

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

                    const SizeType paramCount = FunctionTraits<F>::count;
                    const SizeType paramSize = FunctionTraits<F>::size;
                    const SizeType pointerParamCount = FunctionTraits<F>::pointerCount;
                    const SizeType classParamCount = FunctionTraits<F>::classCount;

                    assert(p->IsTypeValid(paramCount, paramSize, pointerParamCount, classParamCount));
                    std::apply(std::bind_front(f, obj), *eventBody);
                };
        }

    private:
        struct EventSystemImp
        {
            struct CallBackInfo
            {
                FnCallBack cb;
                CallBackHandle id = 0;
            };

            using EvtCallBackInfo = std::map<const void*, std::map<const void*, std::forward_list<CallBackInfo>>>;
            std::map<int, EvtCallBackInfo> _listeners;
            EventSystem::CallBackHandle _id = 0;
        };

        EventSystem(const EventSystem&) = delete;
        void operator=(const EventSystem&) = delete;
        EventSystemImp _imp;
        
        static void inline DoCall(const EventSystemImp::EvtCallBackInfo& listeners
            , const void* sender, const EventSystem::CallBackParam* args);
    };

    [[nodiscard]] EventSystem& ESI()
    {
        return EventSystem::Inst();
    }

    void EventSystem::Unregister(const void* ob)
    {
        for (auto& l : _imp._listeners)
        {
            l.second.erase(ob);
            for (auto& item : l.second)
            {
                item.second.erase(ob);
            }
        }
    }

    void EventSystem::Unregister(CallBackHandle id)
    {
        for (auto& l : _imp._listeners)
        {
            for (auto& snd : l.second)
            {
                for (auto& rcv : snd.second)
                {
                    std::erase_if(rcv.second, [id](const auto& cbi) { return cbi.id == id; });
                }
            }
        }
    }

    void EventSystem::Clear()
    {
        _imp._listeners.clear();
    }


    EventSystem::CallBackHandle EventSystem::Reg(int evt, const void* sd, const void* rc, FnCallBack&& cb)
    {
        _imp._listeners[evt][sd][rc].push_front(EventSystemImp::CallBackInfo{ std::move(cb), ++_imp._id });
        return _imp._id;
    }

    void inline EventSystem::DoCall(const EventSystemImp::EvtCallBackInfo& listeners, const void* sender, const EventSystem::CallBackParam* args)
    {
        if (auto recvers = listeners.find(sender); recvers != listeners.end())
        {
            for (auto& item : recvers->second)
            {
                for (auto& cbi : item.second)
                {
                    cbi.cb(args);
                }
            }
        }
    }

    void EventSystem::Call(int evtID, const void* sender, const EventSystem::CallBackParam* args) const
    {
        auto evtPairs = _imp._listeners.find(evtID);
        if (evtPairs == _imp._listeners.end())
        {
            return;
        }

        DoCall(evtPairs->second, sender, args);

        //sender is not null, send to both null listeners and obj listeners
        if (sender != nullptr)
        {
            DoCall(evtPairs->second, nullptr, args);
        }
    }
}
