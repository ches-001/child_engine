#include "include/negamax_search.hpp"
#include "include/id_pv_search.hpp"

std::string const INPUT_NEGAMAX_PREFIX = "play_negamax: ";
std::string const INPUT_ID_PV_PREFIX = "play_id_pv: ";
std::string const OUTPUT_PREFIX = "best move: ";
std::string const EXIT_COMMAND = "exit";

int main(int argc, char *argv[]){
    bool use_ttable = (argc >= 2) ? std::stoi(argv[1]) : true;
    std::string command;
    std::string fen_str;
    std::string::size_type dsz;

    TranspositionTable *TTABLE = use_ttable ? new TranspositionTable() : nullptr;
    
    std::cout<< "Input format: " << INPUT_NEGAMAX_PREFIX << "<DEPTH> <FEN_STRING>" << std::endl;
    std::cout<< "Input format: " << INPUT_ID_PV_PREFIX << "<TIMEOUT(ms)> <FEN_STRING>" << std::endl;
    
    while(true){
        std::cout 
        << "Awaiting command, " << "enter '" << EXIT_COMMAND 
        << "' to exit this program or press Ctrl-C" << std::endl;

        std::getline(std::cin, command);
        if(command.substr(0, INPUT_NEGAMAX_PREFIX.length()) == INPUT_NEGAMAX_PREFIX){
            command = command.substr(INPUT_NEGAMAX_PREFIX.length());
            int depth = std::stoi(command, &dsz);
            fen_str = command.substr(dsz + 1);
            pair_t<std::string, int16_t> best_move = negamax_agent(fen_str, depth, TTABLE, false);
            std::cout << OUTPUT_PREFIX << best_move.first << " " << best_move.second << std::endl;
        }
        else if(command.substr(0, INPUT_ID_PV_PREFIX.length()) == INPUT_ID_PV_PREFIX){
            command = command.substr(INPUT_ID_PV_PREFIX.length());
            int timeout = std::stoi(command, &dsz);
            fen_str = command.substr(dsz + 1);
            pair_t<pair_t<std::string, int16_t>, int> best_move = id_pv_search_agent(fen_str, timeout, TTABLE, false);
            std::cout << OUTPUT_PREFIX << best_move.first.first 
            << " " << best_move.first.second << " " << best_move.second << std::endl;
        }
        else if(command == EXIT_COMMAND){
            std::cout << "Exiting program!" << std::endl;
            break;
        }
        else{
            std::cout << "Unkown command!" << std::endl;
            continue;
        }
    }
    delete TTABLE;
}