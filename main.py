import pygame
import os
import sys
import _thread
import socket

from mapgenerator import generate_smooth_map
from pddsave import save, Load, delsave
from defi import quit_game, find_ground_y, ButtonMenu, Slider
from playercc import Player0
from playerco import Player1
from inventory import Inventory, Item
from sshost import SShost

pygame.init()
pygame.mixer.init()

volume = 0.04
sfx_volume = 0.04
music_volume = 0.04

current_music = None
current_music2 = None

dungeoncp = False

load = Load()

# --- Klawisze Gracza Main(1) ---
Jump_Button = pygame.K_SPACE
Jump_Button1 = pygame.K_w
LongJump_Button = pygame.K_e
Crought_Button = pygame.K_s
Left_Button = pygame.K_a
Right_Button = pygame.K_d
Inventory_Button = pygame.K_TAB
ItemDrop_Button = pygame.K_q
Ability1_Button = pygame.K_z
Ability2_Button = pygame.K_x
Dash_Button = pygame.K_r
Sprint_Button = pygame.K_LSHIFT
Break_Button = 1
Place_Button = 3

# --- Klawisze Gracza Slave(2) ---
# gracz poboczny
Jump1_Button = pygame.K_UP
LongJump1_Button = pygame.K_SLASH
Crought1_Button = pygame.K_DOWN
Left1_Button = pygame.K_LEFT
Right1_Button = pygame.K_RIGHT
Dash1_Button = pygame.K_BACKSLASH
Break1_Button = pygame.K_COMMA
Place1_Button = pygame.K_PERIOD

# Stałe gry
LOGICAL_WIDTH = 960
LOGICAL_HEIGHT = 540
TILE_SIZE = 16
MAP_WIDTH_BLOCKS = 12000
MAP_HEIGHT_BLOCKS = 70

minimap_width = 120
minimap_height = 120

minimap_pos_x = 830  # 10 px od prawej krawędzi
minimap_pos_y = 25

BLACK = (0, 0, 0)

is_fullscreen = False

rinun = True

logical_surface = pygame.Surface((LOGICAL_WIDTH, LOGICAL_HEIGHT), pygame.SRCALPHA)

screen = pygame.display.set_mode((LOGICAL_WIDTH, LOGICAL_HEIGHT), pygame.RESIZABLE)
pygame.display.set_caption("Forgewild")

base_path = os.path.dirname(os.path.abspath(__file__))
player1og = False

pygame.mouse.set_visible(False)
cursor_img = pygame.image.load(os.path.join(base_path, "Textures", "cursor.png")).convert_alpha()

# ===============================================================
#                      Tła, HUD & grafika
# ===============================================================
settings_background = pygame.image.load(os.path.join(base_path, "Textures", "Menu", "bgsettings.png")).convert()
menu_background = pygame.image.load(os.path.join(base_path, "Textures", "Menu", "bgmenu.png")).convert()
menu_background2 = pygame.image.load(os.path.join(base_path, "Textures", "Menu", "bgplay.png")).convert()
hudgraphic = pygame.image.load(os.path.join(base_path, "Textures", "Menu", "hud.png")).convert_alpha()
pausemenu_background = pygame.image.load(os.path.join(base_path, "Textures", "Menu", "bgpausemenu.png")).convert()
playerog_background = pygame.image.load(os.path.join(base_path, "Textures", "Menu", "playerpopup.png")).convert()
spopup_background = pygame.image.load(os.path.join(base_path, "Textures", "Menu", "spopup.png")).convert()
volumepopup_background = pygame.image.load(os.path.join(base_path, "Textures", "Menu", "volumepopup.png")).convert()
langpopup_background = pygame.image.load(os.path.join(base_path, "Textures", "Menu", "languagepopup.png")).convert()
autosavpopup_background = pygame.image.load(os.path.join(base_path, "Textures", "Menu", "autosavepopup.png")).convert()

# --- Nowe tło gry ---
background = pygame.image.load(os.path.join(base_path, "Textures", "Tiles", "BG", "bggame_night.png")).convert()
background = pygame.transform.scale(background, (960, 540))
background_width = background.get_width()
background1 = pygame.image.load(os.path.join(base_path, "Textures", "Tiles", "BG", "bggame_day.png")).convert()
background1 = pygame.transform.scale(background, (960, 540))
background1_width = background.get_width()

