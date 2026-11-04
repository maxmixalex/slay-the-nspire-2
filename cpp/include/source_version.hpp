#pragma once
#include <stddef.h>

namespace spire {
inline constexpr char PortVersion[] = "0.1.0";
inline constexpr char PortVersionLabel[] = "nSpire 0.1.0";
inline constexpr char SourceVersionFilename[] = "nSpire2.version.tns";
inline constexpr unsigned SourceVersionPayloadCapacity = 40;

// Optional conversion metadata. Invalid/missing metadata clears any previous
// value, leaving the port version visible without inventing a game version.
// Input is exactly "NSVR1\n" + a dotted numeric version + "\n".
bool set_source_version(const char* payload, size_t size);
const char* version_label();
} // namespace spire
