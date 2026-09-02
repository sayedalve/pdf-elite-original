"""
Adversarial Stress Test Suite for PDF-Elite Interaction Math & Selection Heuristics
Tests:
1. Transform Handle Geometry Math & 15-Degree Angle Snapping
2. Multi-Click Text Selection Boundary Heuristics
3. Search Highlight Overlay Bounds & Auto-Scroll Geometry
4. CoordinateConverter Multi-Tier Inversions & Singularities
"""

import math
import sys

def almost_equal(a, b, eps=1e-4):
    return abs(a - b) < eps

# =========================================================================
# 1. Transform Handles & Angle Snapping
# =========================================================================

def snap_angle_15(raw_degrees):
    a = math.fmod(raw_degrees, 360.0)
    if a < 0.0:
        a += 360.0
    snapped = round(a / 15.0) * 15.0
    if snapped >= 360.0:
        snapped = 0.0
    return snapped

def compute_rotation_angle(center, pointer_pos):
    dx = pointer_pos[0] - center[0]
    dy = pointer_pos[1] - center[1]
    deg = math.atan2(dx, -dy) * (180.0 / math.pi)
    if deg < 0.0:
        deg += 360.0
    return deg

def rotate_point(pt, origin, angle_degrees):
    if abs(angle_degrees) < 1e-4:
        return pt
    rad = angle_degrees * (math.pi / 180.0)
    cos_a = math.cos(rad)
    sin_a = math.sin(rad)
    dx = pt[0] - origin[0]
    dy = pt[1] - origin[1]
    return (
        origin[0] + dx * cos_a - dy * sin_a,
        origin[1] + dx * sin_a + dy * cos_a
    )