# ===============================================================
#                           Tiles
# ===============================================================
tile_names = [
    "air", "grass_le", "grass_bl", "dirt_top", "dirt_middle", "dirt_bottom", "stone", "barier",
    "flower0", "flower1", "flower2", "bush",
    "house_part0", "house_part1", "house_part2", "house_part3", "house_part4", "house_part5",
    "house_part6", "house_part7", "house_part8", "house_part9", "house_part10", "house_part11",
    "house_part12", "house_part13", "house_part14", "house_part15", "house_part16", "house_part17",
    "house_part18", "house_part19", "house_part20", "house_part21", "house_part22", "house_part23",
    "tree0_part0", "tree0_part1", "tree0_part2",
    "tree0_part3", "tree0_part4", "tree0_part5",
    "tree0_part6", "tree0_part7", "tree0_part8",
    "tree0_part9", "tree0_part10", "tree0_part11",
    "tree1_part0", "tree1_part1", "tree1_part2",
    "tree1_part3", "tree1_part4", "tree1_part5",
    "tree1_part6", "tree1_part7", "tree1_part8",
    "tree1_part9", "tree1_part10", "tree1_part11",
    "tree_hiveon", "tree_hiveoff",
]
tiles = {name: pygame.image.load(os.path.join(base_path, "Textures", "Tiles", f"{name}.png")).convert_alpha() for name in tile_names}

clock = pygame.time.Clock()

def toggle_fullscreen():
    global is_fullscreen
    global screen

    is_fullscreen = not is_fullscreen

    if is_fullscreen:
        screen = pygame.display.set_mode((0, 0), pygame.FULLSCREEN)
    else:
        screen = pygame.display.set_mode((960, 540), pygame.RESIZABLE)

# ===============================================================
#                         AudioMenager
# ===============================================================

def play_menu_music():
    global current_music
    global volume
    global music_volume
    track = os.path.join(base_path, "Audio", "Music", "menu.wav")

    # jeśli to samo gra → nie restartuj
    if current_music == track:
        return

    pygame.mixer.music.stop()
    pygame.mixer.music.load(track)
    pygame.mixer.music.set_volume(((volume + music_volume) - 0.04))
    pygame.mixer.music.play(-1)

    current_music = track
    
def play_game_music():
    global current_music2
    global volume
    global music_volume
    global MUSIC_END_EVENT
    global game_playlist
    global playlist_index

    # jeśli playlisty jeszcze nie ma → stwórz
    if "game_playlist" not in globals():
        game_playlist = [
            os.path.join(base_path, "Audio", "Music", "Game", "1.wav"),
            os.path.join(base_path, "Audio", "Music", "Game", "2.wav"),
            os.path.join(base_path, "Audio", "Music", "Game", "3.wav")
        ]
        playlist_index = 0

        # Pygame event do końca muzyki
        MUSIC_END_EVENT = pygame.USEREVENT + 5
        pygame.mixer.music.set_endevent(MUSIC_END_EVENT)

    track = game_playlist[playlist_index]

    # jeśli ta sama gra → nie restartuj
    if current_music2 == track:
        return

    # ustawienie głośności zależnie od utworu
    if playlist_index == 0:
        track_volume = min(1.0, (volume + music_volume) + 0.1)
    elif playlist_index == 1:
        track_volume = max(0, (volume + music_volume) - 0.05)
    elif playlist_index == 2:
        track_volume = max(0, (volume + music_volume) - 0.06)
    else:
        track_volume = (volume + music_volume)  # zabezpieczenie, jeśli więcej utworów

    pygame.mixer.music.stop()
    pygame.mixer.music.load(track)
    pygame.mixer.music.set_volume(track_volume)
    pygame.mixer.music.play()

    current_music2 = track


def fade_out(surface, duration=0.7):
    fade = pygame.Surface((LOGICAL_WIDTH, LOGICAL_HEIGHT))
    fade.fill((0, 0, 0))
    alpha = 0
    clock_fade = pygame.time.Clock()
    steps = int(duration * 60)

    for i in range(steps):
        alpha = int(255 * (i / steps))
        fade.set_alpha(alpha)
        surface.blit(fade, (0, 0))
        screen.blit(pygame.transform.scale(surface, screen.get_size()), (0, 0))
        pygame.display.flip()
        clock_fade.tick(60)

