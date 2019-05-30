#ifndef CELL_HELPERS_HPP
#define CELL_HELPERS_HPP

#include <boost/functional/hash.hpp>

using position = std::vector<int>;

position absolute_to_relative(position current, position other) {
    position relative(current.size(), 0);
    std::transform(
            current.begin(),
            current.end(),
            other.begin(),
            relative.begin(),
            [](int curr_idx, int other_idx) -> int{ return other_idx - curr_idx ; }
    );
    return relative;
}

template <typename Container> // we can make this generic for any container [1]
struct container_hash {
    std::size_t operator()(Container const& c) const {
        return boost::hash_range(c.begin(), c.end());
    }
};


#endif // CELL_HELPERS_HPP

