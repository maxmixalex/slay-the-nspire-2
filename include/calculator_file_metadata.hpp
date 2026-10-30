#pragma once
#include <stddef.h>
#include <stdint.h>

namespace spire {
// Exact nuc_stat ABI from the official Ndless r2022 nucleus.h. This is the
// OS structure, not the host/newlib struct stat (whose layout is different).
struct CalculatorFileMetadata {
    uint16_t device;
    uint32_t inode,mode;
    uint16_t links,uid,gid,rdevice;
    uint32_t size,accessed,modified,changed;
};
static_assert(sizeof(CalculatorFileMetadata)==36&&offsetof(CalculatorFileMetadata,size)==20,
              "Keep the official ARM32 nuc_stat layout");
}
