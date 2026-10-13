import re, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen
from PIL import Image
C = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'client'))
glyph, patches = gen.build('v1-row')
x0, y0, x1, y1 = glyph.bounds

def placer(bx0, by0, bx1, by1):
    s = min((bx1-bx0)/(x1-x0), (by1-by0)/(y1-y0))
    ox = (bx0+bx1)/2 - (x0+x1)/2*s; oy = (by0+by1)/2 - (y0+y1)/2*s
    return lambda x, y: (x*s+ox, y*s+oy)

def d(geom, T, tol=0.12, nd=2):
    g = geom.simplify(tol)
    polys = g.geoms if hasattr(g, 'geoms') else [g]
    out = []
    for p in polys:
        if p.geom_type != 'Polygon' or p.is_empty: continue
        for ring in [p.exterior, *p.interiors]:
            pts = [T(x, y) for x, y in ring.coords[:-1]]
            out.append('M' + 'L'.join(f'{x:.{nd}f},{y:.{nd}f}' for x, y in pts) + 'Z')
    return ''.join(out)

def colored(T, tol):
    parts = [f'<path fill="#A9A4AA" fill-rule="evenodd" d="{d(glyph, T, tol)}"/>']
    parts += [f'<path fill="{c}" fill-rule="evenodd" d="{d(g, T, tol)}"/>' for c, g in patches]
    return '\n'.join(parts)

# controls/amnezia.svg — 23x22 mono
T = placer(0.5, 0, 22.5, 22)
open(f'{C}/images/controls/amnezia.svg', 'w').write(
    f'<svg width="23" height="22" viewBox="0 0 23 22" fill="none" xmlns="http://www.w3.org/2000/svg">\n'
    f'<path fill-rule="evenodd" clip-rule="evenodd" fill="#CCCAC8" d="{d(glyph, T, 0.25, 3)}"/>\n</svg>\n')

# android vector (notification / monochrome launcher icon), 256 viewport
T = placer(28, 28, 228, 228)
open(f'{C}/android/res/drawable/ic_amnezia_round.xml', 'w').write(f'''<vector xmlns:android="http://schemas.android.com/apk/res/android"
    android:width="256dp"
    android:height="256dp"
    android:viewportWidth="256"
    android:viewportHeight="256">
    <path
        android:fillType="evenOdd"
        android:pathData="{d(glyph, T, 0.2, 1)}"
        android:fillColor="#000000"
        android:strokeColor="#00000000"/>
</vector>
''')

# full logo svg: new glyph in the old glyph box, keep the "Лёша VPN" text paths and their gradients
p = f'{C}/images/AmneziaVPN_Full_logo.svg'
s = open(p).read()
head = s[:s.index('</defs>')+len('</defs>')]
texts = re.findall(r'<path d="M196\.67 521\.36.*?/>|<path d="M270\.16 640.*?/>', s, re.S)
assert len(texts) == 2
T = placer(205, 7, 576, 373)
open(p, 'w').write(head + '\n' + colored(T, 0.08) + '\n' + '\n'.join(texts) + '\n</svg>\n')

# app.ico / app.icns
ico = Image.open(f'{C}/images/icon.png').convert('RGBA')
ico.save(f'{C}/images/app.ico', sizes=[(256, 256), (128, 128), (64, 64), (48, 48), (32, 32), (16, 16)])
Image.open(f'{C}/macos/app/Images.xcassets/AppIcon.appiconset/512@2x.png').convert('RGBA').resize((512, 512), Image.LANCZOS).save(f'{C}/images/app.icns')
