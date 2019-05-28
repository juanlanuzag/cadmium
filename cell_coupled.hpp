
#include <cadmium/modeling/dynamic_coupled.hpp>

namespace cadmium {
    namespace dynamic {
        namespace modeling {

            template <typename TIME>
            class cell_coupled : public coupled<TIME> {

            public:
                cell_coupled(std::string id, std::vector<int> dimensions) : coupled<TIME>(id) {
                    cadmium::dynamic::modeling::Models cells;
                    cadmium::dynamic::modeling::ICs ics;

                    for(int i = 0; i < dimensions[0]; i++) {
                        for(int j = 0; j < dimensions[1]; j++) {
                            position current_pos = std::make_pair(i,j);
                            coupled<TIME>::_models.push_back(
                                    cadmium::dynamic::translate::make_dynamic_atomic_model<cell_atomic, TIME>(get_cell_name(current_pos), current_pos, current_pos.first==150)
                                            );

                            for (auto pos : vecinos(current_pos)) {
                                coupled<TIME>::_ic.push_back(
                                        cadmium::dynamic::translate::make_IC<cell_atomic_defs::out, cell_atomic_defs::in>(get_cell_name(pos), get_cell_name(current_pos))
                                );
                            }
                        }
                    }
                    coupled<TIME>::validate_links();
                }

                std::string get_cell_name(position pos) {
                    std::stringstream model_name;
                    model_name << coupled<TIME>::get_id() << "_cell_" << pos.first << "_" << pos.second;
                    return model_name.str();
                }

            };
        }
    }
}