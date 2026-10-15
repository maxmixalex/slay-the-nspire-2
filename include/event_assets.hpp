#pragma once
namespace spire {
// NSP2/10 appends one eight-string record per EventId, preserving old indexes.
constexpr int EventTextBase=228, EventTextStride=8, AromaSilentText=268, EventTextEnd=269;
// Record: title, intro, outcome A, outcome B, option A title/body, option B title/body.
}
