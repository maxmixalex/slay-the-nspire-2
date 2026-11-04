#ifndef NSP_DECODER_ONLY_ERRNO_H
#define NSP_DECODER_ONLY_ERRNO_H
/* The two decoder translation units use no errno constants or services.
   7zTypes.h includes this header for unrelated filesystem helper macros.
   This empty compatibility header is only on the decoder-only cross-compilation path. */
#endif
