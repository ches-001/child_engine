import os
import subprocess
from Chessnut import Game
from typing import Tuple, Optional

INPUT_NEGAMAX_PREFIX = "play_negamax: "
INPUT_ID_PV_PREFIX = "play_id_pv: "
OUTPUT_PREFIX = "best move: "
EXIT_COMMAND = "exit"

class ChessEngine:
    def __init__(self, engine_path: str, use_tt: bool=True):
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
        self.use_tt = use_tt
        
        self.process = subprocess.Popen(
            [command, f"{int(use_tt)}"],
            stdin=subprocess.PIPE, 
            stdout=subprocess.PIPE, 
            text=True
        )

    def run_negamax(self, fen_str: str, depth: int=4) -> Tuple[str, int]:
        self.process.stdin.write(INPUT_NEGAMAX_PREFIX + str(depth) + " " + fen_str + "\n")
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
    

    def run_id_pv(self, fen_str: str, timeout: int=100) -> Tuple[str, int, int]:
        self.process.stdin.write(INPUT_ID_PV_PREFIX + str(timeout) + " " + fen_str + "\n")
        self.process.stdin.flush()
        output = None
        while True:
            output = self.process.stdout.readline()
            if output and output.startswith(OUTPUT_PREFIX):
                break
        self.process.stdout.flush()
        move, score, depth = output.replace(OUTPUT_PREFIX, "").strip().split()
        score, depth = int(score), int(depth)
        return move, score, depth
    
    def exit_process(self):
        self.process.stdin.write(EXIT_COMMAND + "\n")
        self.process.stdin.flush()


ENGINE = None

def agent(obs) -> str:
    global ENGINE
    if ENGINE is None:
        ENGINE = ChessEngine("engine/chess_engine", use_tt=True)
    game = Game(obs.board)
    fen_str = game.get_fen()
    move, _ = ENGINE.run_negamax(fen_str, depth=5)
    return move