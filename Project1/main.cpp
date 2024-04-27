#include <tuple>
#include <map>
#include <forward_list>
#include <array>
#include <functional>
#include <tuple>
#include <type_traits>
#include <iostream>


enum class EventID
{
    NewJob,
    OnVoid,
    Max
};

class ObjectBase;


class EventBase
{
public:
    ~EventBase() = default;
    virtual const void* Data() const
    {
        return nullptr;
    }
    virtual std::size_t Size() const
    {
        return 0;
    }
};

template <typename... Args>
class TEvent : public EventBase
{
public:
    TEvent(Args... args) noexcept
        : value(std::forward<Args>(args)...)
    {
    }
    
    const void* Data() const override
    {
        return &value;
    }


    std::size_t Size() const override
    {
        return sizeof(value);
    }

    using TupleType = std::tuple<Args...>;

private:
    TupleType value;
};



class EventSystem
{
public:
    static EventSystem& Inst()
    {
        static EventSystem es;
        return es;
    }
    template <typename ...Args>
    void Send(EventID evtID, const void* sender, Args... args)
    {
        auto recvers = _listeners[(int)evtID].find(sender);
        if (recvers == _listeners[(int)evtID].end())
        {
            return;
        }

        const TEvent<std::add_lvalue_reference_t<std::add_const_t<std::decay_t<Args>>>...> evt(args...);

        for (auto& item : recvers->second)
        {
            for (auto& cbi : item.second)
            {
                cbi.cb(evt);
            }
        }
    }


    template <typename C, typename ...Args>
    std::size_t Register(EventID evtID, const void* sender, C* recver, void (C::*f)(Args...))
    {
        FnCallBack callBack = MakeCallBack(recver, f);
        const std::size_t id = NewID();
        _listeners[(int)evtID][sender][recver].push_front(CallBackInfo{ std::move(callBack), id});
        return id;
    }


    void Unregister(const void* ob)
    {
        for (auto& l : _listeners)
        {
            l.erase(ob);
            for (auto& item : l)
            {
                item.second.erase(ob);
            }
        }
    }

    void Unregister(std::size_t id)
    {
        for (auto& l : _listeners)
        {
            for (auto& snd : l)
            {
                for (auto& rcv : snd.second)
                {
                    std::erase_if(rcv.second, [id](const CallBackInfo& cbi) { return cbi.id == id; });
                }
            }
        }
    }

private:

    

    template <typename C, typename... Args>
    static auto MakeCallBack(C* obj, void (C::* f)(Args...))
    {
        using FnType = void (C::*)(Args... args);
        return [f = std::forward<FnType>(f), obj](const EventBase& e)
            {
                if constexpr (sizeof...(Args) > 0)
                {
                    using TupleType = std::tuple<std::add_lvalue_reference_t<std::add_const_t<std::decay_t<Args>>>...>;
                    const TupleType* eventBody = reinterpret_cast<const TupleType*>(e.Data());
                    std::apply(std::bind_front(f, obj), *eventBody);
             
                }
                else
                {
                    std::invoke(f, obj);
                }
            };
    }

    using FnCallBack = std::function<void(const EventBase& e)>;
    
    EventSystem() = default;

    struct CallBackInfo
    {
        FnCallBack cb;
        std::size_t id = 0;
    };

    std::size_t NewID()
    {
        return ++_id;
    }

private:
    std::array<std::map<const void*, std::map<const void*, std::forward_list<CallBackInfo>>>, (int)EventID::Max> _listeners;
    std::size_t _id = 0;
};

class ObjectBase
{
public:
    virtual ~ObjectBase() 
    {
        EventSystem::Inst().Unregister(this);
    }
};


class Svr : public ObjectBase
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
};

int main()
{
    Svr s;
    Svr r;
    

    EventSystem::Inst().Register(EventID::NewJob, &s, &r, &Svr::OnReady);
    EventSystem::Inst().Register(EventID::OnVoid, &s, &r, &Svr::OnVoid);

    EventSystem::Inst().Send(EventID::NewJob, &s, 1, std::string("abc"));
    EventSystem::Inst().Send(EventID::OnVoid, &s);


    return 0;
}