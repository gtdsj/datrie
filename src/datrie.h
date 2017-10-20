#ifndef __DATRIE_H_PROTECTED__
#define __DATRIE_H_PROTECTED__

#include "triedefs.h"
#include "alpharange.h"

#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

template <typename T>
class datrie
{
public:
	typedef struct {
		trie_index_t base;
		trie_index_t check;
	}da_cell_t;
	typedef std::vector<da_cell_t> da_array_t;

	typedef struct {
		T data;
		trie_char_t* suffix;
	}da_tail_block_t;
	typedef std::vector<da_tail_block_t> da_tail_t;
	typedef std::vector<trie_char_t> da_codes_t;
public:
	datrie();
	~datrie();
protected:
	alpharange _alpharange;
	da_array_t _array;
	da_tail_t _tails;
public:
	bool store(string_t str, T& data);
	bool retrieve(string_t str, T& data);
	bool remove(string_t str);
	void add_code_range(trie_char_t begin, trie_char_t end);
	bool extend_to_index(trie_index_t to_index);

	
	void set_check(trie_index_t index, trie_index_t val);
	void set_base(trie_index_t index, trie_index_t val);
	trie_index_t get_check(trie_index_t index);
	trie_index_t get_base(trie_index_t index);

	inline trie_index_t get_free_list_base() {
		return -get_base(1);
	}
	inline void set_free_list_base(trie_index_t val) {
		set_base(1, val);
	}
	inline trie_index_t get_free_list_check() {
		return -get_check(1);
	}
	inline void set_free_list_check(trie_index_t val) {
		set_check(1, val);
	}

	bool walk(trie_index_t& s, trie_char_t c);
	bool insert_branch(string_t& str, string_t::size_type start_pos, trie_index_t s, T& data);

	void get_codes(da_codes_t& codes, trie_index_t base);

	trie_index_t find_free_base(da_codes_t& codes);
	bool is_valid_base(da_codes_t& codes, trie_index_t base);
	bool relocate_base(trie_index_t s, trie_index_t new_base, da_codes_t& codes, trie_char_t new_code);

	void prepare_cell(trie_index_t index);
	trie_index_t free_cell(trie_index_t old_next, trie_index_t s);

	trie_index_t insert_state(trie_index_t s, trie_char_t ch);
	double rate();

	bool save(std::string filename);
	bool load(std::string filename);
};

template<typename T>
datrie<T>::datrie()
{
	_array.resize(3);
	_array.at(0).base = TRIE_SIGNATURE;
	_array.at(0).check = 3;

	// TRIE_FREE_INDEX == 1
	_array.at(1).base = -1;
	_array.at(1).check = -1;
	_array.at(2).base = TRIE_BEGIN_INDEX;
	_array.at(2).check = 0;
}

template<typename T>
datrie<T>::~datrie()
{
}

template<typename T>
trie_index_t datrie<T>::insert_state(trie_index_t s, trie_char_t ch)
{
	trie_index_t base = get_base(s);
	trie_index_t code = _alpharange.get_code(ch);
	trie_index_t next = 0;
	if (base > 0) {
		next = base + code;
		// base > TRIE_INDEX_MAX - code 意味着next超过了允许的最大值0x7fffffff，溢出了
		// _array[next].check > 0 意味着next已经有值
		// 以上两种情况均需要重新寻找适当的base
		if (base > TRIE_INDEX_MAX - code || !extend_to_index(next) || get_check(next) >= 0) {
			// 获取所有base开始的状态
			da_codes_t codes;
			get_codes(codes, s);
			da_codes_t::iterator iter = std::lower_bound(codes.begin(), codes.end(), code);
			codes.insert(iter, code);

			// 查找适当的base使得所有的codes都能正常放置
			trie_index_t new_base = find_free_base(codes);
			relocate_base(s, new_base, codes, code);
			next = new_base + code;
		}else {
			prepare_cell(next);
			set_check(next, s);
		}
	}else {
		da_codes_t codes(1, code);
		trie_index_t new_base = find_free_base(codes);
		set_base(s, new_base);
		next = new_base + code;
		prepare_cell(next);
		set_check(next, s);
	}
	return next;
}