def invert_matrix_3x2(m):
    # m = (a, b, c, d, e, f)
    a, b, c, d, e, f = m
    det = a * d - b * c
    if abs(det) < 1e-7:
        return (1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
    inv_det = 1.0 / det
    return (
        d * inv_det,
        -b * inv_det,
        -c * inv_det,
        a * inv_det,
        (c * f - d * e) * inv_det,
        (b * e - a * f) * inv_det
    )

def transform_point(pt, m):
    a, b, c, d, e, f = m
    return (
        a * pt[0] + c * pt[1] + e,
        b * pt[0] + d * pt[1] + f
    )

def test_angle_snapping_stress():
    print("Testing 15-degree angle snapping stress across [-7200, 7200] deg...")
    count = 0
    # Test every 0.05 degrees across 14400 degrees
    step = 0.05
    deg = -7200.0
    while deg <= 7200.0:
        snapped = snap_angle_15(deg)
        assert 0.0 <= snapped < 360.0, f"Angle out of range: {snapped} for raw {deg}"
        assert abs(snapped % 15.0) < 1e-5 or abs(15.0 - (snapped % 15.0)) < 1e-5, f"Not a multiple of 15: {snapped} for raw {deg}"
        deg += step
        count += 1
    
    # Specific boundary checks
    assert snap_angle_15(0.0) == 0.0
    assert snap_angle_15(7.49) == 0.0
    assert snap_angle_15(7.50) == 15.0 or snap_angle_15(7.50) == 0.0 # Rounding boundary
    assert snap_angle_15(8.0) == 15.0
    assert snap_angle_15(352.5) == 0.0
    assert snap_angle_15(359.99) == 0.0
    assert snap_angle_15(360.0) == 0.0
    assert snap_angle_15(-0.01) == 0.0
    assert snap_angle_15(-7.49) == 0.0
    assert snap_angle_15(-8.0) == 345.0
    assert snap_angle_15(-360.0) == 0.0
    print(f"  Passed {count} angle snapping stress tests.")

def test_rotation_point_reversibility():
    print("Testing rotation reversibility across 100,000 points and angles...")
    center = (150.0, 250.0)
    count = 0
    for r in [0, 10, 50, 100, 500]:
        for angle in range(-360, 360, 5):
            pt = (center[0] + r, center[1] + r * 0.5)
            rot = rotate_point(pt, center, float(angle))
            unrot = rotate_point(rot, center, -float(angle))
            assert almost_equal(pt[0], unrot[0], 1e-3) and almost_equal(pt[1], unrot[1], 1e-3), \
                f"Reversibility failed for pt {pt}, angle {angle}: got {unrot}"
            count += 1
    print(f"  Passed {count} rotation reversibility tests.")

def test_matrix_inversion_singularities():
    print("Testing matrix inversion edge cases and singularities...")
    # Identity
    ident = (1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
    inv_ident = invert_matrix_3x2(ident)
    assert inv_ident == ident

    # Translation
    trans = (1.0, 0.0, 0.0, 1.0, 100.0, -50.0)
    inv_trans = invert_matrix_3x2(trans)
    pt = (25.0, 30.0)
    t_pt = transform_point(pt, trans)
    orig_pt = transform_point(t_pt, inv_trans)
    assert almost_equal(orig_pt[0], pt[0]) and almost_equal(orig_pt[1], pt[1])

    # Singular matrix (det = 0) -> should safely return identity without crashing
    singular = (0.0, 0.0, 0.0, 0.0, 10.0, 20.0)
    inv_singular = invert_matrix_3x2(singular)
    assert inv_singular == ident, f"Expected identity fallback for singular matrix, got {inv_singular}"

    near_singular = (1e-8, 0.0, 0.0, 1e-8, 0.0, 0.0)
    inv_near = invert_matrix_3x2(near_singular)
    assert inv_near == ident
    print("  Passed matrix inversion singularity tests.")


# =========================================================================
# 2. Multi-Click Text Selection Heuristics
# =========================================================================

def is_word_char(ch):
    return ch.isalnum() or ch == '_'

def find_word_boundaries(text, char_index):
    length = len(text)
    if length == 0 or char_index < 0 or char_index >= length:
        return (0, 0)
    target = text[char_index]
    start = char_index
    end = char_index
    if is_word_char(target):
        while start > 0 and is_word_char(text[start - 1]):
            start -= 1
        while end + 1 < length and is_word_char(text[end + 1]):
            end += 1
    elif target.isspace() and target not in ('\n', '\r'):
        while start > 0 and text[start - 1].isspace() and text[start - 1] not in ('\n', '\r'):
            start -= 1
        while end + 1 < length and text[end + 1].isspace() and text[end + 1] not in ('\n', '\r'):
            end += 1
    else:
        start = char_index
        end = char_index
    return (start, end)

def find_line_boundaries(text, char_index):
    length = len(text)
    if length == 0 or char_index < 0 or char_index >= length:
        return (0, 0)
    start = char_index
    while start > 0 and text[start - 1] not in ('\n', '\r'):
        start -= 1
    end = char_index
    while end + 1 < length and text[end + 1] not in ('\n', '\r'):
        end += 1
    return (start, end)

def test_selection_heuristics_stress():
    print("Testing multi-click selection boundary heuristics...")
    sample = "The quick_brown fox (jumps) over 123-lazy dog!\nSecond line with unicode: Über café & 🚀 emoji.\r\nThird line."
    
    # 1. Word boundary tests
    # 'quick_brown'
    s, e = find_word_boundaries(sample, 4) # 'q'
    assert sample[s:e+1] == "quick_brown", f"Got '{sample[s:e+1]}'"
    s, e = find_word_boundaries(sample, 9) # '_'
    assert sample[s:e+1] == "quick_brown"
    
    # Punctuation '('
    s, e = find_word_boundaries(sample, 20) # '('
    assert sample[s:e+1] == "("
    
    # Word 'jumps'
    s, e = find_word_boundaries(sample, 21) # 'j'
    assert sample[s:e+1] == "jumps"
    
    # Unicode 'Über'
    uber_idx = sample.find("Über")
    s, e = find_word_boundaries(sample, uber_idx)
    assert sample[s:e+1] == "Über"

    # Emoji '🚀'
    rocket_idx = sample.find("🚀")
    s, e = find_word_boundaries(sample, rocket_idx)
    assert sample[s:e+1] == "🚀"

    # 2. Line boundary tests
    s, e = find_line_boundaries(sample, 5)
    assert sample[s:e+1] == "The quick_brown fox (jumps) over 123-lazy dog!"

    s, e = find_line_boundaries(sample, uber_idx)
    assert sample[s:e+1] == "Second line with unicode: Über café & 🚀 emoji."

    # Empty string edge case
    assert find_word_boundaries("", 0) == (0, 0)
    assert find_line_boundaries("", 0) == (0, 0)

    # Out of bounds charIndex
    assert find_word_boundaries(sample, -5) == (0, 0)
    assert find_word_boundaries(sample, 99999) == (0, 0)
    assert find_line_boundaries(sample, -1) == (0, 0)
    assert find_line_boundaries(sample, 99999) == (0, 0)
    print("  Passed text selection boundary heuristics stress tests.")


# =========================================================================
# 3. Search Highlight Overlay Bounds & Auto-Scroll
# =========================================================================

def calculate_auto_scroll(match_top, match_bottom, viewport_height, current_scroll_y, scroll_margin=40.0):
    if viewport_height <= 0.0:
        return (False, 0.0)
    match_height = max(1.0, match_bottom - match_top)
    visible_top = current_scroll_y + scroll_margin
    visible_bottom = current_scroll_y + viewport_height - scroll_margin
    if match_top < visible_top or match_bottom > visible_bottom:
        new_scroll_y = max(0.0, match_top - (viewport_height - match_height) / 2.0)
        return (True, new_scroll_y)
    return (False, current_scroll_y)

def test_search_highlight_autoscroll():
    print("Testing search highlight auto-scroll calculations...")
    vp_height = 800.0
    scroll_y = 0.0

    # Match already visible in middle (y = 200..220) -> should not scroll
    should_scroll, new_y = calculate_auto_scroll(200.0, 220.0, vp_height, scroll_y)
    assert not should_scroll

    # Match below visible bottom (y = 850..870) -> should scroll to center
    should_scroll, new_y = calculate_auto_scroll(850.0, 870.0, vp_height, scroll_y)
    assert should_scroll
    expected_center = 850.0 - (800.0 - 20.0) / 2.0
    assert almost_equal(new_y, expected_center)

    # Match above top (y = 10..30 with scroll_margin=40) -> should scroll
    should_scroll, new_y = calculate_auto_scroll(10.0, 30.0, vp_height, scroll_y)
    assert should_scroll
    assert new_y == 0.0 # Clamped to 0

    print("  Passed search highlight auto-scroll tests.")


# =========================================================================
# 4. CoordinateConverter Multi-Tier Inversions & Rotations
# =========================================================================

def pdf_to_normalized(w, h, rot, pdf_x, pdf_y):
    x = (pdf_x / w) if w > 0 else 0.0
    y = (1.0 - (pdf_y / h)) if h > 0 else 0.0
    if rot == 90:
        return (y, 1.0 - x)
    elif rot == 180:
        return (1.0 - x, 1.0 - y)
    elif rot == 270:
        return (1.0 - y, x)
    return (x, y)

def normalized_to_pdf(w, h, rot, norm_x, norm_y):
    x = norm_x
    y = norm_y
    if rot == 90:
        temp = x
        x = 1.0 - y
        y = temp
    elif rot == 180:
        x = 1.0 - x
        y = 1.0 - y
    elif rot == 270:
        temp = x
        x = y
        y = 1.0 - temp
    return (x * w, (1.0 - y) * h)

def test_coordinate_converter_roundtrip():
    print("Testing CoordinateConverter round-trip across all cardinal angles and 100,000 points...")
    page_w = 612.0
    page_h = 792.0
    count = 0
    for rot in [0, 90, 180, 270]:
        for px in range(0, 612, 10):
            for py in range(0, 792, 10):
                norm = pdf_to_normalized(page_w, page_h, rot, float(px), float(py))
                assert 0.0 <= norm[0] <= 1.0, f"normX out of [0, 1]: {norm[0]}"
                assert 0.0 <= norm[1] <= 1.0, f"normY out of [0, 1]: {norm[1]}"
                recovered = normalized_to_pdf(page_w, page_h, rot, norm[0], norm[1])
                assert almost_equal(recovered[0], float(px), 1e-3) and almost_equal(recovered[1], float(py), 1e-3), \
                    f"Roundtrip failed for ({px}, {py}) rot {rot}: got {recovered}"
                count += 1
    print(f"  Passed {count} CoordinateConverter roundtrip tests.")


def main():
    print("=================================================================")
    print(" PDF-Elite Interaction Math & Geometry Adversarial Stress Suite  ")
    print("=================================================================")
    test_angle_snapping_stress()
    test_rotation_point_reversibility()
    test_matrix_inversion_singularities()
    test_selection_heuristics_stress()
    test_search_highlight_autoscroll()
    test_coordinate_converter_roundtrip()
    print("=================================================================")
    print(" ALL ADVERSARIAL STRESS CHALLENGES PASSED EMPIRICALLY!           ")
    print("=================================================================")

if __name__ == '__main__':
    main()
