#ifndef ADS_SET_H
#define ADS_SET_H

#include <functional>
#include <algorithm>
#include <iostream>
#include <stdexcept> 

template <typename Key, size_t N = 7>
class ADS_set {
public:
	class Iterator;
	using value_type = Key;
	using key_type = Key;
	using reference = key_type&;
	using const_reference = const key_type&;
	using size_type = size_t;
	using difference_type = std::ptrdiff_t;
	using iterator = Iterator;
	using const_iterator = Iterator;

private:
	enum class Mode { free, used, end };
	struct element {
		key_type key;
		Mode mode = Mode::free;
		int next = -1;
	};
	element* table = nullptr;
	size_type last_free = 0;
	size_type table_size = 0;
	size_type curr_size = 0;
	size_type tableWithKeller_size = 0;
	float max_lf = 0.6;
	size_type h(const key_type& key) const { return hasher{}(key) % table_size; }
	void reserve(size_type n);
	void rehash(size_type n);
	element* insert_(const key_type& key);
	element* find_(const key_type& key) const;
public:
	ADS_set() { rehash(N); }
	ADS_set(std::initializer_list<key_type> ilist) : ADS_set{} { insert(ilist); }
	template<typename InputIt>
	ADS_set(InputIt first, InputIt last) : ADS_set{} { insert(first, last); }
	ADS_set(const ADS_set& other) :ADS_set{} {
		reserve(other.curr_size);
		for (const auto& k : other) {
			insert_(k);
		}
	}
	~ADS_set() { delete[] table; }

	ADS_set& operator=(const ADS_set& other) {
		if (this == &other) return *this;
		ADS_set tmp{ other };
		swap(tmp);
		return *this;
	}
	ADS_set& operator=(std::initializer_list<key_type> ilist) {
		ADS_set tmp{ ilist };
		swap(tmp);
		return *this;
	}

	size_type size() const { return curr_size; }
	bool empty() const { return curr_size == 0; }

	size_type count(const key_type& key) const { return !!find_(key); }
	iterator find(const key_type& key) const {
		if (auto  p{ find_(key) }) return iterator{ p };
		return end();
	}

	void clear() {
		ADS_set tmp;
		swap(tmp);
	}
	void swap(ADS_set& other) {
		using std::swap;
		swap(table, other.table);
		swap(curr_size, other.curr_size);
		swap(table_size, other.table_size);
		swap(max_lf, other.max_lf);
		swap(tableWithKeller_size, other.tableWithKeller_size);
		swap(last_free, other.last_free);
	}

	void insert(std::initializer_list<key_type> ilist) { insert(std::begin(ilist), std::end(ilist)); }
	std::pair<iterator, bool> insert(const key_type& key) {
		if (auto p{ find_(key) }) return { iterator{p}, false };
		reserve(curr_size + 1);
		return { iterator{insert_(key)}, true };
	}
	template<typename InputIt> void insert(InputIt first, InputIt last);

	size_type erase(const key_type& key) {
		if (auto p{ find_(key) }) {
			size_type idx{ h(key) };
			if (table[idx].next == -1 && key_equal{}(table[idx].key, key)) {
				p->mode = Mode::free;
				--curr_size;
				return 1;
			}
			else {
				last_free = tableWithKeller_size - 1;
				int wanted_point = idx;
				int previous_point = idx;

				if (key_equal{}(table[idx].key, key)) {
					int tmp = table[idx].next;
					p->key = table[tmp].key;
					p->mode = table[tmp].mode;
					p->next = table[tmp].next;
					table[tmp].mode = Mode::free;
					table[tmp].next = -1;
					--curr_size;
					return 1;
				}
				else {
					while (!key_equal{}(table[wanted_point].key, key)) {
						previous_point = wanted_point;
						wanted_point = table[wanted_point].next;
					}
					if (table[wanted_point].next == -1) {
						table[previous_point].next = -1;
						p->mode = Mode::free;
						--curr_size;
						return 1;
					}
					else {
						table[previous_point].next = p->next;
						p->next = -1;
						p->mode = Mode::free;
						--curr_size;
						return 1;
					}
				}
			}
		}
		return 0;

	}
	const_iterator begin() const { return const_iterator{ table }; }
	const_iterator end() const { return const_iterator{ table + tableWithKeller_size }; }

	void dump(std::ostream& o = std::cerr) const;

