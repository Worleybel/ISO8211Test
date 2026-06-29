/******************************************************************************
 *
 * Generic ISO 8211 (DDF) reader for the standalone sample project.
 *
 * Usage: iso8211_test <chart_file>
 *
 * Parses any ISO 8211 file (e.g. S-57 ENC charts). Does not assume any
 * specific field tag or subfield name. For each file it prints:
 *   1. A DDR overview: every field definition and its subfields/formats.
 *   2. A full per-record dump: every field, every subfield value (typed
 *      extraction via ExtractInt/Float/StringData).
 *
 * Exit code 0 on success, non-zero if the file cannot be opened/parsed.
 *
 *****************************************************************************/

#include "iso8211.h"

#include <cstdio>
#include <cstring>
#include <string>

namespace
{
// Trim trailing spaces/terminators from a fixed-width DDF string so the
// printed value is readable. The caller owns the data within poField, so
// trimming into a local std::string is safe.
std::string TrimField(const char *data, int len)
{
    int end = len;
    while (end > 0)
    {
        const char c = data[end - 1];
        if (c == ' ' || c == '\0')
            --end;
        else
            break;
    }
    return std::string(data, end);
}

// Dump one subfield value using the type-appropriate extractor. Advances the
// data pointer / bytes-remaining by the number of consumed bytes.
void DumpSubfield(const DDFSubfieldDefn *sf, const char *&pachData,
                  int &nBytesRemaining)
{
    int nConsumed = 0;
    std::printf("        %-12s [%s] = ", sf->GetName(), sf->GetFormat());

    switch (sf->GetType())
    {
        case DDFInt:
        {
            const int v =
                sf->ExtractIntData(pachData, nBytesRemaining, &nConsumed);
            std::printf("%d (int)\n", v);
            break;
        }
        case DDFFloat:
        {
            const double v = sf->ExtractFloatData(pachData, nBytesRemaining,
                                                  &nConsumed);
            std::printf("%g (float)\n", v);
            break;
        }
        case DDFString:
        {
            const char *s = sf->ExtractStringData(pachData, nBytesRemaining,
                                                  &nConsumed);
            std::printf("'%s' (string)\n", TrimField(s, nConsumed).c_str());
            break;
        }
        case DDFBinaryString:
        default:
        {
            // Binary: print length + hex preview of the first few bytes.
            const char *s = sf->ExtractStringData(pachData, nBytesRemaining,
                                                  &nConsumed);
            const int show = nConsumed < 16 ? nConsumed : 16;
            std::printf("<binary, %d bytes> ", nConsumed);
            for (int i = 0; i < show; ++i)
                std::printf("%02X",
                            static_cast<unsigned char>(s[i]));
            if (nConsumed > show)
                std::printf("...");
            std::printf("\n");
            break;
        }
    }

    if (nConsumed < 0)
        nConsumed = 0;
    pachData += nConsumed;
    nBytesRemaining -= nConsumed;
    if (nBytesRemaining < 0)
        nBytesRemaining = 0;
}

void DumpField(const DDFField *poField)
{
    const DDFFieldDefn *poDefn = poField->GetFieldDefn();
    std::printf("    Field '%s' (%s): %d bytes, repeat=%d\n",
                poDefn->GetName(),
                poDefn->GetDescription() ? poDefn->GetDescription() : "",
                poField->GetDataSize(), poField->GetRepeatCount());

    const char *pachData = poField->GetData();
    int nBytesRemaining = poField->GetDataSize();
    const int nRepeat = poField->GetRepeatCount();
    const auto &subfields = poDefn->GetSubfields();

    for (int iRepeat = 0; iRepeat < nRepeat; ++iRepeat)
    {
        if (subfields.empty())
            break;
        if (nRepeat > 1)
            std::printf("      [occurrence %d]\n", iRepeat);
        for (const auto &sf : subfields)
        {
            if (nBytesRemaining <= 0)
                break;
            DumpSubfield(sf.get(), pachData, nBytesRemaining);
        }
    }
}
}  // namespace

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::fprintf(stderr, "Usage: %s <chart_file>\n", argv[0]);
        return 1;
    }

    const char *pszFile = argv[1];
    DDFModule oModule;
    if (!oModule.Open(pszFile))
    {
        std::fprintf(stderr, "FAIL: DDFModule::Open('%s') failed\n", pszFile);
        return 1;
    }

    // ----------------------------------------------------------------------
    // DDR overview: field definitions + subfields/formats.
    // ----------------------------------------------------------------------
    std::printf("==== DDR overview: %s ====\n", pszFile);
    std::printf("Module: interchange=%c leader=%c fieldControlLen=%d "
                "sizeFieldTag=%d sizeFieldLen=%d sizeFieldPos=%d\n",
                oModule.GetInterchangeLevel(), oModule.GetLeaderIden(),
                oModule.GetFieldControlLength(), oModule.GetSizeFieldTag(),
                oModule.GetSizeFieldLength(), oModule.GetSizeFieldPos());
    std::printf("Field definitions: %d\n", oModule.GetFieldCount());

    for (int i = 0; i < oModule.GetFieldCount(); ++i)
    {
        const DDFFieldDefn *poDefn = oModule.GetField(i);
        std::printf("  [%2d] tag='%s' name='%s' struct=%d type=%d",
                    i, poDefn->GetName(), poDefn->GetDescription(),
                    static_cast<int>(poDefn->GetDataStructCode()),
                    static_cast<int>(poDefn->GetDataTypeCode()));
        if (poDefn->IsRepeating())
            std::printf(" (repeating)");
        std::printf("\n");
        if (!poDefn->GetFormatControls()[0])
            std::printf("        format: %s\n", poDefn->GetFormatControls());
        for (const auto &sf : poDefn->GetSubfields())
            std::printf("        %-12s format=%s type=%d\n", sf->GetName(),
                        sf->GetFormat(), static_cast<int>(sf->GetType()));
    }
    std::printf("\n");

    // ----------------------------------------------------------------------
    // Data records: full per-field, per-subfield dump.
    // ----------------------------------------------------------------------
    std::printf("==== Data records ====\n");
    int nRecordCount = 0;
    DDFRecord *poRecord = nullptr;
    while ((poRecord = oModule.ReadRecord()) != nullptr)
    {
        std::printf("Record %d: %d field(s), %d bytes\n", nRecordCount,
                    poRecord->GetFieldCount(), poRecord->GetDataSize());
        for (int iField = 0; iField < poRecord->GetFieldCount(); ++iField)
            DumpField(poRecord->GetField(iField));
        std::printf("\n");
        ++nRecordCount;
    }

    std::printf("==== Summary: read %d record(s) ====\n", nRecordCount);
    oModule.Close();
    return 0;
}
