#ifndef __TRIEDEFS_H_PROTECTED__
#define __TRIEDEFS_H_PROTECTED__

#include <stdint.h>
#include <string>


typedef uint16_t trie_char_t;
typedef int32_t trie_index_t;
typedef std::basic_string<trie_char_t> string_t;

#define TRIE_INDEX_ERROR	0
#define TRIE_INDEX_MAX		0x7fffffff
#define TRIE_SIGNATURE		0xdeadbeaf

#define TRIE_FREE_INDEX		1
#define TRIE_ROOT_INDEX		2
#define TRIE_BEGIN_INDEX	3
#define TRIE_TERM_CHAR		0

#endif // !__TRIEDEFS_H_PROTECTED__

