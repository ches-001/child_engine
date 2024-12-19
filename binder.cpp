// #include "include/mcts.hpp"
// #include "include/minimax.hpp"
// #include <pybind11/pybind11.h>

// namespace py = pybind11;

// PYBIND11_MODULE(chess_agent, handle) {
//     handle.doc() = "Chess Agents";
//     handle.def(
//         "minimax_agent", 
//         &minimax_agent, 
//         "Minimax implementation to compute the next best chess game move given the current position",
//         py::arg("fen_pos"),
//         py::arg("depth"),
//         py::arg("use_negamax")
//     );
//     handle.def(
//         "mcts_agent", 
//         &mcts_agent, 
//         "MCTS implementation to compute the next best chess game move given the current position",
//         py::arg("fen_pos"),
//         py::arg("c"),
//         py::arg("rollout_depth"),
//         py::arg("max_iter")
//     );
// }