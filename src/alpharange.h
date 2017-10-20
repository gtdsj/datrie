#ifndef __ALPHARANGE_H_PROTECTED__
#define __ALPHARANGE_H_PROTECTED__

#include "triedefs.h"
#include <list>

class alpharange
{
public:
	typedef struct _code_range {
		trie_char_t begin;
		trie_char_t end;
	}code_range_t;
	typedef std::list<code_range_t> ranges_t;
public:
	alpharange();
	~alpharange();

	void add_range(trie_char_t begin, trie_char_t end);
	int32_t get_code(trie_char_t ch);
	inline int32_t get_max_code() { return _max_code; }

	std::string serialize();
	void deserialize(std::string&);

	void show();
protected:
	void _update_max_code();
protected:
	ranges_t _ranges;
	int32_t _max_code;
public:
	
};


#endif // !__ALPHARANGE_H_PROTECTED__

