import pygame
import sys

pygame.init()

# --- Stałe kolorów ---
BUTTON_GRAY = (12, 24, 34)
BUTTON2_GRAY = (24, 36, 30)
BUTTON_TEXT = (255, 255, 255)
BUTTON_TEXT2 = (75, 75, 75)
BUTTON_TRANSLUSENT = (48, 48, 48)

solid_tiles = ["grass_bl", "dirt_top", "dirt_middle", "dirt_bottom", "stone", "barier"]

# --- Funkcje pomocnicze ---
def quit_game():
    pygame.quit()
    sys.exit()

def find_ground_y(map_data, x_block):
    for y in range(len(map_data)):
        if map_data[y][x_block] in solid_tiles:
            return y * 16
    return 0

def check_collision(rect, map_data):
    TILE_SIZE = 16
    MAP_WIDTH_BLOCKS = len(map_data[0])
    MAP_HEIGHT_BLOCKS = len(map_data)
    start_x = max(0, rect.left // TILE_SIZE - 1)
    end_x = min(MAP_WIDTH_BLOCKS, rect.right // TILE_SIZE + 2)
    start_y = max(0, rect.top // TILE_SIZE - 1)
    end_y = min(MAP_HEIGHT_BLOCKS, rect.bottom // TILE_SIZE + 2)
    for y in range(start_y, end_y):
        for x in range(start_x, end_x):
            if map_data[y][x] in solid_tiles:
                if rect.colliderect(pygame.Rect(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE)):
                    return True
    return False

def find_flat_ground_for_house(map_data, center_x_block):
    MAP_WIDTH_BLOCKS = len(map_data[0])
    MAP_HEIGHT_BLOCKS = len(map_data)
    required_left, required_right = 3, 5
    total_width = required_left + 1 + required_right

    def get_ground_y(x):
        for y in range(MAP_HEIGHT_BLOCKS):
            if map_data[y][x] in solid_tiles:
                return y
        return None

    for offset in range(-5, 6):
        x = center_x_block + offset
        start_x, end_x = x - required_left, x + required_right
        if start_x < 0 or end_x >= MAP_WIDTH_BLOCKS:
            continue
        heights = [get_ground_y(start_x + dx) for dx in range(total_width)]
        if None not in heights and all(abs(heights[i] - heights[i + 1]) <= 1 for i in range(len(heights) - 1)):
            return x * 16, min(heights) * 16

    target_y = get_ground_y(center_x_block) or (MAP_HEIGHT_BLOCKS - 10)
    for dx in range(-required_left, required_right + 1):
        x = center_x_block + dx
        if 0 <= x < MAP_WIDTH_BLOCKS:
            for y in range(MAP_HEIGHT_BLOCKS):
                if y < target_y:
                    map_data[y][x] = "air"
                elif y == target_y:
                    map_data[y][x] = "grass_bl"
                elif target_y < y <= target_y + 1:
                    map_data[y][x] = "dirt_top"
                elif target_y + 1 < y <= target_y + 4:
                    map_data[y][x] = "dirt_middle"
                else:
                    map_data[y][x] = "stone"
    return center_x_block * 16, target_y * 16

class ButtonMenu:
    def __init__(self, text, pos, x, y, border, fsize,action=None):
        self.text = text
        self.pos = pos
        self.action = action
        self.width = x
        self.height = y
        self.border = border
        self.font = pygame.font.Font("font.TTF", fsize)
        self.rect = pygame.Rect(pos[0], pos[1], self.width, self.height)

    def draw(self, surface):
        # tło
        # obramówka
        if self.border:
            pygame.draw.rect(surface, BUTTON2_GRAY, self.rect, 4)
        # tekst
        text_surf = self.font.render(self.text, True, BUTTON_TEXT)
        text_rect = text_surf.get_rect(center=self.rect.center)
        surface.blit(text_surf, text_rect)

    def check_click(self, mouse_pos):
        if self.rect.collidepoint(mouse_pos):
            if self.action:
                self.action()

class Slider:
    def __init__(self, x, y, width, height, min_value, max_value, start_value=None):
        self.rect = pygame.Rect(x, y, width, height)
        self.min = min_value
        self.max = max_value
        self.value = start_value if start_value is not None else min_value
        self.knob_width = 20
        self.knob_height = height
        self.dragging = False

    def draw(self, surface):
        # Pasek

        # Obliczenie pozycji gałki
        knob_x = self.rect.x + (self.value - self.min) / (self.max - self.min) * (self.rect.width - self.knob_width)
        knob_rect = pygame.Rect(knob_x, self.rect.y, self.knob_width, self.knob_height)

        # Gałka
        pygame.draw.rect(surface, (200, 200, 200), knob_rect)

    def handle_event(self, event):
        if event.type == pygame.MOUSEBUTTONDOWN:
            if event.button == 1:
                if self.rect.collidepoint(event.pos):
                    self.dragging = True

        elif event.type == pygame.MOUSEBUTTONUP:
            if event.button == 1:
                self.dragging = False

        elif event.type == pygame.MOUSEMOTION and self.dragging:
            # Przesuwanie gałki
            relative_x = event.pos[0] - self.rect.x
            relative_x = max(0, min(relative_x, self.rect.width))
            self.value = self.min + (relative_x / self.rect.width) * (self.max - self.min)

    def get_value(self):
        return int(self.value)
