#include <tuple>
#include <map>
#include <forward_list>
#include <array>
#include <functional>
#include <tuple>
#include <iostream>


enum class EventID
{
    NewJob,
    Max
};

class ObjectBase
{
    
};

class EventBase
{
public:
    ~EventBase() = default;
    virtual const void* Data() const
    {
        return nullptr;
    }
};

template <typename... Args>
class TEvent : public EventBase
{
public:
    TEvent() = default;
    TEvent(Args... args)
        : value(std::forward<Args>(args)...)
    {
        std::cout << 1 << typeid(value).name() << std::endl;
    }
    
    const void* Data() const
    {
        return &value;
    }

    using TupleType = std::tuple<Args...>;

private:
    TupleType value;
};

template <typename C, typename... Args>
auto MakeCallBack(ObjectBase *obj, void (C::*f)(Args...))
{
    using FnType = void (C::*)(Args... args);
    return [f = std::forward<FnType>(f), obj](const EventBase &e)
        {
            using TupleType = typename TEvent<Args...>::TupleType;
            const TupleType* eventBody = reinterpret_cast<const TupleType*>(e.Data());
            std::cout << 2 << typeid(eventBody).name() << std::endl;
            std::apply(std::bind_front(f, static_cast<C*>(obj)), *eventBody);
        };
}


class EventSystem
{
public:
    template <typename ...Args>
    void Send(EventID evtID, ObjectBase* sender, Args... args)
    {
        auto recvers = _listeners[(int)evtID].find(sender);
        if (recvers == _listeners[(int)evtID].end())
        {
            return;
        }

        const TEvent evt(std::forward<Args>(args)...);

        std::cout << 3 << typeid(evt).name() << std::endl;

        for (auto& item : recvers->second)
        {
            for (auto& cb : item.second)
            {
                cb(evt);
            }
        }
    }
    
    template <typename C, typename ...Args>
    void Register(EventID evtID, ObjectBase* sender, ObjectBase* recver, void (C::*f)(Args...))
    {
        FnCallBack callBack = MakeCallBack(recver, f);
        _listeners[(int)evtID][sender][recver].push_front(callBack);
    }

private:
    using FnCallBack = std::function<void(const EventBase& e)>;

    std::array<std::map<ObjectBase*, std::map<ObjectBase*, std::forward_list<FnCallBack>>>, (int)EventID::Max> _listeners;
};



class Svr : public ObjectBase
{
public:
    void OnReady(int code, const std::string& b)
    {
        std::cout << "read " << code << " " << b << std::endl;
    }
};

int main()
{
    Svr s;
    Svr r;
    

    EventSystem es;
    es.Register(EventID::NewJob, &s, &r, &Svr::OnReady);

    const std::string b("abc");
    es.Send(EventID::NewJob, &s, 1, b);

    return 0;
}