template<typename T>
bool datrie<T>::insert_branch(string_t& str, string_t::size_type start_pos, trie_index_t s, T& data)
{
	if (start_pos >= str.size()) {
		return false;
	}

	trie_index_t next = insert_state(s, str[start_pos]);
	da_tail_block_t tail = { data, NULL };
	trie_index_t left_size = str.size() - start_pos - 1;

	if (left_size > 0 && (str[start_pos + 1] != TRIE_TERM_CHAR)) {
		tail.suffix = new trie_char_t[left_size];
		std::memcpy(tail.suffix, &str[start_pos + 1], left_size * sizeof(trie_char_t));
	}

	// set tail index
	_tails.push_back(tail);
	set_base(next, -((trie_index_t)_tails.size() - 1));
	return true;
}

template<typename T>
bool datrie<T>::store(string_t str, T& data)
{
	if (str.empty()) {
		return false;
	}
	if (str[str.size() - 1] != TRIE_TERM_CHAR) {
		str.push_back(TRIE_TERM_CHAR);
	}
	trie_index_t s = TRIE_ROOT_INDEX;
	string_t::size_type i = 0;
	for (; i < str.size() && get_base(s) > 0; ++i) {
		trie_index_t c = _alpharange.get_code(str[i]);
		if (c == TRIE_INDEX_MAX) {
			return false;
		}
		if (!walk(s, c)) {
			// insert
			return insert_branch(str, i, s, data);
		}
	}

	// in tail
	trie_index_t tail_index = -get_base(s);
	if (tail_index >= (trie_index_t)_tails.size()) {
		return false;
	}
	da_tail_block_t& tail = _tails[tail_index];
	string_t suffix(1, 0);
	if (tail.suffix != NULL) {
		suffix = tail.suffix;
		if (suffix[suffix.size() - 1] != TRIE_TERM_CHAR) {
			suffix.push_back(TRIE_TERM_CHAR);
		}
	}
	for (trie_index_t x = i, suffix_index = 0; x < (trie_index_t)str.size(); ++x, ++suffix_index) {
		if (str[x] == suffix[suffix_index]) {
			s = insert_state(s, str[x]);
		}else {
			// modify old tail
			trie_index_t old_s = insert_state(s, suffix[suffix_index]);
			set_base(old_s, -tail_index);
			if (tail.suffix != NULL){
				delete[] tail.suffix;
				tail.suffix = NULL;
			}
			
			trie_index_t left_size = suffix.size() - suffix_index - 1;
			if (left_size > 0) {
				tail.suffix = new trie_char_t[left_size];
				std::memcpy(tail.suffix, &suffix[suffix_index + 1], left_size * sizeof(trie_char_t));
			}

			// insert new 
			/*trie_index_t new_s = insert_state(s, str[i]);*/
			return insert_branch(str, x, s, data);
		}
	}
	return false;
}

template<typename T>
bool datrie<T>::retrieve(string_t str, T& data)
{
	// add a Terminator
	if (str.empty()) {
		return false;
	}
	if (str[str.size() - 1] != TRIE_TERM_CHAR) {
		str.push_back(TRIE_TERM_CHAR);
	}

	// retrieve
	trie_index_t s = TRIE_ROOT_INDEX;
	string_t::size_type i = 0;
	for (; i < str.size() && get_base(s) > 0; ++i) {
		trie_index_t c = _alpharange.get_code(str[i]);
		if (!walk(s, c)) {
			return false;
		}
	}

	trie_index_t tail_index = -get_base(s);
	if (tail_index >= (trie_index_t)_tails.size()) {
		return false;
	}
	da_tail_block_t& tail = _tails[tail_index];

	if (tail.suffix == NULL ) {
		if (i < str.size() && str[i] != TRIE_TERM_CHAR) {
			return false;
		}
		
	}else {
		string_t tail_suffix(tail.suffix);
		string_t suffix = str.substr(i);
		if ((int)(suffix.size() - 1) > 0 &&  suffix[suffix.size() - 1] == TRIE_TERM_CHAR) {
			suffix.resize(suffix.size() - 1);
		}
		if (tail_suffix != suffix) {
			return false;
		}
	}
	
	data = tail.data;
	return true;
}

template<typename T>
bool datrie<T>::remove(string_t str)
{
	return false;
}

template<typename T>
void datrie<T>::add_code_range(trie_char_t begin, trie_char_t end)
{
	_alpharange.add_range(begin, end);
}

