"""Generate ЛЁША monogram variants in the style of the flipped Amnezia logo.

Glyphs are built from thick strokes + tapered spikes (shapely), the outline is
roughened with smooth noise, and the fill is a set of flat "camo" patches in the
logo palette. Output is plain-path SVG (no filters) so it works in Qt/Android.
"""
import math, random, sys, os
import numpy as np
from shapely.geometry import LineString, Point, Polygon, MultiPolygon, box
from shapely.ops import unary_union
from shapely import voronoi_polygons, MultiPoint

OUT = sys.argv[1] if len(sys.argv) > 1 else 'out'

# ---------- primitives ----------

def quad(p0, p1, p2, n=24):
    return [((1-t)**2*p0[0]+2*(1-t)*t*p1[0]+t*t*p2[0],
             (1-t)**2*p0[1]+2*(1-t)*t*p1[1]+t*t*p2[1]) for t in np.linspace(0, 1, n)]

def stroke(pts, w, cap='flat', join=2):
    cs = {'flat': 2, 'round': 1, 'square': 3}[cap]
    return LineString(pts).buffer(w/2, cap_style=cs, join_style=join, mitre_limit=3)

def spike(base, direction, length, bw, bend=0.0):
    """Tapered horn/drip from `base` toward `direction` (deg, 0=right, 90=down)."""
    a = math.radians(direction)
    d = np.array([math.cos(a), math.sin(a)]); n = np.array([-d[1], d[0]])
    b = np.array(base, float)
    left, right = [], []
    for t in np.linspace(0, 1, 16):
        c = b + d*length*t + n*bend*length*t*t
        hw = bw/2*(1-t)**1.4 + 0.15
        left.append(c + n*hw); right.append(c - n*hw)
    return Polygon(left + right[::-1]).buffer(0)

def blob(c, r):
    return Point(c).buffer(r, 24)

# ---------- roughening ----------

class Noise:
    def __init__(self, seed, amp, freqs=(0.035, 0.08, 0.17, 0.4)):
        r = random.Random(seed)
        self.terms = [(amp*w, f, r.uniform(0, 2*math.pi))
                      for f, w in zip(freqs, (0.6, 0.35, 0.15, 0.05))]
    def __call__(self, s):
        return sum(a*np.sin(2*math.pi*f*s+ph) for a, f, ph in self.terms)

def roughen_ring(coords, noise, step=0.35):
    c = np.array(coords)
    seg = np.diff(c, axis=0); L = np.hypot(*seg.T)
    s = np.concatenate([[0], np.cumsum(L)]); total = s[-1]
    ss = np.arange(0, total, step)
    x = np.interp(ss, s, c[:, 0]); y = np.interp(ss, s, c[:, 1])
    p = np.stack([x, y], 1)
    tang = np.roll(p, -1, 0) - np.roll(p, 1, 0)
    tang /= np.maximum(np.hypot(*tang.T), 1e-9)[:, None]
    nrm = np.stack([tang[:, 1], -tang[:, 0]], 1)
    return p + nrm*noise(ss)[:, None]

def roughen(geom, seed, amp=0.9):
    r = random.Random(seed)
    polys = geom.geoms if isinstance(geom, MultiPolygon) else [geom]
    out = []
    for pg in polys:
        ext = roughen_ring(pg.exterior.coords, Noise(r.random(), amp))
        holes = [roughen_ring(h.coords, Noise(r.random(), amp*0.8)) for h in pg.interiors]
        out.append(Polygon(ext, holes).buffer(0))
    g = unary_union(out)
    # soften the corners a bit, like the original's blobby outline
    g = g.buffer(0.7, 16).buffer(-1.2, 16).buffer(0.5, 16)
    # drop specks: tiny holes and islands produced by the noise
    polys = g.geoms if isinstance(g, MultiPolygon) else [g]
    keep = [Polygon(p.exterior, [h for h in p.interiors if Polygon(h).area > 20]) for p in polys if p.area > 6]
    return unary_union(keep)

# ---------- camo fill ----------

PALETTE = [  # (u, v) in glyph bbox, colour — sampled from the current icon
    ((0.10, 0.10), '#F59B3D'), ((0.30, 0.05), '#F7A64A'), ((0.05, 0.45), '#EF8A2D'),
    ((0.22, 0.35), '#F9B56A'), ((0.35, 0.50), '#F9C88A'), ((0.42, 0.66), '#F8E6C6'),
    ((0.15, 0.80), '#EBD3DA'), ((0.30, 0.95), '#E9C9D3'), ((0.55, 0.25), '#ABA6AC'),
    ((0.62, 0.05), '#FBD9A0'), ((0.75, 0.20), '#8C84BF'), ((0.92, 0.35), '#7563B0'),
    ((0.80, 0.50), '#7A6BB2'), ((0.58, 0.52), '#9E99A8'), ((0.50, 0.80), '#82B3C9'),
    ((0.65, 0.70), '#85A9CF'), ((0.88, 0.70), '#75C2B4'), ((0.95, 0.90), '#67B3A6'),
    ((0.50, 1.00), '#6A8DBF'), ((0.70, 0.95), '#79B7C4'),
]