	friend bool operator==(const ADS_set& lhs, const ADS_set& rhs) {
		if (lhs.curr_size != rhs.curr_size) return false;
		for (const auto& k : rhs) if (!lhs.count(k)) return false;
		return true;
	}
	friend bool operator!=(const ADS_set& lhs, const ADS_set& rhs) { return !(lhs == rhs); }
};
template <typename Key, size_t N>
typename ADS_set<Key, N>::element* ADS_set<Key, N>::insert_(const key_type& key) {
	size_type idx = h(key);
	//if idx is free
	if (table[idx].mode == Mode::free) {
		table[idx].key = key;
		table[idx].mode = Mode::used;
		table[idx].next = -1;
		++curr_size;
	}
	else {

		while (last_free >= 0 && table[last_free].mode == Mode::used) {
			last_free--;
		}
		if (table[idx].next == -1) {
			//if idx is not free & has no next
			table[last_free].key = key;
			table[last_free].mode = Mode::used;
			table[last_free].next = -1;
			table[idx].next = last_free;
			++curr_size;
			return table + last_free;
		}
		else {
			//if idx is not free and has next
			int last_next = idx;
			bool found_last_next = false;
			do {
				last_next = table[last_next].next;
				if (table[last_next].next == -1) {
					found_last_next = true;
				}
			} while (!found_last_next);

			table[last_free].key = key;
			table[last_free].mode = Mode::used;
			table[last_free].next = -1;
			table[last_next].next = last_free;
			++curr_size;
			return table + last_free;
		}
	}
	return table + idx;
}
template <typename Key, size_t N>
typename ADS_set<Key, N>::element* ADS_set<Key, N>::find_(const key_type& key) const {
	size_type idx = h(key);
	do {
		switch (table[idx].mode) {
		case Mode::free:
			return nullptr;
		case Mode::used:
			if (key_equal{}(table[idx].key, key)) return table + idx;
			else if (table[idx].next == -1) return nullptr;
			else idx = table[idx].next;
			break;
		case Mode::end:
			throw std::runtime_error("Find reached END");
			break;
		}
	} while (true);
	return nullptr;
}

template <typename Key, size_t N>
template <typename InputIt> void ADS_set<Key, N>::insert(InputIt first, InputIt last) {
	for (auto it{ first }; it != last; ++it) {
		if (!count(*it)) {
			reserve(curr_size + 1);
			insert_(*it);
		}
	}
}

template <typename Key, size_t N>
void ADS_set<Key, N>::reserve(size_type n) {
	if (n > table_size * max_lf || last_free == table_size) {
		size_type new_table_size{ table_size };
		do {
			new_table_size = new_table_size * 2 + 1;
		} while (n > new_table_size * max_lf);
		rehash(new_table_size);
	}
}

template <typename Key, size_t N>
void ADS_set<Key, N>::rehash(size_type n) {
	size_type old_table_size_with_keller{ tableWithKeller_size };
	element* old_table{ table };

	size_type new_table_size{ std::max(N,std::max(n, size_type(curr_size / max_lf))) };
	tableWithKeller_size = new_table_size + size_type(new_table_size * 0.1628);

	element* new_table{ new element[tableWithKeller_size + 1] };
	new_table[tableWithKeller_size].mode = Mode::end;

	curr_size = 0;
	table_size = new_table_size;
	table = new_table;

	last_free = tableWithKeller_size - 1;

	for (size_type idx{ 0 }; idx < old_table_size_with_keller; idx++) {
		if (old_table[idx].mode == Mode::used) insert_(old_table[idx].key);
	}
	delete[] old_table;
}
template<typename Key, size_t N>
void ADS_set<Key, N>::dump(std::ostream& o) const {
	o << "curr_size = " << curr_size << " table_size = " << table_size << " table_size_with_keller = " << tableWithKeller_size << "\n";
	for (size_type idx{ 0 }; idx < tableWithKeller_size + 1; ++idx) {
		o << idx << ": ";
		switch (table[idx].mode) {
		case Mode::free:
			o << "--free";
			break;
		case Mode::used:
			o << table[idx].key;
			break;
		case Mode::end:
			o << "--END";
			break;
		}
		o << "\n";
	}
}

template <typename Key, size_t N>
class ADS_set<Key, N>::Iterator {
public:
	using value_type = Key;
	using difference_type = std::ptrdiff_t;
	using reference = const value_type&;
	using pointer = const value_type*;
	using iterator_category = std::forward_iterator_tag;
private:
	element* pos;
	void skip() { while (pos->mode != Mode::used && pos->mode != Mode::end) ++pos; }
public:
	explicit Iterator(element* pos = nullptr) : pos{ pos } { if (pos) skip(); }
	reference operator*() const { return pos->key; }
	pointer operator->() const { return &pos->key; }
	Iterator& operator++() { ++pos; skip(); return *this; }
	Iterator operator++(int) { auto rc{ *this }; ++* this; return rc; }
	friend bool operator==(const Iterator& lhs, const Iterator& rhs) { return lhs.pos == rhs.pos; }
	friend bool operator!=(const Iterator& lhs, const Iterator& rhs) { return lhs.pos != rhs.pos; }
};

template <typename Key, size_t N> void swap(ADS_set<Key, N>& lhs, ADS_set<Key, N>& rhs) { lhs.swap(rhs); }

#endif // ADS_SET_H