template<typename T>
bool datrie<T>::extend_to_index(trie_index_t to_index)
{
	if (to_index <= 0) {
		return false;
	}else if (to_index < (trie_index_t)_array.size()) {
		return true;
	}
	trie_index_t new_begin = _array.size();
	_array.resize(to_index + 1);
	
	for (trie_index_t i = new_begin; i < to_index; i++) {
		this->set_check(i, -(i + 1));
		this->set_base(i + 1, -i);
	}
	
	//
	trie_index_t free_tail = this->get_free_list_base();
	this->set_check(free_tail, -new_begin);
	this->set_base(new_begin, -free_tail);
	this->set_check(to_index, -TRIE_FREE_INDEX);
	this->set_base(TRIE_FREE_INDEX, -to_index);
	
	_array.at(0).check = _array.size();
	return true;
}

template<typename T>
void datrie<T>::set_check(trie_index_t index, trie_index_t val)
{
	if (index < (trie_index_t)_array.size()) {
		_array.at(index).check = val;
	}
}

template<typename T>
void datrie<T>::set_base(trie_index_t index, trie_index_t val)
{
	if (index < (trie_index_t)_array.size()) {
		_array.at(index).base = val;
	}
}

template<typename T>
int32_t datrie<T>::get_check(trie_index_t index)
{
	return index < (trie_index_t)_array.size() ? _array.at(index).check : TRIE_INDEX_MAX;
}

template<typename T>
int32_t datrie<T>::get_base(trie_index_t index)
{
	return index < (trie_index_t)_array.size() ? _array.at(index).base : TRIE_INDEX_MAX;
}

template<typename T>
bool datrie<T>::walk(trie_index_t& s, trie_char_t c)
{
	trie_index_t t = _array[s].base + c;
	if (get_check(t) == s) {
		s = t;
		return true;
	}
	return false;
}

template<typename T>
void datrie<T>::get_codes(da_codes_t& codes, trie_index_t s)
{
	trie_index_t base = _array[s].base;
	trie_index_t max_code = _alpharange.get_max_code();
	max_code = std::min(max_code, _array.size() - base);
	for (trie_index_t i = 0; i <= max_code; ++i) {
		if (get_check(base + i) == s) {
			codes.push_back(i);
		}
	}
}

template<typename T>
trie_index_t datrie<T>::find_free_base(da_codes_t& codes)
{
	if (codes.empty()) {
		return TRIE_INDEX_MAX;
	}
	trie_char_t first_code = codes[0];
	trie_index_t s = get_free_list_check();
	while (s != TRIE_FREE_INDEX && s < TRIE_BEGIN_INDEX + first_code) {
		s = -this->get_check(s);
	}
	if (s == TRIE_FREE_INDEX) {
		for (s = TRIE_BEGIN_INDEX + first_code; ; ++s) {
			if (!extend_to_index(s)) {
				return TRIE_INDEX_ERROR;
			}
			if (get_check(s) < 0) {
				break;
			}
		}
	}

	while (!is_valid_base(codes, s - first_code)){
		if (get_check(s) == TRIE_FREE_INDEX && !extend_to_index(s + codes[codes.size() - 1])) {
			return TRIE_INDEX_ERROR;
		}
		s = -get_check(s);
	}

	return s - first_code;
}

template<typename T>
bool datrie<T>::is_valid_base(da_codes_t& codes, trie_index_t base)
{
	for (da_codes_t::size_type i = 0; i < codes.size(); ++i) {
		trie_index_t next = base + codes[i];
		if (!extend_to_index(next) || get_check(next) >= 0) {
			return false;
		}
	}
	return true;
}

template<typename T>
bool datrie<T>::relocate_base(trie_index_t s, trie_index_t new_base, da_codes_t& codes, trie_char_t new_code)
{
	trie_index_t last_free = TRIE_FREE_INDEX;
	trie_index_t old_base = _array[s].base;
	set_base(s, new_base);

	for (da_codes_t::size_type i = 0; i < codes.size(); ++i) {
		if (codes[i] == new_code) {
			prepare_cell(new_base + codes[i]);
			set_check(new_base + codes[i], s);
			continue;
		}
		trie_index_t old_next = old_base + codes[i];
		trie_index_t new_next = new_base + codes[i];
		trie_index_t old_next_base = get_base(old_next);
		prepare_cell(new_next);
		set_base(new_next, old_next_base);
		set_check(new_next, s);
		
		/* old_next_base < 0 表示遇到后缀 */
		if (old_next_base > 0) {
			trie_index_t max_code = std::max(_alpharange.get_max_code(), (trie_index_t)_array.size() - old_next_base);
			for (trie_index_t i = 0; i < max_code; ++i){
				if (get_check(old_next_base + i) == old_next) {
					set_check(old_next_base + i, new_next);
				}
			}
		}
		// clean old next cell
		last_free = free_cell(old_next, last_free);
	}
	return true;
}

