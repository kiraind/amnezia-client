"""Replace the logo in every fork asset (FORK.md §6) with the ЛЁША monogram (v1-row).

Phase 1 (this script, `prep`): write glyph SVGs + a render manifest.
Phase 2 (`node render_jobs.js jobs.json`): Chromium rasterises them.
Phase 3 (this script, `compose`): composite onto backgrounds and write the repo files.
"""
import json, os, sys, re, io, subprocess
import numpy as np
from PIL import Image
sys.path.insert(0, os.path.dirname(__file__))
import gen

REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
C = f'{REPO}/client'
WORK = os.environ.get('LOGO_WORK', '/tmp/lesha-logo-work')
os.makedirs(WORK, exist_ok=True)

glyph, patches = gen.build('v1-row')
gx0, gy0, gx1, gy1 = glyph.bounds
GW, GH = gx1 - gx0, gy1 - gy0
ident = lambda x, y: (x, y)

def glyph_svg(w, h, mono=None):
    vb = f'{gx0:.2f} {gy0:.2f} {GW:.2f} {GH:.2f}'
    body = [f'<path fill="{mono or "#A9A4AA"}" fill-rule="evenodd" d="{gen.path_d(glyph, ident)}"/>']
    if not mono:
        body += [f'<path fill="{c}" fill-rule="evenodd" d="{gen.path_d(g, ident)}"/>' for c, g in patches]
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="{vb}" '
            f'preserveAspectRatio="xMidYMid meet">' + ''.join(body) + '</svg>')

def fit(box_w, box_h):
    s = min(box_w / GW, box_h / GH)
    return max(1, round(GW * s)), max(1, round(GH * s))

# ---------- asset table ----------
# kind: 'shape'  - background is the original's alpha shape, refilled with a vertical gradient
#       'clear'  - transparent background
#       'keep'   - keep the original pixels outside `clear_box`, fill clear_box with bg colour
JOBS = []
def add(path, frac, kind='shape', mono=None, top=None, bot=None, center=None, box=None, clear_box=None, bg=None):
    JOBS.append(dict(path=path, frac=frac, kind=kind, mono=mono, top=top, bot=bot, center=center,
                     box=box, clear_box=clear_box, bg=bg))

DARK_T, DARK_B = (60, 60, 60), (23, 23, 23)
add(f'{C}/images/icon.png', 0.66, top=(58, 63, 59), bot=(33, 34, 33))
for d in ['hdpi', 'ldpi', 'mdpi', 'xhdpi', 'xxhdpi', 'xxxhdpi']:
    p = f'{C}/android/res/mipmap-{d}'
    add(f'{p}/icon.png', 0.70, top=DARK_T, bot=DARK_B)
    add(f'{p}/icon_round.png', 0.64, top=(25, 29, 34), bot=(21, 19, 20))
    if os.path.exists(f'{p}/ic_launcher_foreground.png'):
        add(f'{p}/ic_launcher_foreground.png', 0.50, kind='clear')
    if os.path.exists(f'{p}/ic_banner.png'):
        k = Image.open(f'{p}/ic_banner.png').width / 320
        add(f'{p}/ic_banner.png', None, kind='keep', bg=(30, 30, 31),
            box=(24*k, 43*k, 118*k, 137*k), clear_box=(0, 0, round(125*k), None))
for f in os.listdir(f'{C}/ios/app/Media.xcassets/AppIcon.appiconset'):
    if f.endswith('.png'):
        add(f'{C}/ios/app/Media.xcassets/AppIcon.appiconset/{f}', 0.68, top=(23, 23, 25), bot=(23, 23, 25))
add(f'{C}/ios/app/launch.png', 0.55, top=(41, 41, 44), bot=(30, 30, 31))
for f in os.listdir(f'{C}/macos/app/Images.xcassets/AppIcon.appiconset'):
    if f.endswith('.png'):
        add(f'{C}/macos/app/Images.xcassets/AppIcon.appiconset/{f}', 0.58, top=(58, 58, 58), bot=(22, 22, 22))
