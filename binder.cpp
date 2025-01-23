// #include "include/minimax.hpp"
// #include "include/id_pv_search.hpp"
// #include <pybind11/pybind11.h>

// namespace py = pybind11;

// PYBIND11_MODULE(chess_agent, handle) {
//     handle.doc() = "Chess Agents";
//     handle.def(
//         "minimax_agent", 
//         &minimax_agent, 
//         "Minimax chess implementation",
//         py::arg("fen_pos"),
//         py::arg("depth"),
//         py::arg("tt")
//     );

//     handle.def(
//         "id_pv_search_agent", 
//         &id_pv_search_agent, 
//         "PV search chess implementation with iterative deepening",
//         py::arg("fen_pos"),
//         py::arg("start_depth"),
//         py::arg("depth_increment"),
//         py::arg("timelimit_ms"),
//         py::arg("tt")
//     );

//     handle.def(
//         "mcts_agent", 
//         &mcts_agent, 
//         "MCTS chess implementation",
//         py::arg("fen_pos"),
//         py::arg("c"),
//         py::arg("rollout_depth"),
//         py::arg("max_iter")
//     );
// }