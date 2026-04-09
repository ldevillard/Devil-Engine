#pragma once

#include <functional>
#include <vector>
#include <algorithm>
#include <cstddef>

template<typename... Args>
class Event
{
public:
    using Callback = std::function<void(Args...)>;
    using ListenerID = std::size_t;

    ListenerID AddListener(const Callback& callback)
    {
        ListenerID id = nextID++;
        listeners.push_back({ id, callback });
        return id;
    }

    void RemoveListener(ListenerID id)
    {
        listeners.erase(
            std::remove_if(listeners.begin(), listeners.end(),
                [id](const Listener& listener)
                {
                    return listener.id == id;
                }),
            listeners.end());
    }

    void Broadcast(Args... args)
    {
        for (Listener& listener : listeners)
        {
            listener.callback(args...);
        }
    }

    void Clear()
    {
        listeners.clear();
    }

private:
    struct Listener
    {
        ListenerID id;
        Callback callback;
    };

    std::vector<Listener> listeners;
    ListenerID nextID = 0;
};