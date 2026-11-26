/* protobuf config.h for the Waf build, all platforms.
 * No hash_map or hash_set is declared, so google/protobuf/stubs/hash.h falls back to std::map and std::set.
 * That keeps one configuration for MSVC, GCC and the Android NDK. */
#ifndef PROTOBUF_WAF_CONFIG_H
#define PROTOBUF_WAF_CONFIG_H

#if !defined( _MSC_VER ) && !defined( HAVE_PTHREAD )
#define HAVE_PTHREAD 1
#endif

#endif
