
#include <tuple>
#include <queue>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <fstream>
// #include <cadmium/modeling/dynamic_model_translator.hpp>
#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/message_bag.hpp>

using VALUE = bool;
using position = std::pair<int, int>;
using msg_type = std::pair<position, VALUE>;

#define WIDTH 300
#define DEPTH 1

#define DURATION 1

position up_left (position& pos) {
    return std::make_pair<int, int>(
            (pos.first-1) % WIDTH + (pos.first-1 < 0 ? WIDTH : 0),
            (pos.second+1) % DEPTH
            );
}
position up_right (position& pos) {
    return std::make_pair<int, int>(
            (pos.first+1) % WIDTH,
            (pos.second+1) % DEPTH
            );
}
position up (position& pos) {
    return std::make_pair<int, int>(
            (pos.first) % WIDTH,
            (pos.second+1) % DEPTH
            );
}

std::vector<position> vecinos(position pos) {
    return { up_left(pos), up_right(pos), up(pos) };
}







struct cell_atomic_defs{
    //custom ports
    struct in : public cadmium::in_port<msg_type> {};
    struct out : public cadmium::out_port<msg_type> {};
};


template<typename TIME>
class cell_atomic {
    using defs=cell_atomic_defs;

    //ports_definition

    protected:
        void set_cell_state(VALUE& val) {
            if (state.cell_state != val) {
                state.cell_state = val;
            }
        }

    public:
        struct state_type {
            TIME next_internal;
            VALUE cell_state;
            std::priority_queue<std::pair<TIME, VALUE>,std::vector<std::pair<TIME, VALUE>>, std::greater<std::pair<TIME, VALUE>> > delayed_outputs;

            VALUE up_left;
            VALUE up_right;
            VALUE up;
        };
        state_type state;
        position my_position;

        using input_ports=std::tuple<typename defs::in>;
        using output_ports=std::tuple<typename defs::out>;

        void update_state() {
            if (state.up_left && state.up && state.up_right) {
                state.cell_state = false;
            } else if (state.up_left &&  state.up && !state.up_right) {
                state.cell_state = true;
            } else if (state.up_left && !state.up && state.up_right) {
                state.cell_state = true;
            } else if (!state.up_left && state.up && state.up_right) {
                state.cell_state = true;
            } else if (!state.up_left && !state.up && state.up_right) {
                state.cell_state = true;
            } else if (!state.up_left && state.up && !state.up_right) {
                state.cell_state = true;
            } else if (state.up_left && !state.up && !state.up_right) {
                state.cell_state = false;
            } else if (!state.up_left && !state.up && !state.up_right) {
                state.cell_state = false;
            }
        }


        //USER DEFINED
        // virtual std::tuple<TIME, VALUE> local_comupation_function(TIME e, typename make_message_bags<input_ports>::type mbs) = 0;
        // virtual std::tuple<TIME, VALUE> local_comupation_confluence_function(TIME e, typename make_message_bags<input_ports>::type mbs) = 0;
        std::pair<TIME, VALUE> local_comupation_function(TIME e, typename cadmium::make_message_bags<input_ports>::type mbs) {
            for (auto msg : cadmium::get_messages<typename defs::in>(mbs)) {
                position sender = msg.first;
                if (sender == up_left(my_position)) {
                    state.up_left = msg.second;
                } else if (sender == up_right(my_position)) {
                    state.up_right = msg.second;
                } else if (sender == up(my_position)) {
                    state.up = msg.second;
                }
            }
            update_state();
            return std::make_pair(DURATION, state.cell_state);
        };

        constexpr cell_atomic() noexcept {}


        cell_atomic(position _pos, VALUE initial) {
            my_position = _pos;
            state.cell_state = initial;
            state.delayed_outputs.push(std::make_pair(1, state.cell_state));
            state.next_internal = 1;
        }

        void internal_transition() {
            state.delayed_outputs.pop();
        }

        void external_transition(TIME e, typename cadmium::make_message_bags<input_ports>::type mbs) {
            auto computed_output = local_comupation_function(e, mbs);
            state.delayed_outputs.push(computed_output);
            state.next_internal = state.delayed_outputs.top().first;
        }

        void confluence_transition(TIME e, typename cadmium::make_message_bags<input_ports>::type mbs) {
            internal_transition();
            external_transition(e, mbs);
        }

        typename cadmium::make_message_bags<output_ports>::type output() const {
            typename cadmium::make_message_bags<output_ports>::type bags;

            std::pair<TIME, VALUE> out = state.delayed_outputs.top();
            cadmium::get_messages<typename defs::out>(bags).push_back(std::make_pair(my_position, out.second));
            return bags;

        }

        TIME time_advance() const {
            return state.next_internal;
        }


        friend std::ostringstream& operator<<(std::ostringstream& os, const typename cell_atomic<TIME>::state_type& i) {
            os << i.cell_state;
            return os;
        }
};
