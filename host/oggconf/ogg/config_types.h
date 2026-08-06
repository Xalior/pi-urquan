/*
 * ogg/config_types.h — the fixed-width integer types libogg expects its
 * build system to work out.
 *
 * Upstream ships include/ogg/config_types.h.in and lets configure or cmake
 * decide which header carries the fixed-width types and what to spell each
 * one. There is nothing to decide here: this port compiles against newlib,
 * which has <stdint.h>, and every type below is the exact-width one from
 * it. Written down rather than generated, and placed on the include path
 * rather than inside the submodule, which is never written to.
 */

#ifndef __CONFIG_TYPES_H__
#define __CONFIG_TYPES_H__

#define INCLUDE_INTTYPES_H 0
#define INCLUDE_STDINT_H 1
#define INCLUDE_SYS_TYPES_H 1

#if INCLUDE_INTTYPES_H
#  include <inttypes.h>
#endif
#if INCLUDE_STDINT_H
#  include <stdint.h>
#endif
#if INCLUDE_SYS_TYPES_H
#  include <sys/types.h>
#endif

typedef int16_t ogg_int16_t;
typedef uint16_t ogg_uint16_t;
typedef int32_t ogg_int32_t;
typedef uint32_t ogg_uint32_t;
typedef int64_t ogg_int64_t;
typedef uint64_t ogg_uint64_t;

#endif