template<typename T>
void datrie<T>::prepare_cell(trie_index_t index)
{
	if (get_check(index) >= 0) {
		return;
	}
	trie_index_t pre_free = -get_base(index);
	trie_index_t next_free = -get_check(index);
	set_check(pre_free, -next_free);
	set_base(next_free, -pre_free);
}

template<typename T>
trie_index_t datrie<T>::free_cell(trie_index_t old_next, trie_index_t s)
{
	do 
	{
		s = -get_check(s);
	} while (s != TRIE_FREE_INDEX && s < old_next);

	trie_index_t pre_free = -get_base(s);
	set_check(pre_free, -old_next);
	set_base(old_next, -pre_free);
	set_check(old_next, -s);
	set_base(s, -old_next);
	return old_next;
}

template<typename T>
double datrie<T>::rate()
{
	trie_index_t valid = 0;
	for (da_array_t::size_type i = 3; i < _array.size(); ++i) {
		if (get_check(i) > 0) {
			valid++;
		}
	}
	return (double)valid / (double)_array.size();
}

template<typename T>
bool datrie<T>::save(std::string filename)
{
	std::ofstream ofile;
	ofile.open(filename, ios::out | ios::binary);
	if (ofile.is_open()) {
		// code range
		std::string code_range = _alpharange.serialize();
		ofile.write(&code_range[0], code_range.size());
		
		// double array
		int size = _array.size() * sizeof(da_cell_t);
		ofile.write(reinterpret_cast<const char*>(&_array[0]), size);

		// tail
		int signature = TRIE_SIGNATURE;
		ofile.write(reinterpret_cast<const char*>(&signature), sizeof(int));
		trie_index_t tail_size = (trie_index_t)_tails.size();
		ofile.write(reinterpret_cast<const char*>(&tail_size), sizeof(tail_size));
		for (trie_index_t i = 0; i < tail_size; ++i) {
			ofile.write(reinterpret_cast<const char*>(&_tails[i].data), sizeof(T));
			int len = 0;
			if (_tails[i].suffix != 0) {
				trie_char_t* tmp = _tails[i].suffix;
				while (*tmp++ != TRIE_TERM_CHAR) {
					len++;
				}
			}
			ofile.write(reinterpret_cast<const char*>(&len), sizeof(len));
			if (len > 0) {
				ofile.write(reinterpret_cast<const char*>(_tails[i].suffix), len * sizeof(trie_char_t));
			}
		}
		ofile.close();
		return true;
	}
	return false;
}

template<typename T>
bool datrie<T>::load(std::string filename)
{
	std::ifstream infile;
	infile.open(filename, ios::in | ios::binary);
	if (infile.is_open()) {
		// load code range
		std::string str(4, 0);
		infile.read(&str[0], sizeof(int));
		int size = *reinterpret_cast<int*>(&str[0]);
		str.resize(size);
		infile.read(&str[sizeof(int)], size - sizeof(int));
		_alpharange.deserialize(str);

		// load double array
		infile.read(reinterpret_cast<char*>(&_array[0]), sizeof(da_cell_t) * 3);
		if (_array[0].base != TRIE_SIGNATURE) {
			return false;
		}
		int count = _array[0].check;
		_array.resize(count * sizeof(da_cell_t));
		infile.read(reinterpret_cast<char*>(&_array[3]), (count - 3) * sizeof(da_cell_t));

		// load tail
		da_cell_t signature = { 0, 0 };
		infile.read(reinterpret_cast<char*>(&signature), sizeof(da_cell_t));
		if (signature.base != TRIE_SIGNATURE){
			return false;
		}
		if (signature.check == 0) {
			return false;
		}
		_tails.resize(signature.check);
		int len = 0;
		int i = 0;
		for (; i < signature.check; ++i) {
			infile.read(reinterpret_cast<char*>(&_tails[i].data), sizeof(T));
			infile.read(reinterpret_cast<char*>(&len), sizeof(int));
			if (len > 0) {
				_tails[i].suffix = new trie_char_t[len];
				infile.read(reinterpret_cast<char*>(_tails[i].suffix), len * sizeof(trie_char_t));
			}
		}
		infile.close();
		return true;
	}
	return false;
}
#endif // !__DATRIE_H_PROTECTED__

