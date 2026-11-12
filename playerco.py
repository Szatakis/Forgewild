import pygame
import os
from defi import check_collision, find_ground_y

pygame.init()
pygame.mixer.init()

TILE_SIZE = 16
gravity = 1
jump_strength = -10
longjump_strength = -14
move_speed = 180

#jump info
JUMP_COOLDOWN = 0.35
LONGJUMP_COOLDOWN = 1.5

#dash info
DASH_COOLDOWN = 3.0
DASH_SPEED = 8
DASH_DURATION = 0.25  # sekundy

volumesfx = 0.0

base_path = os.path.dirname(os.path.abspath(__file__))

# Sound Effects
jump_sound = pygame.mixer.Sound(os.path.join(base_path, "Audio", "SFX", "jump.wav"))
jump_sound.set_volume((0.05))
dash_sound = pygame.mixer.Sound(os.path.join(base_path, "Audio", "SFX", "dashend.wav"))
dash_sound.set_volume((0.15))

def load_player_images():
    folder = os.path.join(base_path, "Textures", "Player1")

    images = {
        "left": [pygame.image.load(os.path.join(folder, f"left{i}.png")).convert_alpha() for i in range(1, 4)],
        "right": [pygame.image.load(os.path.join(folder, f"right{i}.png")).convert_alpha() for i in range(1, 4)],
        "idle": pygame.image.load(os.path.join(folder, "idle.png")).convert_alpha(),
        "jump_right": [pygame.image.load(os.path.join(folder, f"jump{i}_right.png")).convert_alpha() for i in range(1, 4)],
        "jump_left": [pygame.image.load(os.path.join(folder, f"jump{i}_left.png")).convert_alpha() for i in range(1, 4)],
        "jump_idle": [pygame.image.load(os.path.join(folder, f"jump{i}_idle.png")).convert_alpha() for i in range(1, 4)],
    }

    return images


