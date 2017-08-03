// From serialize.c directly:
#define REFSXP            255
#define NILVALUE_SXP      254
#define GLOBALENV_SXP     253
#define UNBOUNDVALUE_SXP  252
#define MISSINGARG_SXP    251
#define BASENAMESPACE_SXP 250
#define NAMESPACESXP      249
#define PACKAGESXP        248
#define PERSISTSXP        247
/* the following are speculative--we may or may not need them soon */
#define CLASSREFSXP       246
#define GENERICREFSXP     245
#define BCREPDEF          244
#define BCREPREF          243
#define EMPTYENV_SXP      242
#define BASEENV_SXP       241

#define IS_OBJECT_BIT_MASK (1 << 8)
#define HAS_ATTR_BIT_MASK (1 << 9)
#define HAS_TAG_BIT_MASK (1 << 10)
#define DECODE_LEVELS(v) ((v) >> 12)
#define DECODE_TYPE(v) ((v) & 255)

// From Defn.h
#define BYTES_MASK (1<<1)
#define LATIN1_MASK (1<<2)
#define UTF8_MASK (1<<3)
