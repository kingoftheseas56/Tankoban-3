#pragma once
#include <QString>

// fmtTime — "M:SS" under an hour, "H:MM:SS" at/over; NaN/negative -> "0:00".
// Ports Harbor's transport-utils.ts fmtTime 1:1 (spec §7.1).
QString fmtTime(double sec);
