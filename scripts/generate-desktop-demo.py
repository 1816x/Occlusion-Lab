#!/usr/bin/env python3
"""Original schematic arches: 14 disjoint closed ellipsoids per arch, meters.
Not anatomical/clinical data. Lower mesh compensates the engine's +0.16 m pose offset.
"""
from pathlib import Path
import math

out = Path(__file__).resolve().parents[1] / 'native/apps/occlusion-desktop/resources'
for name, center_y in [('upper-arch.obj', .004), ('lower-arch.obj', -.164)]:
    vertices, faces = [], []
    for tooth in range(14):
        angle = -1.35 + tooth * 2.7 / 13
        cx, cz = .027 * math.sin(angle), .034 * math.cos(angle)
        rings, segments = 6, 12
        start = len(vertices)
        vertices.append((cx, center_y + .004, cz))
        for ring in range(1, rings):
            phi = math.pi * ring / rings
            for j in range(segments):
                theta = 2 * math.pi * j / segments
                # Rounded crowns, separated so each component remains a closed shell.
                vertices.append((cx + .0024 * math.sin(phi) * math.cos(theta),
                                 center_y + .004 * math.cos(phi),
                                 cz + .0024 * math.sin(phi) * math.sin(theta)))
        bottom = len(vertices)
        vertices.append((cx, center_y - .004, cz))
        for j in range(segments):
            faces.append((start, start + 1 + (j + 1) % segments, start + 1 + j))
        for ring in range(rings - 2):
            for j in range(segments):
                a = start + 1 + ring * segments + j
                b = start + 1 + ring * segments + (j + 1) % segments
                faces.extend([(a, b, b + segments), (a, b + segments, a + segments)])
        for j in range(segments):
            a = start + 1 + (rings - 2) * segments + j
            b = start + 1 + (rings - 2) * segments + (j + 1) % segments
            faces.append((a, b, bottom))
    text = '# Original synthetic teaching geometry. Meters. Not anatomical data.\n'
    text += ''.join('v %.9f %.9f %.9f\n' % p for p in vertices)
    text += ''.join('f %d %d %d\n' % tuple(i + 1 for i in f) for f in faces)
    (out / name).write_text(text)
