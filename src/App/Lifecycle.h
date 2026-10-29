#pragma once

#include <cstddef>
#include <functional>

namespace sujud::lifecycle {

// Call on the UI thread.
void start();
void settingsChanged();

std::size_t subscribe(std::function<void()> handler);
void unsubscribe(std::size_t token);

}
