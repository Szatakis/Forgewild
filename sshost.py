import pygame
import os, sys
import json, xml
import _thread, random
import socket
import requests

class SShost:
    def __init__(self):
        self.ping = 0
        self.port = 17625
        self.active = False
        self.serverip = "0"
        self.public_ip = "0"
        self.local_ipv4 = "0"

    def connect(self):
        pass

    def disconnect(self):
        pass

    def host(self):
        def ping_client():
            pass
        def get_public_ip():
            ip = requests.get("https://api.ipify.org").text
            return ip
        def get_local_ipv4():
            s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            s.connect(("8.8.8.8", 80))  # połączenie nie jest wysyłane, tylko ustawia interfejs
            ip = s.getsockname()[0]
            s.close()
            return ip
        self.public_ip = (get_public_ip())
        self.local_ipv4 = (get_local_ipv4())
        print("Server Debbuging Console")
        print("  Public IP:", self.public_ip)
        print("  Local IPv4:", self.local_ipv4)
        print("  Port:", self.port)
        print("")
        print("  Is Server Status Active:", self.active)