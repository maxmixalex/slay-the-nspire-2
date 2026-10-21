#ifndef NSP_DECODER_ONLY_STRING_H
#define NSP_DECODER_ONLY_STRING_H
/* Cross-only declaration, resolved by the project's existing freestanding memcpy. */
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
void *memcpy(void *destination, const void *source, size_t count);
#ifdef __cplusplus
}
#endif
#endif
