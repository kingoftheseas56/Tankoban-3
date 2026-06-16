// Tankoban 3 - StreamParser (Slice 5).
// Parses a raw Stream into a ParsedStream by analyzing the name/title/filename
// text. Recreated from Harbor's src/lib/streams/parser/ subsystem.
#pragma once

#include "core/StreamModels.h"

namespace tankoban {

class StreamParser {
public:
    static ParsedStream parseStream(const Stream& stream);

    StreamParser() = delete;
};

} // namespace tankoban
