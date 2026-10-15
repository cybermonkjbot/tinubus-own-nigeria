"""Additive road art recipe. Pure metre-space mesh data; no engine or file I/O."""

import hashlib
import json
import math


SURFACE_CHANNELS = ("compacted_earth", "loose_sand", "damp_soil", "gravel", "asphalt")
VARIANTS = ("compacted_earth", "sandy_lane", "rutted_earth", "paved_to_earth")


def _number(value, name, minimum, maximum):
    if type(value) not in (int, float) or not math.isfinite(value):
        raise ValueError(name + " must be finite")
    if not minimum <= value <= maximum:
        raise ValueError(name + " outside bounded authoring range")
    return float(value)


def _smooth(value):
    value = min(1.0, max(0.0, value))
    return value * value * (3.0 - 2.0 * value)


def _gaussian(value, centre, spread):
    return math.exp(-((value - centre) / spread) ** 2)


def make_road_module(variant, *, seed=0, length_m=10.0, lane_width_m=5.0,
                     shoulder_m=0.6, crown_m=0.035, max_relief_m=0.09):
    """Return one closed mesh, fixed end profiles, UVs and ordered blend weights.

    Local X crosses the road, +Y follows it, +Z is up. End profiles align for
    equal dimensions/crown. The caller owns placement, material assets, imports,
    collision and preview. Weights are authoring data, not a native shader.
    """
    if variant not in VARIANTS:
        raise ValueError("Unknown unpaved art variant")
    if type(seed) is not int or not 0 <= seed <= 65535:
        raise ValueError("seed must be a bounded nonnegative integer")
    length = _number(length_m, "length_m", 2.0, 20.0)
    lane = _number(lane_width_m, "lane_width_m", 3.0, 8.0)
    shoulder = _number(shoulder_m, "shoulder_m", 0.0, 1.5)
    crown = _number(crown_m, "crown_m", 0.0, 0.08)
    relief = _number(max_relief_m, "max_relief_m", 0.0, 0.16)
    half_lane, half_width = lane / 2.0, lane / 2.0 + shoulder
    columns = math.ceil(2.0 * half_width / 0.25)
    rows = math.ceil(length / 0.5)
    phase = seed * 0.61803398875
    wet = variant == "rutted_earth"
    transition = variant == "paved_to_earth"
    base = []
    weights = []
    wet_masks = []
    uv = []

    for j in range(rows + 1):
        y = length * j / rows
        longitudinal = math.sin(math.pi * y / length) ** 2
        earth_mix = _smooth(y / length) if transition else 1.0
        for i in range(columns + 1):
            x = -half_width + 2.0 * half_width * i / columns
            edge = _smooth((abs(x) - half_lane + 0.20) / max(shoulder + 0.20, 0.20))
            cross = max(0.0, 1.0 - (x / half_lane) ** 2)
            z = crown * cross - 0.035 * edge
            # Relief fades at both joins. Track channels stay inside the lane.
            wander = 0.10 * math.sin(2.0 * math.pi * y / length + phase) * longitudinal
            tracks = _gaussian(x, -0.82 + wander, 0.22) + _gaussian(x, 0.82 + wander, 0.22)
            rut_depth = relief * (0.70 if wet else 0.34)
            corrugation = 0.012 * math.sin(y * 2.2 + phase) * (1.0 - edge)
            offset = corrugation - rut_depth * tracks
            if shoulder:
                offset += edge * 0.018 * math.sin(y * 1.7 + x + phase)
            basin = 0.0
            if wet:
                basin = _gaussian(x, -1.20, 0.63) * _gaussian(y, length * 0.57, length * 0.13)
                offset -= relief * 0.60 * basin
            z += earth_mix * longitudinal * max(-relief, min(relief, offset))
            damp = min(0.85, basin * longitudinal * 0.78 + tracks * longitudinal * (0.20 if wet else 0.0))
            sand = min(0.65, edge * 0.50 + (0.23 if variant == "sandy_lane" else 0.0))
            gravel = 0.09 * edge
            earth = max(0.0, 1.0 - damp - sand - gravel)
            total = earth + sand + damp + gravel
            soil_weights = [v / total * earth_mix for v in (earth, sand, damp, gravel)]
            base.append([x, y, z])
            weights.append(soil_weights + [1.0 - earth_mix])
            wet_masks.append(min(1.0, basin * longitudinal))
            uv.append([x, y])

    stride = columns + 1
    faces = []
    for j in range(rows):
        for i in range(columns):
            a = j * stride + i
            b, c, d = a + 1, a + stride + 1, a + stride
            faces.extend([[a, b, c], [a, c, d]])
    top_face_count = len(faces)
    top_vertex_count = len(base)
    # Ordered perimeter joins a flat underside, keeping the asset a closed solid.
    perimeter = list(range(stride))
    perimeter += [j * stride + columns for j in range(1, rows + 1)]
    perimeter += [rows * stride + i for i in range(columns - 1, -1, -1)]
    perimeter += [j * stride for j in range(rows - 1, 0, -1)]
    floor_z = min(point[2] for point in base) - 0.12
    underside = []
    for index in perimeter:
        underside.append(len(base))
        x, y, _ = base[index]
        base.append([x, y, floor_z])
        uv.append([x, y])
        weights.append(weights[index].copy())
        wet_masks.append(0.0)
    for i, top in enumerate(perimeter):
        next_i = (i + 1) % len(perimeter)
        next_top = perimeter[next_i]
        low, next_low = underside[i], underside[next_i]
        faces.extend([[top, low, next_low], [top, next_low, next_top]])
    centre = len(base)
    base.append([0.0, length / 2.0, floor_z])
    uv.append([0.0, length / 2.0])
    weights.append([1.0, 0.0, 0.0, 0.0, 0.0])
    wet_masks.append(0.0)
    for i, low in enumerate(underside):
        faces.append([centre, underside[(i + 1) % len(underside)], low])

    parameters = {"variant": variant, "seed": seed, "length_m": length,
                  "lane_width_m": lane, "shoulder_m": shoulder,
                  "crown_m": crown, "max_relief_m": relief}
    identity = hashlib.sha256(json.dumps(parameters, sort_keys=True,
                                        separators=(",", ":")).encode()).hexdigest()
    return {
        "id": "unpaved-art-v1:" + variant + ":" + identity,
        "variant": variant,
        "authoring_parameters": parameters,
        "units": "metres",
        "axes": {"up": "+Z", "run": "+Y", "across": "+X"},
        "canonical_assignment": None,
        "vertices": base,
        "triangles": faces,
        "top_vertex_count": top_vertex_count,
        "top_triangle_count": top_face_count,
        "uv_metres": uv,
        "surface_channels": list(SURFACE_CHANNELS),
        "surface_weights": weights,
        "candidate_wet_mask": wet_masks,
        "texture_coverage_m": None,
        "join_profile": {"length_m": length, "lane_width_m": lane,
                         "shoulder_m": shoulder, "crown_m": crown,
                         "end_relief_m": 0.0},
        "material_state": "requires-real-earth-sand-gravel-inputs-or-explicit-authored-materials",
        "puddles": "candidate-mask-only; clip-water-to-inspected-depression-before-authoring",
        "collision_state": "not-generated",
        "gameplay_effects": False,
        "native_import_state": "not-executed",
    }