add(f'{REPO}/deploy/data/linux/AmneziaVPN.png', 0.64, top=(30, 30, 30), bot=(30, 30, 30))
add(f'{REPO}/metadata/en-US/images/icon.png', 0.68, top=(41, 41, 44), bot=(31, 31, 32))
add(f'{C}/images/tray/active.png', 0.96, kind='clear', mono='#FFFFFF')
add(f'{C}/images/tray/default.png', 0.96, kind='clear', mono='#808080')
add(f'{C}/images/tray/error.png', 0.96, kind='clear', mono='#FF0000')
add(f'{C}/images/amneziaBigLogo.png', None, kind='keep', bg=(0, 0, 0, 0),
    box=(234, 246, 1169, 770), clear_box=(0, 0, None, 800))

def shape_center(alpha):
    ys, xs = np.nonzero(alpha > 128)
    return (xs.min() + xs.max()) / 2, (ys.min() + ys.max()) / 2, xs.max() - xs.min() + 1, ys.max() - ys.min() + 1

def plan(job):
    im = Image.open(job['path']).convert('RGBA')
    W, H = im.size
    if job['box']:
        x0, y0, x1, y1 = job['box']
        gw, gh = fit(x1 - x0, y1 - y0)
        cx, cy = (x0 + x1) / 2, y1 - gh / 2  # bottom-aligned
    else:
        a = np.array(im)[..., 3]
        if job['kind'] == 'shape' and (a > 128).any():
            cx, cy, sw, sh = shape_center(a)
        else:
            cx, cy, sw, sh = W / 2, H / 2, W, H
        gw, gh = fit(sw * job['frac'], sh * job['frac'])
    job.update(W=W, H=H, gw=gw, gh=gh, gxo=round(cx - gw / 2), gyo=round(cy - gh / 2))
    return job

def prep():
    render = []
    for i, j in enumerate(JOBS):
        plan(j)
        svg = os.path.join(WORK, f'g{i}.svg')
        open(svg, 'w').write(glyph_svg(j['gw'], j['gh'], j['mono']))
        j['png'] = os.path.join(WORK, f'g{i}.png')
        render.append(dict(svg=svg, png=j['png'], w=j['gw'], h=j['gh']))
    json.dump(JOBS, open(os.path.join(WORK, 'jobs.json'), 'w'))
    json.dump(render, open(os.path.join(WORK, 'render.json'), 'w'))

def gradient(W, H, top, bot):
    t = np.linspace(0, 1, H)[:, None, None]
    g = np.array(top, float)[None, None, :3] * (1 - t) + np.array(bot, float)[None, None, :3] * t
    return np.repeat(g, W, 1)

def compose():
    for j in json.load(open(os.path.join(WORK, 'jobs.json'))):
        orig = Image.open(j['path']).convert('RGBA')
        W, H = orig.size
        if j['kind'] == 'shape':
            o = np.array(orig).astype(float)
            inside = o[..., 3] >= 250
            ys = np.nonzero(inside.any(1))[0]
            rgb = np.zeros((H, W, 3))
            rgb[ys.min():ys.max()+1] = gradient(W, ys.max()-ys.min()+1, j['top'], j['bot'])
            # edge/shadow pixels keep the original; the solid interior (incl. the old glyph) is refilled
            o[inside, :3] = rgb[inside]
            base = Image.fromarray(o.astype(np.uint8), 'RGBA')
        elif j['kind'] == 'clear':
            base = Image.new('RGBA', (W, H), (0, 0, 0, 0))
        else:
            base = orig.copy()
            x0, y0, x1, y1 = j['clear_box']
            bg = tuple(j['bg']) + ((255,) if len(j['bg']) == 3 else ())
            base.paste(Image.new('RGBA', ((x1 or W) - x0, (y1 or H) - y0), bg), (x0, y0))
        g = Image.open(j['png']).convert('RGBA')
        base.alpha_composite(g, (j['gxo'], j['gyo']))
        save(base, j['path'])

def save(img, path):
    src = Image.open(path)
    if src.mode == 'P':  # amneziaBigLogo is palette-based upstream; RGBA is fine for Qt
        pass
    img.save(path, optimize=True)

if __name__ == '__main__':
    {'prep': prep, 'compose': compose}[sys.argv[1]]()
