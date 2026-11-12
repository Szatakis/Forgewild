import random

house_parts = [
    "house_part0", "house_part1", "house_part2", "house_part3", "house_part4", "house_part5",
    "house_part6", "house_part7", "house_part8", "house_part9", "house_part10", "house_part11",
    "house_part12", "house_part13", "house_part14", "house_part15", "house_part16", "house_part17",
    "house_part18", "house_part19", "house_part20", "house_part21", "house_part22", "house_part23"
]

# ==========================================================
# GŁÓWNA FUNKCJA
# ==========================================================
def generate_smooth_map(width, height, dungeoncp=False):
    map_data = []
    surface_heights = []

    # --------------------
    # GENEROWANIE TERENU
    # --------------------
    h = random.randint(24, 28)
    plateau_min = 3
    plateau_counter = 0
    direction = 0

    for i in range(width):
        if plateau_counter == 0:
            direction = random.choice([-1, 0, 1])
            plateau_counter = plateau_min

        surface_heights.append(h)
        plateau_counter -= 1

        if plateau_counter == 0:
            h += direction
            h = max(20, min(32, h))

    # --------------------
    # BUDOWA MAPY
    # --------------------
    for y in range(height):
        row = []
        for x in range(width):
            s = surface_heights[x]
            if y < s:
                row.append("air")
            elif y == s:
                row.append("grass_bl")
            elif y == s + 1:
                row.append("dirt_top")
            elif s + 1 < y <= s + 4:
                row.append("dirt_middle")
            elif y == s + 5:
                row.append("dirt_bottom")
            else:
                row.append("stone")
        map_data.append(row)

    # --------------------
    # POZYCJE
    # --------------------
    house_x_block = width // 4
    house_width = 6

    treedungeon_width = 10  # 10 tile = 160px
    treedungeon_x_block = random.randint(width // 4 + 200, width // 4 + 500)

    # -------------------------
    # FUNKCJE POMOCNICZE
    # -------------------------
    def is_house_area(x, y):
        return house_x_block <= x < house_x_block + house_width

    def find_grass_y(map_data, x):
        for y in range(len(map_data)):
            if map_data[y][x] == "grass_bl":
                return y
        return None

    def find_ground_y(map_data, x):
        for y in range(len(map_data)):
            if map_data[y][x] == "grass_bl":
                return y
        return 10

    def clear_column_above(map_data, x, from_y):
        for y in range(0, from_y):
            map_data[y][x] = "air"

    # -------------------------
    # DEKORACJE
    # -------------------------
    for x in range(width):
        for y in range(height):
            if map_data[y][x] == "grass_bl" and y > 0 and not is_house_area(x, y):
                r = random.random()
                if r < 0.2:
                    map_data[y-1][x] = "grass_le"
                elif r < 0.25:
                    map_data[y-1][x] = "bush"
                elif r < 0.27:
                    map_data[y-1][x] = "flower0"
                elif r < 0.29:
                    map_data[y-1][x] = "flower1"
                elif r < 0.31:
                    map_data[y-1][x] = "flower2"
                break

    # -------------------------
    # DRZEWA (BEZ ZMIAN)
    # -------------------------
    def can_place_tree(x, y):
        for xx in range(x-2, x+3):
            for yy in range(y-4, y+1):
                if 0 <= xx < width and 0 <= yy < height:
                    if str(map_data[yy][xx]).startswith("tree") or is_house_area(xx, yy):
                        return False
        return True

    for x in range(2, width - 2):
        for y in range(height):
            if map_data[y][x] in ("grass_le", "bush"):
                if random.random() < 0.25 and y > 4 and can_place_tree(x, y):
                    t = random.choice([0, 1])
                    base = f"tree{t}_part"

                    map_data[y][x]     = base + "10"
                    map_data[y-1][x]   = base + "7"
                    map_data[y-2][x]   = base + "4"
                    map_data[y-3][x]   = base + "1"

                    map_data[y][x-1]   = base + "9"
                    map_data[y-1][x-1] = base + "6"
                    map_data[y-2][x-1] = base + "3"
                    map_data[y-3][x-1] = base + "0"

                    map_data[y][x+1]   = base + "11"
                    map_data[y-1][x+1] = base + "8"
                    map_data[y-2][x+1] = base + "5"
                    map_data[y-3][x+1] = base + "2"
                break

    # -------------------------
    # DOMEK (BEZ ZMIAN)
    # -------------------------
    house_y_block = find_ground_y(map_data, house_x_block)
    for dx in range(house_width):
        map_data[house_y_block][house_x_block + dx] = "grass_bl"
        map_data[house_y_block + 1][house_x_block + dx] = "dirt_top"

    place_house(map_data, house_x_block, house_y_block - 4)

    # -------------------------
    # TREE DUNGEON (JEDYNE ZMIANY)
    # -------------------------
    ground_levels = []
    for dx in range(treedungeon_width):
        x = treedungeon_x_block + dx
        ground_levels.append(find_ground_y(map_data, x))

    dungeon_ground_y = max(ground_levels)

    # NISZCZENIE TERENU NAD DUNGEONEM
    for dx in range(treedungeon_width):
        x = treedungeon_x_block + dx
        clear_column_above(map_data, x, dungeon_ground_y)

        map_data[dungeon_ground_y][x] = "grass_bl"
        map_data[dungeon_ground_y + 1][x] = "dirt_top"

    place_treedungeon(map_data, treedungeon_x_block, dungeon_ground_y - 10, dungeoncp)

    # -------------------------
    # BARIERA NA KRANCACH MAPY
    # -------------------------
    for x in (0, width - 1):
        grass_y = find_grass_y(map_data, x)
        if grass_y is not None:
            for y in range(0, grass_y):
                map_data[y][x] = "barier"

    return map_data


# ==========================================================
# STRUKTURY
# ==========================================================
def place_house(map_data, x_block, y_block):
    parts = [
        (0, 0, "house_part0"), (1, 0, "house_part1"), (2, 0, "house_part2"),
        (3, 0, "house_part3"), (4, 0, "house_part4"), (5, 0, "house_part5"),
        (0, 1, "house_part6"), (1, 1, "house_part7"), (2, 1, "house_part8"),
        (3, 1, "house_part9"), (4, 1, "house_part10"), (5, 1, "house_part11"),
        (0, 2, "house_part12"), (1, 2, "house_part13"), (2, 2, "house_part14"),
        (3, 2, "house_part15"), (4, 2, "house_part16"), (5, 2, "house_part17"),
        (0, 3, "house_part18"), (1, 3, "house_part19"), (2, 3, "house_part20"),
        (3, 3, "house_part21"), (4, 3, "house_part22"), (5, 3, "house_part23")
    ]

    for dx, dy, tile in parts:
        map_data[y_block + dy][x_block + dx] = tile


def place_treedungeon(map_data, x_block, y_block, dungeoncp):
    tile = "tree_hiveoff" if dungeoncp else "tree_hiveon"
    map_data[y_block][x_block] = tile
