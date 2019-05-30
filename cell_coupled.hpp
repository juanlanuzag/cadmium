#include <cadmium/modeling/dynamic_coupled.hpp>
#include "./cell_helpers.hpp"

namespace cadmium {
    namespace dynamic {
        namespace modeling {
            template <template<typename> typename CELL_ATOMIC, typename TIME>
            class cell_coupled : public coupled<TIME> {

            private:
                std::vector<position> _relative_neighborhood;
                std::vector<int> _dimensions;

            public:
                cell_coupled(std::string id, std::vector<int> dimensions, std::vector<position> relative_neighbors)
                    : coupled<TIME>(id),
                      _dimensions(dimensions),
                      _relative_neighborhood(relative_neighbors) {

                    cadmium::dynamic::modeling::Models cells;
                    cadmium::dynamic::modeling::ICs ics;

                    for(int i = 0; i < dimensions[0]; i++) {
                        for(int j = 0; j < dimensions[1]; j++) {
                            position current_pos = {i,j};
                            coupled<TIME>::_models.push_back(
                                    cadmium::dynamic::translate::make_dynamic_atomic_model<CELL_ATOMIC, TIME>(get_cell_name(current_pos), current_pos, current_pos[0]==1, _relative_neighborhood)
                            );

                            for (auto pos : neighbors(current_pos)) {
                                coupled<TIME>::_ic.push_back(
                                        cadmium::dynamic::translate::make_IC<
                                                typename std::tuple_element<0, typename CELL_ATOMIC<TIME>::output_ports>::type,
                                                typename std::tuple_element<0, typename CELL_ATOMIC<TIME>::input_ports>::type
                                        >(get_cell_name(pos), get_cell_name(current_pos))
                                );
                                // TODO: we should assert static somewhere that the CELL ATOMIC has exactly 1 inport and an 1 outport
                            }
                        }
                    }
                    coupled<TIME>::validate_links();
                }



                std::string get_cell_name(position pos) {
                    std::stringstream model_name;
                    model_name << coupled<TIME>::get_id() << "_cell";
                    for (auto elem : pos) {
                        model_name << "_" << elem;
                    }
                    return model_name.str();
                }


                std::vector<position> neighbors(position pos) {
//                    Returns the neighbors of the position pos

                    auto get_absolute_pos = [this, &pos](position& rel_neighbor) -> position  {
                        position absolute_pos(pos.size(), 0);
                        std::transform(
                                pos.begin(),
                                pos.end(),
                                rel_neighbor.begin(),
                                absolute_pos.begin(),
                                [](int index, int offset) -> int{ return index + offset; }
                        );
                        position final_pos(pos.size(), 0);
                        std::transform(
                                absolute_pos.begin(),
                                absolute_pos.end(),
                                _dimensions.begin(),
                                final_pos.begin(),
                                [](int coord, int max_coord) -> int {
                                    int remainder = coord % max_coord;
                                    return remainder >= 0 ? remainder : remainder + max_coord;
                                }
                        );
                        return final_pos;
                    };

                    std::vector<position> neighors_pos(_relative_neighborhood.size(), position(pos.size(), 0));
                    std::transform(_relative_neighborhood.begin(),
                                   _relative_neighborhood.end(),
                                   neighors_pos.begin(),
                                   get_absolute_pos);
                    return neighors_pos;
                }


            };
        }
    }
}