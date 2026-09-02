#pragma once
#include <functional>
#include <vector>

template<typename... Args>
class Event {
public:
    using Handler = std::function<void(Args...)>;
    
    void AddListener(Handler handler) {
        handlers.push_back(std::move(handler));
    }
    
    void Invoke(Args... args) {
        for (auto& handler : handlers) {
            handler(args...);
        }
    }
    
private:
    std::vector<Handler> handlers;
};