def delsavem():
    global screen
    global logical_surface

    running = True  # ✅ kontrola pętli

    def back():
        nonlocal running
        running = False  # ✅ przerwij pętlę
    def nott():
        nonlocal running
        running = False  # ✅ przerwij pętlę
    def yestt():
        nonlocal running
        running = False  # ✅ przerwij pętlę
        delsave()

    buttons = [
        ButtonMenu("", (103, 391), 200, 44, False, 40, back),
        ButtonMenu("", (152, 320), 288, 67, False, 20, yestt),
        ButtonMenu("", (505, 320), 290, 67, False, 20, nott)
    ]

    while running:   # ✅ pętla trwa dopóki running = True
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
                quit_game()
            elif event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
                scale_x = screen.get_width() / LOGICAL_WIDTH
                scale_y = screen.get_height() / LOGICAL_HEIGHT
                pos = (event.pos[0] / scale_x, event.pos[1] / scale_y)
                for b in buttons:
                    b.check_click(pos)
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_F11:
                    toggle_fullscreen()

        logical_surface.blit(spopup_background, (0, 0))


        for b in buttons:
            b.draw(logical_surface)

        screen.blit(pygame.transform.scale(logical_surface, screen.get_size()), (0, 0))

        mouse_x, mouse_y = pygame.mouse.get_pos()
        screen.blit(cursor_img, (mouse_x, mouse_y))

        pygame.display.flip()

def autosavett(ismmenu):
    global screen
    global player0
    global player1
    global map_data
    global logical_surface
    
    pdd_folder = os.path.join(base_path, "PDD")
    os.makedirs(pdd_folder, exist_ok=True)
    map_path = os.path.join(pdd_folder, "map_data.txt")
    save_path = os.path.join(pdd_folder, "player_stats.txt")

    running = True  # ✅ kontrola pętli

    if ismmenu:
        pass
    else:
        save(map_data, save_path, player0.rect.x, player0.rect.y, player1.rect.x, player1.rect.y)

    def back():
        nonlocal running
        running = False  # ✅ przerwij pętlę

    buttons = [
        ButtonMenu("", (110, 396), 172, 42, False, 40, back),
    ]

    while running:   # ✅ pętla trwa dopóki running = True
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
                quit_game()
            elif event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
                scale_x = screen.get_width() / LOGICAL_WIDTH
                scale_y = screen.get_height() / LOGICAL_HEIGHT
                pos = (event.pos[0] / scale_x, event.pos[1] / scale_y)
                for b in buttons:
                    b.check_click(pos)
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_F11:
                    toggle_fullscreen()

        logical_surface.blit(autosavpopup_background, (0, 0))


        for b in buttons:
            b.draw(logical_surface)

        screen.blit(pygame.transform.scale(logical_surface, screen.get_size()), (0, 0))

        mouse_x, mouse_y = pygame.mouse.get_pos()
        screen.blit(cursor_img, (mouse_x, mouse_y))

        pygame.display.flip()

