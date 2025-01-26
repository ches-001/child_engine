import os
import subprocess
from Chessnut import Game
from typing import Tuple, Optional

INPUT_PREFIX = "play: "
OUTPUT_PREFIX = "best move: "
EXIT_COMMAND = "exit"

class ChessEngine:
    def __init__(self, engine_path: str, depth: int=4, use_tt: bool=True):
        if os.name == "nt":
            engine_path += ".exe"
            command = engine_path
        elif os.name == "posix":
            engine_path += ".out"
            command = "./" + engine_path
        else:
            raise RuntimeError(f"Unkown OS name {os.name}")
        
        if not os.path.exists(engine_path):
            raise FileNotFoundError(f"{engine_path} is not found")
        self.engine_path = engine_path
        self.depth = depth
        self.use_tt = use_tt
        
        self.process = subprocess.Popen(
            [command, f"{int(use_tt)}"],
            stdin=subprocess.PIPE, 
            stdout=subprocess.PIPE, 
            text=True
        )

    def get_best_move(self, fen_str: str, depth: Optional[int]=None) -> Tuple[str, int]:
        self.process.stdin.write(INPUT_PREFIX + str((depth or self.depth)) + " " + fen_str + "\n")
        self.process.stdin.flush()
        output = None
        while True:
            output = self.process.stdout.readline()
            if output and output.startswith(OUTPUT_PREFIX):
                break
        self.process.stdout.flush()
        move, score = output.replace(OUTPUT_PREFIX, "").strip().split()
        score = int(score)
        return move, score
    
    def exit_process(self):
        self.process.stdin.write(EXIT_COMMAND + "\n")
        self.process.stdin.flush()


ENGINE = None

def agent(obs) -> str:
    global ENGINE
    if ENGINE is None:
        ENGINE = ChessEngine("engine/chess_engine", depth=5, use_tt=True)
    game = Game(obs.board)
    fen_str = game.get_fen()
    move, _ = ENGINE.get_best_move(fen_str)
    return move