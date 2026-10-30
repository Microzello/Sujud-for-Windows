#pragma once

#include "sujud/PrayTimes.h"
#include "sujud/Timetable.h"

#include <optional>

namespace sujud::settings {

Calculator calculator();
void setMethod(Method method);
void setAsr(AsrJuristic asr);

std::optional<Coordinates> location();
void setLocation(Coordinates where);

std::optional<Timetable> timetable();

// Some PCs report the compass heading the wrong way round; the user corrects it once.
bool compassReversed();
void setCompassReversed(bool reversed);

bool fullScreenTimes();
void setFullScreenTimes(bool show);

}
