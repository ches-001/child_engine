#include "include/negamax_search.hpp"
// #include "include/id_pv_search.hpp"

std::string const INPUT_PREFIX = "play: ";
std::string const OUTPUT_PREFIX = "best move: ";
std::string const EXIT_COMMAND = "exit";

int main(int argc, char *argv[]){
    bool use_ttable = (argc >= 2) ? std::stoi(argv[1]) : true;
    std::string command;
    std::string fen_str;
    int depth;
    std::string::size_type dsz;
    pair_t<std::string, int16_t> best_move;

    int pfsz = INPUT_PREFIX.length();
    map_t<uint64_t, TTEntry> *TTABLE = use_ttable ? new map_t<uint64_t, TTEntry>() : nullptr;
    
    std::cout<< "Input format: " << INPUT_PREFIX << "<DEPTH> <FEN_STRING>" << std::endl;
    
    while(true){
        std::cout 
        << "Awaiting command, " << "enter '" << EXIT_COMMAND 
        << "' to exit this program or press Ctrl-C" << std::endl;

        std::getline(std::cin, command);
        if(command.substr(0, INPUT_PREFIX.length()) == INPUT_PREFIX){
            command = command.substr(INPUT_PREFIX.length());
            depth = std::stoi(command, &dsz);
            fen_str = command.substr(dsz + 1);
            best_move = negamax_agent(fen_str, depth, TTABLE, false);
            std::cout << OUTPUT_PREFIX << best_move.first << " " << best_move.second << std::endl;
        }
        else if(command == EXIT_COMMAND){
            std::cout << "Exiting program!" << std::endl;
            break;
        }
        else{
            std::cout << "Unkown command!" << std::endl;
        }
    }
    delete TTABLE;
}