def setvol():
    global volume, sfx_volume, music_volume
    global screen
    global logical_surface

    running = True  # ✅ kontrola pętli

    # Utworzenie trzech suwaków
    master_slider = Slider(
        x=379, y=194,
        width=412, height=24,
        min_value=0,
        max_value=1,
        start_value=volume
    )
    sfx_slider = Slider(
        x=379, y=273,
        width=412, height=24,
        min_value=0,
        max_value=1,
        start_value=sfx_volume
    )
    music_slider = Slider(
        x=379, y=363,
        width=412, height=24,
        min_value=0,
        max_value=1,
        start_value=music_volume
    )

    def back():
        nonlocal running
        running = False  # ✅ przerwij pętlę

    buttons = [
        ButtonMenu("", (110, 393), 170, 45, False, 40, back),
    ]

    while running:   # ✅ pętla trwa dopóki running = True
        for event in pygame.event.get():
            # zawsze oblicz skalowanie na początku
            scale_x = screen.get_width() / LOGICAL_WIDTH
            scale_y = screen.get_height() / LOGICAL_HEIGHT

            if event.type == pygame.QUIT:
                running = False
                quit_game()

            elif event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
                pos = (event.pos[0] / scale_x, event.pos[1] / scale_y)
                for b in buttons:
                    b.check_click(pos)

                # obsługa wszystkich suwaków
                for slider in (master_slider, sfx_slider, music_slider):
                    slider.handle_event(pygame.event.Event(event.type, {"pos": pos, "button": event.button}))

            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_F11:
                    toggle_fullscreen()

            elif event.type == pygame.MOUSEBUTTONUP and event.button == 1:
                pos = (event.pos[0] / scale_x, event.pos[1] / scale_y)
                for slider in (master_slider, sfx_slider, music_slider):
                    slider.handle_event(pygame.event.Event(event.type, {"pos": pos, "button": event.button}))

            elif event.type == pygame.MOUSEMOTION:
                pos = (event.pos[0] / scale_x, event.pos[1] / scale_y)
                for slider in (master_slider, sfx_slider, music_slider):
                    slider.handle_event(pygame.event.Event(event.type, {
                        "pos": pos,
                        "rel": event.rel,
                        "buttons": event.buttons
                    }))

        # rysowanie tła
        logical_surface.blit(volumepopup_background, (0, 0))

        # aktualizacja wartości
        volume = master_slider.value
        sfx_volume = sfx_slider.value
        music_volume = music_slider.value

        # rysowanie przycisków
        for b in buttons:
            b.draw(logical_surface)

        # rysowanie suwaków
        master_slider.draw(logical_surface)
        sfx_slider.draw(logical_surface)
        music_slider.draw(logical_surface)

        # skalowanie na ekran
        screen.blit(pygame.transform.scale(logical_surface, screen.get_size()), (0, 0))

        # kursor
        mouse_x, mouse_y = pygame.mouse.get_pos()
        screen.blit(cursor_img, (mouse_x, mouse_y))

        pygame.display.flip()

def scplayer():
    global player1og
    global screen
    global logical_surface

    # czas startu
    start = pygame.time.get_ticks()

    # pętla lokalna funkcji
    while True:
        now = pygame.time.get_ticks()

        # rysuj okienko
        if player1og:
            return
        else:
            logical_surface.blit(playerog_background, (0, 0))

        screen.blit(pygame.transform.scale(logical_surface, screen.get_size()), (0, 0))
        pygame.display.flip()

        # wyjście z funkcji po 2 sekundach
        if now - start > 2000:
            player1og = True
            return

        # obsługa eventów (żeby okno się nie zamroziło)
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                quit_game()

