"""Reproduce the two authored maze layouts, or verify them without editing.

Seeds are offline authoring inputs; runtime exploration never receives the wall
layout. Each cell spans 14 tiles, retaining the original 0.35 m pitch at 0.25 scale.
"""

import random
from collections import deque

def generate(n, seed):
    r = random.Random(seed)
    g = [[] for _ in range(n * n)]
    seen = {0}
    stack = [0]
    while stack:
        u = stack[-1]
        x, y = (u % n, u // n)
        choices = [ny * n + nx for nx, ny in ((x + 1, y), (x, y + 1), (x - 1, y), (x, y - 1)) if 0 <= nx < n and 0 <= ny < n and (ny * n + nx not in seen)]
        if not choices:
            stack.pop()
            continue
        v = r.choice(choices)
        g[u].append(v)
        g[v].append(u)
        seen.add(v)
        stack.append(v)
    return g

def metrics(g, n):
    q = deque([0])
    p = {0: -1}
    while q:
        u = q.popleft()
        for v in g[u]:
            if v not in p:
                p[v] = u
                q.append(v)
    path = [n * n - 1]
    while path[-1]:
        path.append(p[path[-1]])
    ds = [b - a for a, b in zip(path, path[1:])]
    visited = set()
    known = [set() for _ in g]
    u = 0
    walk = [0]
    while u != n * n - 1:
        visited.add(u)
        for v in g[u]:
            known[u].add(v)
            known[v].add(u)
        d = {v: 0 for v in range(n * n) if v not in visited}
        q = deque(d)
        while q:
            a = q.popleft()
            for b in known[a]:
                if b not in d:
                    d[b] = d[a] + 1
                    q.append(b)
        nxt = next((v for step in (1, n, -1, -n) for v in known[u] if v - u == step and d[v] == d[u] - 1))
        walk.append(nxt)
        u = nxt
    return (len(ds), sum((a != b for a, b in zip(ds, ds[1:]))), sum((len(v) == 1 for v in g)), len(walk) - 1)
LEVELS = (('Medium', 12, 1732), ('Hard', 16, 1326))

def tilemap(g, n):
    size = n * 14 + 1
    walls = [[0] * size for _ in range(size)]
    for y in range(size):
        for x in range(size):
            if x % 14 == 0 or y % 14 == 0:
                walls[y][x] = 1
    for u, neighbours in enumerate(g):
        x, y = (u % n, u // n)
        for v in neighbours:
            if v == u + 1:
                for k in range(1, 14):
                    walls[y * 14 + k][(x + 1) * 14] = 0
            elif v == u + n:
                for k in range(1, 14):
                    walls[(y + 1) * 14][x * 14 + k] = 0
    surface = [[0 if v else 2 for v in row] for row in walls]
    markings = [[0] * size for _ in range(size)]
    for cell, colour in ((0, 5), (n - 1, 4)):
        for y in range(cell * 14 + 5, cell * 14 + 10):
            for x in range(cell * 14 + 5, cell * 14 + 10):
                markings[y][x] = colour
    lines = ['PIPEFRAME_TILEMAP 4', f'{size} {size} 0.100000001 0 0', '""', '6', '1 170 184 193 255 1', '2 40 48 57 255 0', '3 62 88 68 255 0', '4 231 190 66 255 0', '5 62 187 146 255 0', '6 230 235 229 255 0', '3']
    for title, collision, grid in (('Walls', 1, walls), ('Surface', 0, surface), ('Markings', 0, markings)):
        lines.append(f'"{title}" 1 {collision} 0')
        lines.extend((' '.join(map(str, row)) + ' ' for row in grid))
    lines.append('0')
    lines.extend((' '.join(['0'] * size) for _ in range(3 * size)))
    return '\n'.join(lines) + '\n'
if __name__ == '__main__':
    import argparse
    from pathlib import Path
    parser = argparse.ArgumentParser(description='Reproduce the authored Microbot difficulty maps.')
    parser.add_argument('--write', action='store_true', help='Replace the two generated map assets; otherwise verify them.')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    for name, n, seed in LEVELS:
        graph = generate(n, seed)
        content = tilemap(graph, n)
        path = root / 'Assets' / 'Tilemaps' / f'Microbot{name}.pftilemap'
        if args.write:
            path.write_text(content)
        else:
            assert path.read_text() == content, f'{name} differs from its reproducible layout'
        print(name, 'cells per side:', n, 'seed:', seed, '(solution edges, turns, dead ends, discovery edges):', metrics(graph, n))
