from Chessnut import Game
from modules import chess_agent

def agent(obs) -> str:
    game = Game(obs.board)
    fen_str = game.get_fen()
    depth = 4
    _, move = chess_agent.minimax_agent(fen_str, depth, False)
    return move