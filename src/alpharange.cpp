#include "alpharange.h"

#include <iostream>
#include <algorithm>

alpharange::alpharange()
{
}


alpharange::~alpharange()
{
}


void alpharange::add_range(trie_char_t begin, trie_char_t end)
{
	if (begin > end) {
		return;
	}

	bool insert_to_start = false;
	// 找到第一个包含begin的区间
	ranges_t::iterator it = _ranges.begin(), begin_node = _ranges.end(), end_node = _ranges.end();
	for (; it != _ranges.end() && begin + 1 >= it->begin; ++it){
		if (begin <= it->end + 1) {
			it->begin = std::min(begin, it->begin);
			it->end = std::max(it->end, end);
			begin_node = it;
			break;
		}
	}
	if (it != _ranges.end()) {
		insert_to_start = true;
		for (++it; it != _ranges.end();) {
			if (end + 1 <= it->begin) {
				break;
			}
			if (end <= it->end + 1) {
				if (begin_node != _ranges.end()) {
					begin_node->end = std::max(it->end, end);
				}else {
					end = std::max(end, it->end);
				}
				it = _ranges.erase(it);
				break;
			}
			else {
				it = _ranges.erase(it);
			}
		}
	}
	
	if (begin_node == _ranges.end()) {
		code_range_t new_range = { begin, end };
		if (insert_to_start) {
			_ranges.insert(_ranges.begin(), new_range);
		}else {
			_ranges.insert(_ranges.end(), new_range);
		}
	}

	_update_max_code();
}

int32_t alpharange::get_code(trie_char_t ch)
{
	if (ch == 0) {
		return 0;
	}
	ranges_t::iterator it = _ranges.begin();
	int32_t code = 1;
	for (; it != _ranges.end() ; ++it) {
		if (it->begin <= ch && ch <= it->end) {
			return code + ch - it->begin;
		}
		code += it->end - it->begin + 1;
	}
	return TRIE_INDEX_MAX;
}

void alpharange::_update_max_code()
{
	int32_t code = 0;
	for (ranges_t::iterator it = _ranges.begin(); it != _ranges.end(); ++it) {
		code += it->end - it->begin + 1;
	}
	_max_code = code;
}

void alpharange::show()
{
	std::cout << sizeof(code_range_t) << std::endl;
	ranges_t::iterator it = _ranges.begin();
	for (; it != _ranges.end();++it) {
		std::cout << "range : " << it->begin << " ~ " << it->end << std::endl;
	}
}

std::string alpharange::serialize()
{
	int total_size = 4 + _ranges.size() * sizeof(code_range_t);

	std::string str(total_size, 0);
	int offset = 0;
	int* size = reinterpret_cast<int*>(&str[0]);
	*size = total_size;
	offset += sizeof(int);
	ranges_t::iterator it = _ranges.begin();
	for (; it != _ranges.end(); ++it){
		code_range_t* p = reinterpret_cast<code_range_t*>(&str[offset]);
		p->begin = it->begin;
		p->end = it->end;
		offset += sizeof(code_range_t);
	}
	return str;
}

void alpharange::deserialize(std::string& str)
{
	int total_size =  *reinterpret_cast<int*>(&str[0]);
	code_range_t* p = NULL;
	int offset = sizeof(total_size);
	for (int32_t i = 0; i < (total_size - offset)/(int32_t)sizeof(code_range_t); ++i) {
		p = reinterpret_cast<code_range_t*>(&str[offset]);
		add_range(p->begin, p->end);
		offset += sizeof(code_range_t);
	}
}
