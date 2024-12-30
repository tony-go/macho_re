#include <stdint.h>

/*
 * This whole file is a shim for the structures in cs_blobs.h
 * Ref:
 * https://github.com/apple-oss-distributions/xnu/blob/main/osfmk/kern/cs_blobs.h
 *
 */

#define CSMAGIC_EMBEDDED_SIGNATURE 0xfade0cc0
#define CSMAGIC_EMBEDDED_ENTITLEMENTS 0xfade7171
#define CS_RUNTIME 0x00010000 /* Apply hardened runtime policies */

enum {
  CSSLOT_CODEDIRECTORY = 0,
  CSSLOT_REQUIREMENTS = 2,
  CSSLOT_ENTITLEMENTS = 5,
};

typedef struct {
  uint32_t type;   /* type of entry */
  uint32_t offset; /* offset of entry */
} CS_BlobIndex_shim;

typedef struct {
  uint32_t magic;            /* magic number */
  uint32_t length;           /* total length of SuperBlob */
  uint32_t count;            /* number of index entries following */
  CS_BlobIndex_shim index[]; /* (count) entries */
} CS_SuperBlob_shim;

typedef struct {
  uint32_t magic;
  uint32_t length;
  char data[];
} CS_GenericBlob_shim;

typedef struct {
  uint32_t magic;   /* magic number (CSMAGIC_CODEDIRECTORY) */
  uint32_t length;  /* total length of CodeDirectory blob */
  uint32_t version; /* compatibility version */
  uint32_t flags;
} CS_CodeDirectory_shim;
