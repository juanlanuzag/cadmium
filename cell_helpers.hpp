#ifndef CELL_HELPERS_HPP
#define CELL_HELPERS_HPP


using position = std::vector<int>;

position absolute_to_relative(position current, position other) {
    position relative(current.size(), 0);
    std::transform(
            current.begin(),
            current.end(),
            other.begin(),
            relative.begin(),
            [](int curr_idx, int other_idx) -> int{ return curr_idx - other_idx; }
    );
    return relative;
}


#endif // CELL_HELPERS_HPP