# ===============================================================
#                            GRA
# ===============================================================
def game():
    def pausemenu():
        global screen
        global volume
        global logical_surface
        global rinun
        global player1og

        rinun = True
        popup_active = False
        popup_end_time = 0
    
        pygame.mixer.music.stop()
        pygame.mixer.music.load(os.path.join(base_path, "Audio", "Music", "menu.wav"))
        pygame.mixer.music.set_volume((volume - 0.04))
        pygame.mixer.music.play(-1)   # zapętlone menu
    
        def back():
            global rinun
            rinun = False
            fade_out(logical_surface)
            pygame.mixer.music.stop()

        def menufade():
            fade_out(logical_surface)
            menu()
        def scplayertt():
            result = scplayer()
            if result == "done":
                return   # dalej rysuje pausemenu
        def autosavedd():
            autosavett(False)
    
        buttons = [
            ButtonMenu("", (22, 142), 293, 74, False, 40,setvol),
            ButtonMenu("", (22, 220), 293, 74, False, 40),
            ButtonMenu("", (22, 297), 293, 74, False, 40),
            ButtonMenu("", (22, 379), 293, 74, False, 40, back),
            ButtonMenu("", (507, 142), 293, 74, False, 40, delsavem),
            ButtonMenu("", (507, 220), 293, 74, False, 40, autosavedd),
            ButtonMenu("", (507, 297), 293, 74, False, 40, scplayertt),
            ButtonMenu("", (507, 379), 293, 74, False, 40, menufade)
        ]

        while rinun:
            dt = clock.tick(60) / 1000.0

            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    quit_game()
                elif event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
                    scale_x = screen.get_width() / LOGICAL_WIDTH
                    scale_y = screen.get_height() / LOGICAL_HEIGHT
                    pos = (event.pos[0] / scale_x, event.pos[1] / scale_y)
                    for b in buttons:
                        b.check_click(pos)
                elif event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_F11:
                        toggle_fullscreen()

            keys = pygame.key.get_pressed()
            if rinun:
                logical_surface.blit(pausemenu_background, (0, 0))
            for b in buttons:
                b.draw(logical_surface)

            screen.blit(pygame.transform.scale(logical_surface, screen.get_size()), (0, 0))

            mouse_x, mouse_y = pygame.mouse.get_pos()
            screen.blit(cursor_img, (mouse_x, mouse_y))
            pygame.display.flip()
    
    pygame.mixer.stop()
    global logical_surface
    global playlist_index
    global player0
    global player1
    global map_data

    def inventorygui():
        pass

    def pausefade():
        fade_out(logical_surface)
        pausemenu()

    buttonsgui = [
        ButtonMenu("", (772, 23), 37, 37, False, 40, pausefade),
        ButtonMenu("", (772, 67), 37, 37, False, 40),
        ButtonMenu("", (772, 111), 37, 37, False, 40)
    ]

    # PAUSE BUTTONS
    buttonspause = [
        ButtonMenu("Settings", (10, 175), 290, 78, True, 40),
        ButtonMenu("Codex", (10, 250), 290, 78, True, 40),
        ButtonMenu("Resume", (10, 325), 290, 78, True, 40),
        ButtonMenu("Back To Menu", (10, 402), 290, 78, True, 40, playmenu)
    ]

    global screen

    # PDD folder
    pdd_folder = os.path.join(base_path, "PDD")
    os.makedirs(pdd_folder, exist_ok=True)
    map_path = os.path.join(pdd_folder, "map_data.txt")
    playerstats_path = os.path.join(pdd_folder, "player_stats.txt")

    # Ładowanie lub generowanie mapy
    if not os.path.exists(map_path):
        map_data = generate_smooth_map(MAP_WIDTH_BLOCKS, MAP_HEIGHT_BLOCKS, dungeoncp)
        save(map_data, map_path, 24576, 464, 24576, 464)
    else:
        map_data = Load.load_save(Load, map_path, playerstats_path)

    if not os.path.exists(playerstats_path):
        save(map_data, map_path, 24576, 464, 24576, 464)

    # Spawn
    spawn_x_block = MAP_WIDTH_BLOCKS // 4
    spawn_x_px = spawn_x_block * TILE_SIZE
    spawn_y_px = find_ground_y(map_data, spawn_x_block) - 4 * TILE_SIZE

    wand_img = pygame.image.load(os.path.join(base_path, "Textures", "Items", "wand.png")).convert_alpha()
    berry_img = pygame.image.load(os.path.join(base_path, "Textures", "Items", "berry.png")).convert_alpha()
    sword_img = pygame.image.load(os.path.join(base_path, "Textures", "Items", "sword.png")).convert_alpha()
    stick_img = pygame.image.load(os.path.join(base_path, "Textures", "Items", "stick.png")).convert_alpha()
    wood_img = pygame.image.load(os.path.join(base_path, "Textures", "Items", "wood.png")).convert_alpha()
    leaf_img  = pygame.image.load(os.path.join(base_path, "Textures", "Items", "leaf.png")).convert_alpha()

    inventory = Inventory()

    player0 = Player0(spawn_x_px, spawn_y_px)
    player1 = Player1(spawn_x_px, spawn_y_px)
    
    #dodawanie itemow
    inventory.add_item(Item("Sword", sword_img, 1))
    
    pygame.mixer.music.stop()
    play_game_music()

    while True:
        dt = clock.tick(60) / 1000.0

        # ---------------- EVENTS ----------------
        for event in pygame.event.get():
            inventory.handle_mouse(event, logical_surface, LOGICAL_WIDTH, LOGICAL_HEIGHT, screen)
            if event.type == pygame.QUIT:
                quit_game()
            elif event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
                scale_x = screen.get_width() / LOGICAL_WIDTH
                scale_y = screen.get_height() / LOGICAL_HEIGHT
                pos = (event.pos[0] / scale_x, event.pos[1] / scale_y)
                for b in buttonsgui:
                    b.check_click(pos)
            elif event.type == MUSIC_END_EVENT:
                playlist_index = (playlist_index + 1) % len(game_playlist)
                pygame.mixer.music.stop()
                play_game_music()
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_F11:
                    toggle_fullscreen()
                if event.key == pygame.K_TAB:
                    inventory.visible = not inventory.visible


        keys = pygame.key.get_pressed()
        dx0 = player0.handle_input(keys, Left_Button, Right_Button, Jump_Button, Jump_Button1, LongJump_Button, Ability1_Button, Ability2_Button, Dash_Button, Crought_Button, Sprint_Button, dt)
        dx1 = player1.handle_input(keys, Left1_Button, Right1_Button, Jump1_Button, LongJump1_Button, Ability1_Button, Ability2_Button, Dash1_Button, Crought1_Button, dt)

        inventory.handle_input(keys)

        if event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE:
            pausefade()
        
        # ✅ Fizyczny ruch gracza
        player0.physics(map_data, dx0, dt)
        if player1og:
            player1.physics(map_data, dx1, dt)

        # ✅ Animacja (działa na dt)
        player0.update_animation(dx0, dt)
        if player1og:
            player1.update_animation(dx1, dt)

        # ---------------- CAMERA POSITION ----------------
        if player1og:
            mid_x = (player0.rect.x + player1.rect.x) // 2
            mid_y = (player0.rect.y + player1.rect.y) // 2

            camera_x = max(
                0,
                min(
                    mid_x - LOGICAL_WIDTH // 2,
                    MAP_WIDTH_BLOCKS * TILE_SIZE - LOGICAL_WIDTH
                )
            )

            camera_y = max(
                0,
                min(
                    mid_y - LOGICAL_HEIGHT // 2,
                    MAP_HEIGHT_BLOCKS * TILE_SIZE - LOGICAL_HEIGHT
                )
            )

        else:
            camera_x = max(0, min(player0.rect.x - LOGICAL_WIDTH // 2,
                                  MAP_WIDTH_BLOCKS * TILE_SIZE - LOGICAL_WIDTH))
            camera_y = max(0, min(player0.rect.y - LOGICAL_HEIGHT // 2,
                                  MAP_HEIGHT_BLOCKS * TILE_SIZE - LOGICAL_HEIGHT))

        # --- Parallax background ---
        parallax_factor = 0.3  # im mniejsza wartość, tym wolniej przesuwa się tło
        bg_scroll_x = int((camera_x * parallax_factor) % background_width)
        logical_surface.blit(background, (-bg_scroll_x, 0))
        if bg_scroll_x > 0:
            logical_surface.blit(background, (background_width - bg_scroll_x, 0))

        # --- Tiles ---
        start_x = max(0, camera_x // TILE_SIZE)
        end_x = min(MAP_WIDTH_BLOCKS, (camera_x + LOGICAL_WIDTH) // TILE_SIZE + 1)
        start_y = max(0, camera_y // TILE_SIZE)
        end_y = min(MAP_HEIGHT_BLOCKS, (camera_y + LOGICAL_HEIGHT) // TILE_SIZE + 1)

        #rysowanie mapy
        for y in range(start_y, end_y):
            for x in range(start_x, end_x):
                tile = map_data[y][x]
                if tile in tiles and tile != "air":
                    logical_surface.blit(
                        tiles[tile],
                        (x * TILE_SIZE - camera_x, y * TILE_SIZE - camera_y)
                    )

        for b in buttonsgui:
            b.draw(logical_surface)
        overlay = pygame.Surface((LOGICAL_WIDTH, LOGICAL_HEIGHT), pygame.SRCALPHA)
        overlay.fill((0, 0, 0, 0))  # ostatnia wartość = alfa (0-255)

        # narysuj tło i nakładkę
        logical_surface.blit(overlay, (0, 0))

        # HUD
        logical_surface.blit(hudgraphic, (0, 0))

        #Minimapa
        minimap_surface = pygame.Surface((minimap_width, minimap_height), pygame.SRCALPHA)
        minimap_surface.fill((255, 255, 255, 70))  # tło minimapy
        logical_surface.blit(minimap_surface, (minimap_pos_x, minimap_pos_y))

        

        # Player
        if player1og:
            player1.draw(logical_surface, camera_x, camera_y, dx1)
        player0.draw(logical_surface, camera_x, camera_y, dx0)

        #jakies randowe rzeczy xd
        inventory.draw(logical_surface)

        # 🔸 2. Skalowanie logicznej powierzchni na prawdziwy ekran
        scaled = pygame.transform.scale(logical_surface, screen.get_size())
        screen.blit(scaled, (0, 0))

        # 🔸 3. Rysowanie kursora BEZ skalowania (na screen)
        mouse_x, mouse_y = pygame.mouse.get_pos()
        screen.blit(cursor_img, (mouse_x, mouse_y))

        # 🔸 4. Aktualizacja ekranu
        pygame.display.flip()


# ===============================================================
#                        NEW GAME
# ===============================================================
def newgame():
    pdd_folder = os.path.join(base_path, "PDD")
    os.makedirs(pdd_folder, exist_ok=True)
    map_path = os.path.join(pdd_folder, "map_data.txt")

    map_data = generate_smooth_map(MAP_WIDTH_BLOCKS, MAP_HEIGHT_BLOCKS, False)
    save(map_data, map_path, 24576, 464, 24576, 464)

    game()


# ===============================================================
#                        PLAY MENU
# ===============================================================
def playmenu():
    global screen
    global logical_surface

    play_menu_music()

    def backfade():
        fade_out(logical_surface)
        menu()

    def gamefade():
        fade_out(logical_surface)
        game()

    def newgamefade():
        fade_out(logical_surface)
        newgame()


    buttons = [
        ButtonMenu("", (33, 272), 290, 73, False, 40, gamefade),
        ButtonMenu("", (33, 356), 290, 73, False, 40, newgamefade),
        ButtonMenu("", (33, 439), 290, 73, False, 40, backfade)
    ]

    while True:
        dt = clock.tick(60) / 1000.0

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                quit_game()
            elif event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
                scale_x = screen.get_width() / LOGICAL_WIDTH
                scale_y = screen.get_height() / LOGICAL_HEIGHT
                pos = (event.pos[0] / scale_x, event.pos[1] / scale_y)
                for b in buttons:
                    b.check_click(pos)
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_F11:
                    toggle_fullscreen()

        logical_surface.blit(menu_background2, (0, 0))

        for b in buttons:
            b.draw(logical_surface)

        screen.blit(pygame.transform.scale(logical_surface, screen.get_size()), (0, 0))
        mouse_x, mouse_y = pygame.mouse.get_pos()
        screen.blit(cursor_img, (mouse_x, mouse_y))
        pygame.display.flip()

# ===============================================================
#                        SETTINGS MENU
# ===============================================================
def settingsmenu():
    global screen
    global logical_surface

    play_menu_music()

    def backfade():
        fade_out(logical_surface)
        menu()
    def autosavedd():
        autosavett(True)

    buttons = [
        ButtonMenu("", (22, 142), 293, 74, False, 40, setvol),
        ButtonMenu("", (22, 220), 293, 74, False, 40),
        ButtonMenu("", (22, 297), 293, 74, False, 40),
        ButtonMenu("", (22, 379), 293, 74, False, 40, backfade),
        ButtonMenu("", (507, 142), 293, 74, False, 40, delsavem),
        ButtonMenu("", (507, 220), 293, 74, False, 40, autosavedd),
        ButtonMenu("", (507, 297), 293, 74, False, 40),
        ButtonMenu("", (507, 379), 293, 74, False, 40)
    ]

    while True:
        dt = clock.tick(60) / 1000.0
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                quit_game()
            elif event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
                scale_x = screen.get_width() / LOGICAL_WIDTH
                scale_y = screen.get_height() / LOGICAL_HEIGHT
                pos = (event.pos[0] / scale_x, event.pos[1] / scale_y)
                for b in buttons:
                    b.check_click(pos)
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_F11:
                    toggle_fullscreen()

        logical_surface.blit(settings_background, (0, 0))

        for b in buttons:
            b.draw(logical_surface)

        screen.blit(pygame.transform.scale(logical_surface, screen.get_size()), (0, 0))
        mouse_x, mouse_y = pygame.mouse.get_pos()
        screen.blit(cursor_img, (mouse_x, mouse_y))
        pygame.display.flip()

# ===============================================================
#                        CREDITS
# ===============================================================
def credits():
    global screen
    global logical_surface

    play_menu_music()

    credits_path = os.path.join(base_path, "PDD", "credits.txt")
    bg_path = os.path.join(base_path, "Textures", "Menu", "bgcredits.png")

    try:
        with open(credits_path, "r", encoding="utf-8") as f:
            lines = [line.strip() for line in f.readlines() if line.strip()]
    except:
        lines = ["Error: Can't load credits file."]

    bg_credits = pygame.image.load(bg_path).convert() if os.path.isfile(bg_path) else None
    font_big = pygame.font.Font("font.TTF", 70)
    font = pygame.font.Font("font.TTF", 22)
    font_com = pygame.font.Font("font.TTF", 26)
    small_font = pygame.font.Font("font.TTF", 18)

    y = LOGICAL_HEIGHT
    scroll_speed = 1

    while True:
        dt = clock.tick(60) / 1000.0
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                quit_game()
            elif event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE:
                fade_out(logical_surface)
                return
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_F11:
                    toggle_fullscreen()


        if bg_credits:
            logical_surface.blit(pygame.transform.scale(bg_credits, (LOGICAL_WIDTH, LOGICAL_HEIGHT)), (0, 0))
        else:
            logical_surface.fill((0, 0, 0))

        for i, line in enumerate(lines):
            if i == 0:
                text = font_big.render(line, True, (95, 180, 241))
                offset = i * 70
            elif ":" in line:
                text = font_com.render(line, True, (238, 25, 87))
                offset = i * 50
            else:
                text = font.render(line, True, (255, 255, 255))
                offset = i * 50
            logical_surface.blit(text, text.get_rect(center=(LOGICAL_WIDTH // 2, y + offset)))

        y -= scroll_speed
        if y + len(lines) * 50 < 0:
            fade_out(logical_surface)
            return

        tip_text = small_font.render("Press ESC to return to menu", True, (255, 255, 255))
        logical_surface.blit(tip_text, tip_text.get_rect(topright=(LOGICAL_WIDTH - 10, 10)))

        screen.blit(pygame.transform.scale(logical_surface, screen.get_size()), (0, 0))
        pygame.display.flip()

# ===============================================================
#                         MENU GŁÓWNE
# ===============================================================
def menu():
    global screen
    global logical_surface
    global player1og
    player1og = False

    play_menu_music()
    
    def settingsfade():
        fade_out(logical_surface)
        settingsmenu()

    def playfade():
        fade_out(logical_surface)
        playmenu()

    def creditsfade():
        fade_out(logical_surface)
        credits()

    buttons = [
        ButtonMenu("", (15, 153), 290, 70, False, 40, playfade),
        ButtonMenu("", (15, 247), 290, 78, False, 40, settingsfade),
        ButtonMenu("", (15, 341), 290, 73, False, 40, creditsfade),
        ButtonMenu("", (15, 435), 290, 74, False, 40, quit_game)
    ]

    while True:
        dt = clock.tick(60) / 1000.0
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                quit_game()
            elif event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
                scale_x = screen.get_width() / LOGICAL_WIDTH
                scale_y = screen.get_height() / LOGICAL_HEIGHT
                pos = (event.pos[0] / scale_x, event.pos[1] / scale_y)
                for b in buttons:
                    b.check_click(pos)
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_F11:
                    toggle_fullscreen()

        # półprzezroczysty czarny overlay (efekt „mgły”)
        overlay = pygame.Surface((LOGICAL_WIDTH, LOGICAL_HEIGHT), pygame.SRCALPHA)
        overlay.fill((0, 0, 0, 0))  # ostatnia wartość = alfa (0-255)

        # narysuj tło i nakładkę
        logical_surface.blit(menu_background, (0, 0))
        logical_surface.blit(overlay, (0, 0))

        for b in buttons:
            b.draw(logical_surface)

        screen.blit(pygame.transform.scale(logical_surface, screen.get_size()), (0, 0))
        mouse_x, mouse_y = pygame.mouse.get_pos()
        screen.blit(cursor_img, (mouse_x, mouse_y))
        pygame.display.flip()


# ===============================================================
#                        START GRY
# ===============================================================
if __name__ == "__main__":
    menu()