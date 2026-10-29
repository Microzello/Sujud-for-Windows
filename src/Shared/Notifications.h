#pragma once

namespace sujud::notifications {

// Without force, does nothing while the queue matches the settings and runs well ahead.
void reschedule(bool force = false);

}
