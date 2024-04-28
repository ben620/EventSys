//(C) benyuan 2024
//all rights reserved
#include "EventSystem.hpp"
#include <tuple>
#include <map>
#include <forward_list>
#include <type_traits>

namespace es
{
    struct EventSystemImp
    {
        struct CallBackInfo
        {
            EventSystem::FnCallBack cb;
            EventSystem::CallBackHandle id = 0;
        };

        std::map<int, std::map<const void*, std::map<const void*, std::forward_list<CallBackInfo>>>> _listeners;
        EventSystem::CallBackHandle _id = 0;
    };

    EventSystemImp imp;


    EventSystem::EventSystem()
        : _imp(&imp)
    {

    }

    void EventSystem::Unregister(const void* ob)
    {
        for (auto& l : _imp->_listeners)
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
        for (auto& l : _imp->_listeners)
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
        _imp->_listeners.clear();
    }


    EventSystem::CallBackHandle EventSystem::Reg(int evt, const void* sd, const void* rc, FnCallBack&& cb)
    {
        _imp->_listeners[evt][sd][rc].push_front(EventSystemImp::CallBackInfo{ std::move(cb), ++_imp->_id });
        return _imp->_id;
    }

    static void inline DoCall(std::map<const void*, std::map<const void*, std::forward_list<EventSystemImp::CallBackInfo>>>& listeners
        , const void* sender, const EventSystem::CallBackParam* args)
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
        auto evtPairs = _imp->_listeners.find(evtID);
        if (evtPairs == _imp->_listeners.end())
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