def camo(glyph, seed):
    r = random.Random(seed)
    x0, y0, x1, y1 = glyph.bounds
    W, H = x1-x0, y1-y0
    pts, cols = [], []
    for (u, v), col in PALETTE:
        for _ in range(1):
            pts.append((x0+W*min(max(u+r.uniform(-.07, .07), 0), 1),
                        y0+H*min(max(v+r.uniform(-.07, .07), 0), 1)))
            cols.append(col)
    env = box(x0-20, y0-20, x1+20, y1+20)
    cells = voronoi_polygons(MultiPoint(pts), extend_to=env)
    patches = []
    for cell in cells.geoms:
        idx = min(range(len(pts)), key=lambda i: Point(pts[i]).distance(cell.representative_point())
                  if cell.contains(Point(pts[i])) else 1e9)
        g = roughen(cell.buffer(1.2), r.random(), amp=1.6).intersection(glyph)
        if not g.is_empty:
            patches.append((cols[idx], g))
    return patches

# ---------- svg ----------

def path_d(geom, T):
    polys = geom.geoms if hasattr(geom, 'geoms') else [geom]
    parts = []
    for pg in polys:
        if pg.is_empty or pg.geom_type != 'Polygon':
            continue
        for ring in [pg.exterior, *pg.interiors]:
            c = [T(x, y) for x, y in ring.coords]
            parts.append('M' + ' '.join(f'{x:.2f},{y:.2f}' for x, y in c) + 'Z')
    return ''.join(parts)

def fitter(glyph, size, margin):
    x0, y0, x1, y1 = glyph.bounds
    s = (size-2*margin)/max(x1-x0, y1-y0)
    ox = (size-(x1-x0)*s)/2 - x0*s; oy = (size-(y1-y0)*s)/2 - y0*s
    return lambda x, y: (x*s+ox, y*s+oy)

BG_DEFS = '''<linearGradient id="bg" x1="0" y1="0" x2="0" y2="1">
<stop offset="0" stop-color="#3D3D3D"/><stop offset="1" stop-color="#171717"/></linearGradient>'''

def squircle(size, inset):
    # superellipse n=5, macOS-ish
    n = 5; a = size/2-inset; c = size/2; pts = []
    for t in np.linspace(0, 2*math.pi, 200, endpoint=False):
        ct, st = math.cos(t), math.sin(t)
        pts.append((c+a*np.sign(ct)*abs(ct)**(2/n), c+a*np.sign(st)*abs(st)**(2/n)))
    return 'M' + ' '.join(f'{x:.2f},{y:.2f}' for x, y in pts) + 'Z'

def write_svgs(name, glyph, patches):
    S = 1024
    def glyph_layers(T, mono=None):
        if mono:
            return f'<path fill="{mono}" fill-rule="evenodd" d="{path_d(glyph, T)}"/>'
        out = [f'<path fill="#A9A4AA" fill-rule="evenodd" d="{path_d(glyph, T)}"/>']
        out += [f'<path fill="{c}" fill-rule="evenodd" d="{path_d(g, T)}"/>' for c, g in patches]
        return '\n'.join(out)
    hdr = f'<svg xmlns="http://www.w3.org/2000/svg" width="{S}" height="{S}" viewBox="0 0 {S} {S}">'
    # glyph alone
    T = fitter(glyph, S, 40)
    open(f'{OUT}/{name}-glyph.svg', 'w').write(f'{hdr}\n{glyph_layers(T)}\n</svg>\n')
    open(f'{OUT}/{name}-mono.svg', 'w').write(f'{hdr}\n{glyph_layers(T, "#FFFFFF")}\n</svg>\n')
    # macOS-style squircle
    T = fitter(glyph, S, 205)
    open(f'{OUT}/{name}-icon.svg', 'w').write(
        f'{hdr}<defs>{BG_DEFS}</defs>\n<path fill="url(#bg)" d="{squircle(S, 100)}"/>\n{glyph_layers(T)}\n</svg>\n')
    # round (Windows/Linux/Android round)
    T = fitter(glyph, S, 180)
    open(f'{OUT}/{name}-round.svg', 'w').write(
        f'{hdr}<defs>{BG_DEFS}</defs>\n<circle cx="512" cy="512" r="508" fill="url(#bg)"/>\n{glyph_layers(T)}\n</svg>\n')

# ---------- variants ----------
W = 11.5  # stroke width in the 100-grid

