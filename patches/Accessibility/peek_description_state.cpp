// The state the peek_description files share, defined in one place; each item is
// described where it is declared, in peek_description_internal.h.

#include "peek_description_internal.h"

namespace acc::peek {

int g_blockIdx = kCursorReset;
void* g_blockCacheItem = nullptr;
acc::engine::ItemDescriptionBlocks g_blocks;
const char* g_blockPtrs[4] = {nullptr, nullptr, nullptr, nullptr};
int g_blockCount = 0;

}  // namespace acc::peek
