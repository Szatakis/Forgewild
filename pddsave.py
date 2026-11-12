import os
import json, xml, socket

def save(map_data, filename, player0_x, player0_y, player1_x, player1_y):
    # Zapis mapy
    with open(filename, "w") as f:
        for row in map_data:
            f.write(",".join(row) + "\n")

    # Zapis pozycji gracza jeśli podano
    stats_path = os.path.join(os.path.dirname(filename), "player_stats.txt")
    with open(stats_path, "w") as f:
        f.write(f"player0_x={player0_x}\n")
        f.write(f"player0_y={player0_y}\n")
        f.write(f"player1_x={player1_x}\n")
        f.write(f"player1_y={player1_y}\n")

class Load:
    def __init__(self):
        self.player0_x = 0
        self.player0_y = 0
        self.player1_x = 0
        self.player1_y = 0
    def load_save(self, filename, pfilename):
        with open(pfilename, "r") as fa:
            for line in fa:
                key, value = line.strip().split("=")
                if key == "player0_x":
                    self.player0_x = float(value)
                elif key == "player0_y":
                    self.player0_y = float(value)
                elif key == "player1_x":
                    self.player1_x = float(value)
                elif key == "player1_y":
                    self.player1_y = float(value)
        with open(filename, "r") as f:
            return [line.strip().split(",") for line in f.readlines()]

def delsave():
    base_path = os.path.dirname(os.path.abspath(__file__))
    map_path = os.path.join(base_path, "PDD", "map_data.txt")
    playerstats_path = os.path.join(base_path, "PDD", "player_stats.txt")

    # usuń pliki jeśli istnieją
    if os.path.exists(map_path):
        os.remove(map_path)
    if os.path.exists(playerstats_path):
        os.remove(playerstats_path)