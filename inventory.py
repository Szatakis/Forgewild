import pygame
import os

base_path = os.path.dirname(os.path.abspath(__file__))

class Item:
    def __init__(self, name: str, image: pygame.Surface, quantity: int = 1):
        self.name = name
        self.image = image
        self.quantity = quantity

class Inventory:
    def __init__(self, cols=8, rows=4):
        self.cols = cols
        self.rows = rows
        self.slots = [[None for _ in range(cols)] for _ in range(rows)]
        self.visible = False

        self.slot_size = 45
        self.margin_x = 13
        self.margin_y = 23

        self.active_slot = 0
        self.invgraphic = pygame.image.load(
            os.path.join(base_path, "Textures", "Menu", "hudinv.png")
        ).convert_alpha()

        self.drag_item = None
        self.drag_pos = (0, 0)
        self.drag_from = (0, 0)
        self.selected_name = ""

    def add_item(self, item: Item):
        for y in range(self.rows):
            for x in range(self.cols):
                if self.slots[y][x] is None:
                    self.slots[y][x] = item
                    return True
        return False

    def handle_input(self, keys):
        for i in range(self.cols):
            if keys[pygame.K_1 + i]:
                self.active_slot = i
                item = self.slots[0][i]
                self.selected_name = item.name if item else ""

    def handle_mouse(self, event, surface, LOGICAL_WIDTH, LOGICAL_HEIGHT, screen):
        if not self.visible:
            return

        # szerokość i wysokość inventory w układzie logicznym
        inv_width = self.cols * (self.slot_size + self.margin_x) + self.margin_x
        inv_height = self.rows * (self.slot_size + self.margin_y) + self.margin_y
        inv_x = (LOGICAL_WIDTH - inv_width) // 2 - 8
        inv_y = (LOGICAL_HEIGHT - inv_height) // 2 - 129

        # pozycja myszy w screen
        mouse_x, mouse_y = pygame.mouse.get_pos()

        # przeskalowanie do układu logicznego
        scale_x = screen.get_width() / LOGICAL_WIDTH
        scale_y = screen.get_height() / LOGICAL_HEIGHT
        logical_mouse_x = mouse_x / scale_x
        logical_mouse_y = mouse_y / scale_y

        # obliczenie slotu względem LOGICAL_WIDTH/HEIGHT
        slot_x = int((logical_mouse_x - inv_x - self.margin_x) // (self.slot_size + self.margin_x))
        slot_y = int((logical_mouse_y - inv_y - self.margin_y) // (self.slot_size + self.margin_y))

        # obsługa kliknięcia
        if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
            if 0 <= slot_x < self.cols and 0 <= slot_y < self.rows:
                if self.slots[slot_y][slot_x]:
                    self.drag_item = self.slots[slot_y][slot_x]
                    self.drag_from = (slot_x, slot_y)
                    self.slots[slot_y][slot_x] = None
                    self.selected_name = self.drag_item.name

        elif event.type == pygame.MOUSEBUTTONUP and event.button == 1:
            if self.drag_item:
                if 0 <= slot_x < self.cols and 0 <= slot_y < self.rows:
                    target_item = self.slots[slot_y][slot_x]
                    self.slots[slot_y][slot_x] = self.drag_item
                    if target_item:
                        self.slots[self.drag_from[1]][self.drag_from[0]] = target_item
                else:
                    self.slots[self.drag_from[1]][self.drag_from[0]] = self.drag_item
                self.drag_item = None
                self.selected_name = ""

        # przeciągany item – pozycja w układzie logicznym
        if self.drag_item:
            self.drag_pos = (
                logical_mouse_x - self.slot_size // 2,
                logical_mouse_y - self.slot_size // 2
            )

    def draw(self, surface):
        font = pygame.font.Font("font.TTF", 18)
        inv_width = self.cols * (self.slot_size + self.margin_x) + self.margin_x
        hotbar_y = surface.get_height() - (self.slot_size + self.margin_y) - 15
        hotbar_x = (surface.get_width() - inv_width) // 2 - 7

        # HOTBAR
        for x in range(self.cols):
            slot_x = hotbar_x + self.margin_x + x * (self.slot_size + self.margin_x)
            slot_y = hotbar_y + self.margin_y // 2

            if x == self.active_slot:
                highlight = pygame.Surface((self.slot_size + 6, self.slot_size + 6), pygame.SRCALPHA)
                highlight.fill((230, 193, 201, 128))
                surface.blit(highlight, (slot_x + 6, slot_y + 6))

            item = self.slots[0][x]
            if item:
                surface.blit(item.image, (slot_x + 8, slot_y + 8))
                qty = font.render(str(item.quantity), True, (255, 255, 255))
                surface.blit(qty, (slot_x + self.slot_size - 9, slot_y + self.slot_size - 10))

        # GŁÓWNE INVENTORY
        if not self.visible:
            return

        inv_height = self.rows * (self.slot_size + self.margin_y) + self.margin_y
        inv_x = (surface.get_width() - inv_width) // 2
        inv_y = (surface.get_height() - inv_height) // 2

        surface.blit(self.invgraphic, (250, 20))

        for y in range(self.rows):
            for x in range(self.cols):
                slot_x = inv_x + self.margin_x + x * (self.slot_size + self.margin_x) - 8
                slot_y = inv_y + self.margin_y + y * (self.slot_size + self.margin_y) - 129

                item = self.slots[y][x]
                if item:
                    surface.blit(item.image, (slot_x + 8, slot_y + 8))
                    qty = font.render(str(item.quantity), True, (255, 255, 255))
                    surface.blit(qty, (slot_x + self.slot_size - 9, slot_y + self.slot_size - 10))

        # przeciągany item – rysowany w logicznym układzie
        if self.drag_item:
            surface.blit(self.drag_item.image, self.drag_pos)

        # wyświetlanie nazwy
        if self.selected_name:
            text = font.render(self.selected_name, True, (255, 255, 255))
            surface.blit(text, (surface.get_width() // 2 - text.get_width() // 2, 3))