class Player1:
    def __init__(self, x, y):
        from playerco import load_player_images
        self.images = load_player_images()
        self.rect = pygame.Rect(x, y, TILE_SIZE, TILE_SIZE * 2)

        self.direction = "right"
        self.anim_index = 0
        self.anim_timer = 0
        self.anim_interval = 130  # ms
        self.invdata = [0, 0, 0, 0, 0, 0, 0, 0]

        self.health = 100
        self.stamina = 1000
        self.mglevel = 1

        self.velocity_y = 0
        self.on_ground = False

        self.last_dx = 0

        self.dash_cooldown_timer = 0
        self.dash_timer = 0
        self.is_dashing = False

        self.px = 100
        self.py = 100

        # ---- Jump cooldown ----
        self.jump_cooldown_timer = 0
        self.longjump_cooldown_timer = 0

    # ---------------------------------------------------------
    #                      INPUT
    # ---------------------------------------------------------
    def handle_input(self, keys, left_key, right_key, jump_k1, jump_3,
                 abi_k1, abi_k2, dash_key, crought_key, dt):
        dx = 0
        move_speed_temp = move_speed
        self.anim_interval_temp = self.anim_interval
        # --- Crought ---
        if keys[crought_key] and self.on_ground:
            move_speed_temp -= 120
            self.anim_interval_temp += 70
            #print("crought")

        # RUCH
        if keys[right_key]:
            dx = move_speed_temp
            self.direction = "right"
        elif keys[left_key]:
            dx = -move_speed_temp
            self.direction = "left"

        # Ability 1 / 2
        if keys[abi_k1]:
            #print("ability 1")
            pass
        elif keys[abi_k2]:
            #print("ability 2")
            pass

        # --- Aktualizuj cooldowny ---
        self.jump_cooldown_timer = max(0, self.jump_cooldown_timer - dt)
        self.longjump_cooldown_timer = max(0, self.longjump_cooldown_timer - dt)
        self.dash_cooldown_timer = max(0, self.dash_cooldown_timer - dt)

        # --- Zwykły skok ---
        if keys[jump_k1] and self.on_ground and self.jump_cooldown_timer == 0:
            self.velocity_y = jump_strength
            self.jump_cooldown_timer = JUMP_COOLDOWN
            jump_sound.play()
            #print("jump")

        # --- Długi skok ---
        elif keys[jump_3] and self.on_ground and self.longjump_cooldown_timer == 0:
            self.velocity_y = longjump_strength
            self.longjump_cooldown_timer = LONGJUMP_COOLDOWN
            jump_sound.play()
            #print("long jump")

        # --- Dash ---
        elif keys[dash_key] and self.dash_cooldown_timer == 0 and not self.is_dashing:
            self.is_dashing = True
            self.dash_timer = DASH_DURATION
            self.dash_cooldown_timer = DASH_COOLDOWN
            dash_sound.play()
            #print("DASH START")

        self.last_dx = dx
        return dx


    # ---------------------------------------------------------
    #                       PHYSICS
    # ---------------------------------------------------------
    def physics(self, map_data, dx, dt):
        self.velocity_y += gravity
        dy = self.velocity_y

        self.pos_x = dx * dt

        # ---- Ruch X ----
        new_rect_x = self.rect.move((dx * dt), 0)
        if not check_collision(new_rect_x, map_data):
            self.rect = new_rect_x

        # ---- Ruch Y ----
        new_rect_y = self.rect.move(0, (dy))
        if not check_collision(new_rect_y, map_data):
            self.rect = new_rect_y
            self.on_ground = False
        else:
            # Korekta pozycji
            while dy != 0:
                step = int(dy / abs(dy))
                test_rect = self.rect.move(0, step)
                if check_collision(test_rect, map_data):
                    break
                self.rect = test_rect
                dy -= step

            self.velocity_y = 0
            self.on_ground = True
        
        # --- DASH RUCH (po kolizjach) ---
        if self.is_dashing:
            dash_dir = 1 if self.direction == "right" else -1
            dash_distance = int(DASH_SPEED)

            # Bezpieczne przesuwanie co 1 piksel (sprawdza kolizje)
            for _ in range(dash_distance):
                test_rect = self.rect.move(dash_dir, 0)
                if check_collision(test_rect, map_data):
                    # Uderzył w ścianę — zakończ dash
                    self.is_dashing = False
                    self.dash_timer = 0
                    print("DASH STOPPED BY WALL")
                    break
                self.rect = test_rect

            # Odliczanie czasu dasza
            if self.is_dashing:
                self.dash_timer -= dt
                if self.dash_timer <= 0:
                    self.is_dashing = False
                    print("DASH END")

    # ---------------------------------------------------------
    #                     ANIMATION
    # ---------------------------------------------------------
    def update_animation(self, dx, dt):
        self.anim_timer += dt

        # BIEG
        if dx != 0 and self.on_ground:
            if self.anim_timer >= (self.anim_interval_temp / 1000):
                self.anim_index = (self.anim_index + 1) % 3
                self.anim_timer = 0
            return

        # SKOK
        if not self.on_ground:
            jump_interval = (self.anim_interval_temp * 4) / 1000.0
            if self.anim_timer >= jump_interval:
                self.anim_index = min(self.anim_index + 1, 2)
                self.anim_timer = 0
            return

        # IDLE
        if dx == 0 and self.on_ground:
            self.anim_index = 0

    # ---------------------------------------------------------
    #                       DRAW
    # ---------------------------------------------------------
    def draw(self, surf, cam_x, cam_y, dx):
        frame = self.get_current_frame(dx)
        surf.blit(frame, (self.rect.x - cam_x, self.rect.y - cam_y))

    # ---------------------------------------------------------
    #               FRAME SELECTION (NO FLICKER)
    # ---------------------------------------------------------
    def get_current_frame(self, dx):
        # Anti-flicker
        if dx == 0:
            dx = self.last_dx

        # SKOK
        if not self.on_ground:
            if dx == 0:
                return self.images["jump_idle"][min(self.anim_index, 2)]
            elif self.direction == "right":
                return self.images["jump_right"][min(self.anim_index, 2)]
            else:
                return self.images["jump_left"][min(self.anim_index, 2)]

        # IDLE
        if dx == 0:
            return self.images["idle"]

        # BIEG
        return self.images[self.direction][self.anim_index]