def v1_row():
    """Л-Ё-Ш-А in a row: Л's roof is Ё's top arm, Ё's stem is Ш's left stem, Ш's right stem leans into А."""
    parts = [
        stroke(quad((8, 88), (24, 66), (27, 22)) + [(56, 22)], W),  # Л leg + roof = Ё top arm
        stroke([(43, 20), (43, 86)], W),                            # Ё stem = Ш left stem
        stroke([(43, 52), (55, 52)], W),                            # Ё middle arm
        stroke([(66, 46), (66, 86)], W*0.95),                       # Ш middle stem
        stroke([(37, 85), (82, 85)], W),                            # Ш bar
        stroke([(79, 88), (90, 11), (110, 88)], W*0.95, join=3),            # Ш right stem / А
        stroke([(83, 68), (103, 68)], W*0.8),                       # А crossbar
        blob((49, 8), 4.5), blob((59, 8), 4.5),                     # Ё dots
        spike((8, 87), 160, 17, 11, bend=0.3),                      # Л foot flick
        spike((90, 15), -88, 20, 12, bend=0.12),                     # А apex horn
        spike((66, 45), -92, 9, 8),                                 # Ш middle horn
        spike((66, 89), 92, 21, 9, bend=0.15),                      # drip
        spike((110, 87), 70, 13, 9),
    ]
    return unary_union(parts)

def v2_ring():
    """Anarchy-style ring like the current logo: Λ (Л/А) breaks out, ring bottom is Ш's bar, Ё's dots are horns."""
    c, R = (50, 55), 33
    ring = Point(c).buffer(R+W/2, 64).difference(Point(c).buffer(R-W/2, 64))
    parts = [
        ring,
        stroke([(50, 9), (17, 96)], W),                             # Л / А left leg
        stroke([(50, 9), (83, 96)], W),                             # А right leg
        stroke([(26, 60), (74, 60)], W*0.9),                        # А crossbar = Ё middle arm
        stroke([(50, 60), (50, 88)], W*0.9),                        # Ш middle stem
        spike((50, 10), -90, 17, 9),                                 # apex
        spike((30, 29), -125, 27, 11, bend=-0.25),                   # Ё dot horns
        spike((70, 29), -55, 27, 11, bend=0.25),
        spike((17, 95), 115, 15, 11), spike((83, 95), 65, 15, 11),      # leg drips
        spike((50, 89), 90, 19, 9, bend=0.1),
    ]
    return unary_union(parts)

def v3_crown():
    """ΛΛ over a bar: left Λ = Л, right Λ (with crossbar) = А, bar + three feet = Ш, dots = Ё."""
    parts = [
        stroke([(13, 85), (32, 17)], W), stroke([(32, 17), (50, 85)], W),
        stroke([(50, 85), (68, 17)], W), stroke([(68, 17), (87, 85)], W),
        stroke([(10, 85), (90, 85)], W),                            # Ш bar
        stroke([(56, 60), (80, 60)], W*0.85),                       # А crossbar
        blob((44, 11), 4.6), blob((56, 11), 4.6),                   # Ё dots
        spike((32, 16), -100, 21, 9, bend=-0.2),                    # horns
        spike((68, 16), -80, 21, 9, bend=0.2),
        spike((11, 86), 170, 15, 11, bend=-0.35),                     # Л hook
        spike((50, 89), 90, 23, 11, bend=0.1),                       # drip
        spike((90, 86), 15, 11, 9),
    ]
    return unary_union(parts)

def v4_house():
    """А with a curved Л leg, Ё/Ш stem from the apex, crossbar and bar shared; Ё dots as side horns."""
    parts = [
        stroke(quad((50, 12), (30, 40), (15, 88)), W),              # Л curved leg
        stroke([(50, 12), (85, 88)], W),                            # А right leg
        stroke([(50, 12), (50, 88)], W*0.9),                        # Ё stem / Ш middle stem
        stroke([(26, 58), (70, 58)], W*0.85),                       # crossbar / Ё middle arm
        stroke([(13, 86), (88, 86)], W),                            # Ш bar / Ё bottom arm
        stroke([(50, 34), (62, 34)], W*0.8),                        # Ё top arm
        spike((42, 15), -150, 25, 9, bend=-0.2),                    # dots as horns
        spike((58, 15), -30, 25, 9, bend=0.2),
        spike((50, 11), -90, 15, 8),
        spike((14, 87), 155, 17, 11, bend=0.3),
        spike((86, 87), 40, 13, 9),
        spike((36, 89), 92, 19, 8), spike((64, 89), 88, 13, 7),
    ]
    return unary_union(parts)

def build(name):
    i = list(VARIANTS).index(name)
    g = roughen(VARIANTS[name](), seed=100+i, amp=0.75)
    return g, camo(g, seed=200+i)

VARIANTS = {'v1-row': v1_row, 'v2-ring': v2_ring, 'v3-crown': v3_crown, 'v4-house': v4_house}
if __name__ == '__main__':
  os.makedirs(OUT, exist_ok=True)
  for i, (name, fn) in enumerate(VARIANTS.items()):
    g = roughen(fn(), seed=100+i, amp=0.75)
    write_svgs(name, g, camo(g, seed=200+i))
    print(name, 'ok')
