// An item's description, split into its blocks (tags, values, properties,
// description text).

#include "engine_reads_items_internal.h"

#include "log.h"

namespace acc::engine {

namespace {

typedef CExoString* (__thiscall* PFN_GetPropertyDescription)(void* this_,
                                                              CExoString* out);

typedef void* (__thiscall* PFN_GetBaseItem)(void* item);

// One of the engine's per-category property builders.
void AddProps(uintptr_t builder, void* item, CExoString* acc) {
    reinterpret_cast<PFN_AddItemProperty>(builder)(item, acc);
}

// Read item_type / weapon_type via CSWItem::GetBaseItem (the CSWItem subobject
// is at offset 0 of CSWSItem, so the item pointer is a valid `this`). Offsets
// are CMP-verified from the GetPropertyDescription disassembly.
bool ReadBaseItemFlags(void* item, uint8_t& itemType, uint8_t& weaponType) {
    __try {
        auto fn = reinterpret_cast<PFN_GetBaseItem>(kAddrCSWItemGetBaseItem);
        auto* base = reinterpret_cast<unsigned char*>(fn(item));
        if (!base) return false;
        itemType   = *(base + kBaseItemItemTypeOffset);
        weaponType = *(base + kBaseItemWeaponTypeOffset);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Append the engine's own per-category builders to ONE accumulator, in the same
// order and with the same guards GetPropertyDescription uses, recording
// the visible (strlen) length after each section. The result is the byte offsets
// at which Tags / Values / Properties end inside the canonical
// GetPropertyDescription string — so the caller can slice that string rather
// than re-emit each section (separate accumulators diverge from the canonical
// text by a byte or two; one cumulative accumulator matches it exactly). The
// accumulator's heap c_string is leaked (CRT-mismatch rule). offTags/offValues/
// offProps are left at their incoming values on fault.
//
// The cascade is PER GAME (each replicates its own engine's decompiled
// sequence — a mismatched replica trips the caller's divergence guard and
// degrades to description-only):
//   K1 (0x0055F340): FeatReq | weapon?{Damage,Range,Crit,OnHit,Size} |
//     AttackMod, Defence | Misc.
//   K2 (0x00607790): NEW header, FeatReq | NEW x4 | (weapon || base 0x2d)?
//     {Damage, Range, [Crit unless base 0x2d], OnHit, Size} | AttackMod,
//     Defence | Misc. The lightsaber base id 0x2d forces the weapon block
//     (minus the crit builder) even with weapon_type == 0.
void ComputeSectionOffsets(void* item, uint8_t weaponType,
                           size_t& offTags, size_t& offValues,
                           size_t& offProps) {
    const bool k2 = acc::game::IsKotor2();
    int baseItemId = -1;
    CExoString acc = {nullptr, 0};
    __try {
        if (k2) {
            baseItemId = *reinterpret_cast<int*>(
                reinterpret_cast<unsigned char*>(item) +
                kSwsItemBaseItemIdOffset);
        }
        reinterpret_cast<PFN_CExoStringCtor>(kAddrCExoStringDefaultCtor)(&acc);

        if (k2) {
            AddProps(kAddrItemAddK2StatsHeader, item, &acc);
        }
        AddProps(kAddrItemAddFeatRequirements, item, &acc);
        offTags = acc.c_string ? strlen(acc.c_string) : 0;

        if (k2) {
            AddProps(kAddrItemAddK2Extra1, item, &acc);
            AddProps(kAddrItemAddK2Extra2, item, &acc);
            AddProps(kAddrItemAddK2Extra3, item, &acc);
            AddProps(kAddrItemAddK2Extra4, item, &acc);
        }
        const bool saber = k2 && baseItemId == 0x2d;
        if (weaponType != 0 || saber) {
            AddProps(kAddrItemAddDamageProperties, item, &acc);
            AddProps(kAddrItemAddRangeProperties, item, &acc);
            if (!saber) {
                AddProps(kAddrItemAddCriticalThreatProps, item, &acc);
            }
            AddProps(kAddrItemAddOnHitProperties, item, &acc);
            AddProps(kAddrItemAddWeaponSizeProperties, item, &acc);
        }
        AddProps(kAddrItemAddAttackModifierProps, item, &acc);
        AddProps(kAddrItemAddDefenceProperties, item, &acc);
        offValues = acc.c_string ? strlen(acc.c_string) : offTags;

        AddProps(kAddrItemAddMiscellaneousProps, item, &acc);
        offProps = acc.c_string ? strlen(acc.c_string) : offValues;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write("Engine.Reads",
                      "ComputeSectionOffsets SEH for item=%p", item);
    }
}

// Resolve the item's identified-description text through the TLK strref,
// bypassing the (sometimes corrupt) inline CExoLocString substring that
// GetPropertyDescription/GetString prefer. Returns false when there's no strref
// (-1 sentinel) or the lookup fails / is empty. outStrref reports the strref for
// logging.
bool ReadItemDescriptionViaTlk(void* item, char* outBuf, size_t bufSize,
                               uint32_t& outStrref) {
    outStrref = 0xFFFFFFFF;
    if (!item || !outBuf || bufSize < 2) return false;
    if (!TryReadU32(item, kItemDescriptionLocStringOffset + kExoLocStringStrRefOffset,
                    &outStrref)) {
        outStrref = 0xFFFFFFFF;
        return false;
    }
    if (outStrref == 0xFFFFFFFF) return false;  // no TLK entry for this locstring
    return LookupTlk(outStrref, outBuf, bufSize) && outBuf[0] != '\0';
}

// Copy src[start..end) into dst (NUL-terminated, bounded by cap).
void CopySlice(char* dst, size_t cap, const char* src, size_t start, size_t end) {
    if (!dst || cap == 0) return;
    if (end < start) end = start;
    size_t n = end - start;
    if (n >= cap) n = cap - 1;
    if (n) memcpy(dst, src + start, n);
    dst[n] = '\0';
}

}  // namespace

bool ReadItemPropertyDescription(void* item, char* outBuf, size_t bufSize) {
    if (!item || !outBuf || bufSize < 2) return false;
    CExoString tmp = {nullptr, 0};
    bool ok = false;
    __try {
        auto fn = reinterpret_cast<PFN_GetPropertyDescription>(
            kAddrCSWSItemGetPropertyDescription);
        fn(item, &tmp);
        if (tmp.c_string && tmp.length > 0 && tmp.length < bufSize) {
            memcpy(outBuf, tmp.c_string, tmp.length);
            outBuf[tmp.length] = '\0';
            ok = true;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        acclog::Write("Engine.Reads",
                      "GetPropertyDescription SEH for item=%p", item);
        ok = false;
    }
    return ok;
}

bool BuildItemDescriptionBlocks(void* item, ItemDescriptionBlocks* out) {
    if (!item || !out) return false;
    out->tags[0] = out->values[0] = out->properties[0] =
        out->description[0] = '\0';

    // Canonical text — exactly what the game renders. Every spoken block is a
    // slice of THIS string; we never re-emit a section, so the bytes (and their
    // CP-1252 encoding) always match the game.
    char full[8192];
    if (!ReadItemPropertyDescription(item, full, sizeof(full))) return false;
    size_t fullLen = strlen(full);

    uint8_t itemType = 0, weaponType = 0;
    bool haveFlags = ReadBaseItemFlags(item, itemType, weaponType);

    // Find where Tags / Values / Properties end inside `full` by replaying the
    // engine's builder sequence into one accumulator. Crystals (0x2e) and
    // grenades (6) skip the whole property block — as does a missing base item —
    // so all offsets stay 0 and the entire string is the description. KOTOR 2's
    // twin adds a third skip type (0x31), replicated per game.
    size_t offTags = 0, offValues = 0, offProps = 0;
    bool engineSkips = itemType == 0x2e || itemType == 6 ||
                       (acc::game::IsKotor2() && itemType == 0x31);
    if (haveFlags && !engineSkips) {
        ComputeSectionOffsets(item, weaponType, offTags, offValues, offProps);
    }

    // Guard against any divergence from `full` (offsets past the string, or out
    // of order): fall back to the whole string as the description.
    if (offTags > offValues || offValues > offProps || offProps > fullLen) {
        acclog::Write("Engine.Reads",
                      "BuildItemDescriptionBlocks item=%p offsets diverged "
                      "(tags=%zu values=%zu props=%zu fullLen=%zu); "
                      "description-only fallback",
                      item, offTags, offValues, offProps, fullLen);
        offTags = offValues = offProps = 0;
    }

    CopySlice(out->tags,       sizeof(out->tags),       full, 0,         offTags);
    CopySlice(out->values,     sizeof(out->values),     full, offTags,   offValues);
    CopySlice(out->properties, sizeof(out->properties), full, offValues, offProps);

    // Description: prefer the TLK strref (clean cp1252) over the inline copy that
    // GetPropertyDescription emits, which some items carry corrupt (accented
    // letters collapsed to 0xFD). Without a usable strref, the tail of the
    // GetPropertyDescription text.
    uint32_t descStrref = 0xFFFFFFFF;
    bool descViaTlk = ReadItemDescriptionViaTlk(item, out->description,
                                                sizeof(out->description),
                                                descStrref);
    // No strref and no English substring: the engine's tail is
    // only its "no description" placeholder, so take the text the item does
    // carry in another language (English-only mod items).
    bool descOtherLang = false;
    if (!descViaTlk) {
        descOtherLang = ReadLocStringOtherLanguage(
            item, kItemDescriptionLocStringOffset,
            out->description, sizeof(out->description));
    }
    if (!descViaTlk && !descOtherLang) {
        CopySlice(out->description, sizeof(out->description), full, offProps,
                  fullLen);
    }

    acclog::Write("Engine.Reads",
                  "BuildItemDescriptionBlocks item=%p itemType=%u weaponType=%u "
                  "fullLen=%zu offsets(tags=%zu values=%zu props=%zu) "
                  "descStrref=0x%x descViaTlk=%d descOtherLang=%d "
                  "lens(tags=%zu values=%zu props=%zu desc=%zu)",
                  item, (unsigned)itemType, (unsigned)weaponType, fullLen,
                  offTags, offValues, offProps, descStrref, descViaTlk ? 1 : 0,
                  descOtherLang ? 1 : 0,
                  strlen(out->tags), strlen(out->values),
                  strlen(out->properties), strlen(out->description));
    return fullLen > 0 || out->description[0] != '\0';
}

}  // namespace acc::engine
