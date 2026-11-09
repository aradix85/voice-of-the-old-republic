#include "menus_speak.h"

#include <cstdarg>
#include <cstdio>

#include "log.h"
#include "speech.h"

namespace acc::menus::speak {

void SpeakChoice(const char* tag, const char* label,
                 const char* ctxFmt, ...) {
    char ctx[128];
    va_list ap;
    va_start(ap, ctxFmt);
    std::vsnprintf(ctx, sizeof(ctx), ctxFmt, ap);
    va_end(ap);
    if (!label || label[0] == '\0') {
        acclog::Write(tag, "speak %s -> empty", ctx);
        return;
    }
    acc::speech::Speak(label, /*interrupt=*/true);
    acclog::Write(tag, "speak %s [%s]", ctx, label);
}

}  // namespace acc::menus::speak
