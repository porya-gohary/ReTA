#ifndef INDEX_SET_H
#define INDEX_SET_H


class indexSet {
public:
	// Using uint64_t to store 64 bits in each byte
	typedef std::vector <uint64_t> Set_type;

	// new empty job set
	indexSet() : the_set() {}

	// derive a new set by "cloning" an existing set and adding an index
	indexSet(const indexSet &from, std::size_t idx)
			: the_set(std::max(from.the_set.size(), (idx / 64) + 1)) {
		std::copy(from.the_set.begin(), from.the_set.end(), the_set.begin());
		set_bit(idx, true);
	}

	// derive a new set by "cloning" an existing set and adding a set of indices
	indexSet(const indexSet &from, std::vector <std::size_t> indices)
			: the_set(std::max(from.the_set.size(), ((indices.empty() ? 0 : indices.back()) / 64) + 1)) {
		std::copy(from.the_set.begin(), from.the_set.end(), the_set.begin());
		for (auto i: indices)
			set_bit(i, true);
	}

	// create the diff of two job sets (intended for debugging only)
	indexSet(const indexSet &a, const indexSet &b)
			: the_set(std::max(a.the_set.size(), b.the_set.size()), 0) {
		auto limit = std::min(a.the_set.size(), b.the_set.size());
		for (std::size_t i = 0; i < limit; ++i)
			the_set[i] = a.the_set[i] & ~b.the_set[i];
	}

	bool operator==(const indexSet &other) const {
		return the_set == other.the_set;
	}

	bool operator!=(const indexSet &other) const {
		return the_set != other.the_set;
	}

	// make |= operator available for union of sets
	indexSet &operator|=(const indexSet &other) {
		if (the_set.size() < other.the_set.size())
			the_set.resize(other.the_set.size(), 0);
		for (std::size_t i = 0; i < other.the_set.size(); ++i)
			the_set[i] |= other.the_set[i];
		return *this;
	}

	// make &= operator available for intersection of sets
	indexSet &operator&=(const indexSet &other) {
		for (std::size_t i = 0; i < the_set.size(); ++i)
			if (i < other.the_set.size())
				the_set[i] &= other.the_set[i];
			else
				the_set[i] = 0;
		return *this;
	}

	bool contains(std::size_t idx) const {
		if (idx / 64 >= the_set.size()) {
			return false;
		}
		return get_bit(idx);
	}

	bool includes(std::vector <std::size_t> indices) const {
		for (auto i: indices)
			if (!contains(i))
				return false;
		return true;
	}

	bool is_subset_of(const indexSet &other) const {
		// Compare each integer in the sets
		for (std::size_t i = 0; i < the_set.size(); ++i) {
			// If there are bits set in the current set that are not set in the other set, it's not a subset
			if (the_set[i] > 0 && (other.the_set.size() <= i || (the_set[i] & other.the_set[i]) != the_set[i])) {
				return false;
			}
		}

		return true;
	}

	std::size_t size() const {
		std::size_t count = 0;
		for (std::size_t i = 0; i < the_set.size() * 64; ++i)
			if (contains(i))
				count++;
		return count;
	}

	void add(std::size_t idx) {
		if (idx / 64 >= the_set.size())
			the_set.resize((idx / 64) + 1, 0);
		set_bit(idx, true);
	}

	friend std::ostream &operator<<(std::ostream &stream,
									const indexSet &s) {
		bool first = true;
		stream << "{";
		for (std::size_t i = 0; i < s.the_set.size() * 64; ++i) {
			if (s.contains(i)) {
				if (!first)
					stream << ", ";
				first = false;
				stream << i;
			}
		}
		stream << "}";

		return stream;
	}

private:

	Set_type the_set;

	// Helper functions to set and get individual bits
	void set_bit(std::size_t idx, bool value) {
		std::size_t byte_index = idx / 64;
		std::size_t bit_index = idx % 64;
		if (value) {
			the_set[byte_index] |= (((uint64_t) 1) << bit_index);
		} else {
			the_set[byte_index] &= ~(((uint64_t) 1) << bit_index);
		}
	}

	bool get_bit(std::size_t idx) const {
		std::size_t byte_index = idx / 64;
		std::size_t bit_index = idx % 64;
		return the_set[byte_index] & (((uint64_t) 1) << bit_index);
	}

	// no accidental copies
//			Index_set(const Index_set& origin) = delete;
};


#endif
