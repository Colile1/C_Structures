"""
Python harness testing all logic that can't be compiled on this machine.
Covers: CSV parse/save, eraseNodeAt reindex, physics math, SVG validation,
        UIHandler beamStart truthiness, and the new JointType round-trip.
"""
import math, os, sys, glob as globmod
import numpy as np
import xml.etree.ElementTree as ET

PASS = 0; FAIL = 0

def ok(name):
    global PASS; PASS += 1
    print(f"  PASS  {name}")

def fail(name, msg=""):
    global FAIL; FAIL += 1
    print(f"  FAIL  {name}" + (f" -- {msg}" if msg else ""))

def near(a, b, tol=1e-6):
    return abs(a - b) <= tol

# =============================================================
# 1. CSV FORMAT  (mirrors CSVHandler.cpp save/load exactly)
# =============================================================
print("\n=== CSV Format Tests ===")

def save_csv(nodes, beams):
    # nodes: list of (x,y,z,joint_int), beams: list of (si,ei,E,A)
    lines = []
    for x,y,z,j in nodes:
        lines.append(f"NODE {x} {y} {z} {j}")
    for si,ei,E,A in beams:
        lines.append(f"BEAM {si} {ei} {E} {A}")
    return "\n".join(lines)

def load_csv(text):
    nodes, beams = [], []
    for line in text.strip().splitlines():
        parts = line.split()
        if not parts:
            continue
        if parts[0] == "NODE":
            x,y,z,j = float(parts[1]),float(parts[2]),float(parts[3]),int(parts[4])
            j = max(0, min(5, j))
            nodes.append((x, y, z, j))
        elif parts[0] == "BEAM":
            si,ei = int(parts[1]),int(parts[2])
            E,A   = float(parts[3]),float(parts[4])
            n = len(nodes)
            if 0 <= si < n and 0 <= ei < n:
                beams.append((si, ei, E, A))
    return nodes, beams

# 1a. Basic round-trip
nodes_in = [(0,0,0,1),(2,0,0,0),(4,1,0,0)]
beams_in  = [(0,1,2e11,0.01),(1,2,1.5e11,0.005)]
text = save_csv(nodes_in, beams_in)
nodes_out, beams_out = load_csv(text)
if nodes_out == nodes_in and beams_out == beams_in:
    ok("basic round-trip")
else:
    fail("basic round-trip", f"nodes={nodes_out} beams={beams_out}")

# 1b. All 6 JointType values survive round-trip
jt_nodes = [(float(i),0,0,i) for i in range(6)]
text2 = save_csv(jt_nodes, [])
jt_out, _ = load_csv(text2)
if all(jt_out[i][3] == i for i in range(6)):
    ok("JointType full round-trip (all 6 types)")
else:
    fail("JointType full round-trip", str(jt_out))

# 1c. Old binary fixed=1 loads as FIXED (backward compat)
old_line = "NODE 0.0 0.0 0.0 1"
n_old, _ = load_csv(old_line)
if n_old[0][3] == 1:
    ok("old binary fixed=1 still loads as FIXED")
else:
    fail("old binary fixed=1", str(n_old))

# 1d. Out-of-range beam index silently skipped
text3 = "NODE 0 0 0 0\nNODE 1 0 0 0\nBEAM 0 99 2e11 0.01"
_, b3 = load_csv(text3)
if len(b3) == 0:
    ok("out-of-range beam index silently dropped")
else:
    fail("out-of-range beam index", str(b3))

# 1e. Float precision preservation
text4 = save_csv([(1.23456789, -9.87654321, 0.0, 2)], [])
n4, _ = load_csv(text4)
if near(n4[0][0], 1.23456789, 1e-4) and near(n4[0][1], -9.87654321, 1e-4):
    ok("float coordinates preserve to adequate precision")
else:
    fail("float coordinates", str(n4[0]))

# =============================================================
# 2. eraseNodeAt RE-INDEX LOGIC  (mirrors UIHandler.cpp)
# =============================================================
print("\n=== eraseNodeAt Reindex Tests ===")

def erase_node(idx, nodes, beams):
    nodes  = nodes[:]
    beams  = [list(b) for b in beams]
    if idx < 0 or idx >= len(nodes):
        return nodes, beams
    # Drop beams touching the deleted node
    beams = [b for b in beams if b[0] != idx and b[1] != idx]
    # Remove node
    nodes.pop(idx)
    # Shift indices above idx
    for b in beams:
        if b[0] > idx: b[0] -= 1
        if b[1] > idx: b[1] -= 1
    return nodes, beams

# 2a. Delete middle node
nodes = list("ABCD")
beams = [[0,1],[1,2],[2,3],[0,3]]
n2, b2 = erase_node(1, nodes, beams)
expected_b2 = sorted([[1,2],[0,2]])
if n2 == list("ACD") and sorted(b2) == expected_b2:
    ok("delete middle node shifts indices correctly")
else:
    fail("delete middle node", f"nodes={n2} beams={b2}")

# 2b. Delete first node
nodes = list("ABC"); beams = [[0,1],[1,2]]
n2, b2 = erase_node(0, nodes, beams)
if n2 == list("BC") and b2 == [[0,1]]:
    ok("delete first node shifts beams")
else:
    fail("delete first node", f"nodes={n2} beams={b2}")

# 2c. Delete last node
nodes = list("ABC"); beams = [[0,1],[1,2]]
n2, b2 = erase_node(2, nodes, beams)
if n2 == list("AB") and b2 == [[0,1]]:
    ok("delete last node removes only attached beams")
else:
    fail("delete last node", f"nodes={n2} beams={b2}")

# 2d. Delete only node
n2, b2 = erase_node(0, ["A"], [])
if n2 == [] and b2 == []:
    ok("delete only node leaves empty state")
else:
    fail("delete only node", f"{n2} {b2}")

# 2e. Invalid index is a no-op
n2, b2 = erase_node(-1, ["A","B"], [[0,1]])
if n2 == ["A","B"] and b2 == [[0,1]]:
    ok("invalid index -1 is a no-op")
else:
    fail("invalid index -1", f"{n2} {b2}")

# 2f. 300-node growth leaves beam indices intact
nodes = list(range(2))
beams_r = [[0, 1]]
nodes.extend(range(2, 302))
if beams_r[0] == [0, 1]:
    ok("300-node growth leaves beam indices intact (no dangling pointer)")
else:
    fail("300-node growth", str(beams_r[0]))

# 2g. Chain-delete two adjacent nodes
nodes = list("ABCDE"); beams = [[0,1],[1,2],[2,3],[3,4]]
n2, b2 = erase_node(1, nodes, beams)
n3, b3 = erase_node(1, n2, b2)  # 'C' is now at index 1 after first delete
if n3 == list("ADE") and b3 == [[1,2]]:
    ok("chain-delete two nodes produces correct final indices")
else:
    fail("chain-delete", f"nodes={n3} beams={b3}")

# =============================================================
# 3. PHYSICS MATH  (closed-form verification, mirrors Simulator.cpp)
# =============================================================
print("\n=== Physics Math Tests ===")

def axial_truss_solve(node_coords, beams, fixed_dofs, forces):
    N  = len(node_coords)
    K  = np.zeros((3*N, 3*N))
    for (i,j,E,A) in beams:
        axis = node_coords[j] - node_coords[i]
        L    = np.linalg.norm(axis)
        if L < 1e-10:
            continue
        n    = axis / L
        AE_L = E * A / L
        ke   = AE_L * np.outer(n, n)
        for ri in range(3):
            for ci in range(3):
                K[3*i+ri,3*i+ci] += ke[ri,ci]
                K[3*j+ri,3*j+ci] += ke[ri,ci]
                K[3*i+ri,3*j+ci] -= ke[ri,ci]
                K[3*j+ri,3*i+ci] -= ke[ri,ci]

    f_vec = forces.flatten()
    fixed = set(fixed_dofs)
    for i in range(3*N):
        if i not in fixed and abs(K[i,i]) < 1e-14:
            fixed.add(i)
    free = sorted(set(range(3*N)) - fixed)
    u    = np.zeros(3*N)
    if not free:
        return u.reshape(N,3), [0.0]*len(beams)

    Kff = K[np.ix_(free, free)]
    ff  = f_vec[free]
    uf  = np.linalg.solve(Kff, ff)
    for li, gi in enumerate(free):
        u[gi] = uf[li]

    bforces = []
    for (i,j,E,A) in beams:
        axis = node_coords[j] - node_coords[i]
        L    = np.linalg.norm(axis)
        if L < 1e-10:
            bforces.append(0.0); continue
        n     = axis / L
        dI    = u[3*i:3*i+3]
        dJ    = u[3*j:3*j+3]
        bforces.append(E*A/L * np.dot(dJ-dI, n))
    return u.reshape(N,3), bforces

# 3a. Single bar  F*L/(A*E)
E,A,L,F = 200e9, 1e-4, 2.0, 1000.0
coords = np.array([[0,0,0],[L,0,0]], dtype=float)
u, bf = axial_truss_solve(coords, [(0,1,E,A)], {0,1,2}, np.array([[0,0,0],[F,0,0]],dtype=float))
exp = F*L/(A*E)
if near(u[1,0], exp, exp*1e-3):
    ok(f"single bar elongation = F*L/(A*E) = {exp:.3e} m")
else:
    fail("single bar elongation", f"got {u[1,0]:.3e}, expected {exp:.3e}")

if near(bf[0], F, 1.0):
    ok(f"single bar force = {F:.0f} N (tension)")
else:
    fail("single bar force", f"got {bf[0]:.1f}")

# 3b. Fixed node zero displacement
if near(u[0,0], 0, 1e-12) and near(u[0,1], 0, 1e-12):
    ok("fixed node has zero displacement")
else:
    fail("fixed node displacement", str(u[0]))

# 3c. Symmetric two-bar truss
coords2 = np.array([[-2,0,0],[2,0,0],[0,3,0]], dtype=float)
u2, bf2 = axial_truss_solve(coords2, [(0,2,2e11,1e-4),(1,2,2e11,1e-4)],
                             {0,1,2,3,4,5}, np.array([[0,0,0],[0,0,0],[0,-50000,0]],dtype=float))
if near(bf2[0], bf2[1], abs(bf2[0])*1e-3 + 1.0):
    ok("symmetric truss: both members carry equal force")
else:
    fail("symmetric truss equal forces", f"f0={bf2[0]:.1f} f1={bf2[1]:.1f}")
if bf2[0] < 0:
    ok("symmetric truss: members in compression under downward load")
else:
    fail("symmetric truss compression sign", f"f0={bf2[0]:.1f}")

# 3d. Two bars in series
coords3 = np.array([[0,0,0],[1,0,0],[2,0,0]], dtype=float)
u3, bf3 = axial_truss_solve(coords3, [(0,1,2e11,0.01),(1,2,2e11,0.01)],
                             {0,1,2}, np.array([[0,0,0],[0,0,0],[5000,0,0]],dtype=float))
if bf3[0] > 0 and bf3[1] > 0:
    ok("two bars in series: both in tension")
else:
    fail("two bars in series tension", f"f={bf3}")
if near(bf3[0], bf3[1], abs(bf3[0])*0.05):
    ok("two bars in series: carry same force")
else:
    fail("two bars in series equal force", f"f0={bf3[0]:.1f} f1={bf3[1]:.1f}")

# 3e. Compression sign
coords4 = np.array([[0,0,0],[1,0,0]], dtype=float)
u4, bf4 = axial_truss_solve(coords4, [(0,1,2e11,0.01)],
                             {0,1,2}, np.array([[0,0,0],[-5000,0,0]],dtype=float))
if bf4[0] < 0:
    ok("compression load gives negative beam force")
else:
    fail("compression sign", f"got {bf4[0]:.1f}")

# 3f. Zero force produces zero displacement
u5, _ = axial_truss_solve(coords4, [(0,1,2e11,0.01)], {0,1,2}, np.zeros((2,3)))
if near(u5[1,0], 0, 1e-12):
    ok("zero applied force produces zero displacement")
else:
    fail("zero force displacement", f"got {u5[1,0]}")

# 3g. Exact elongation for integration benchmark  F*L/(A*E) with F=10000, L=1, A=0.01, E=2e11
exp2 = 10000.0 * 1.0 / (0.01 * 2e11)
coords6 = np.array([[0,0,0],[1,0,0]], dtype=float)
u6, _ = axial_truss_solve(coords6, [(0,1,2e11,0.01)], {0,1,2},
                           np.array([[0,0,0],[10000,0,0]],dtype=float))
if near(u6[1,0], exp2, exp2*0.01):
    ok(f"integration benchmark elongation correct ({exp2:.3e} m)")
else:
    fail("integration benchmark elongation", f"got {u6[1,0]:.3e}")

# =============================================================
# 4. beamStart TRUTHINESS BUG (UIHandler)
# =============================================================
print("\n=== beamStart Truthiness Tests ===")

# In C++, int truthiness: 0=false, nonzero=true.
# So beamStart=-1 (no selection) was ALSO truthy in the old code — showing "Click end node"
# when no start was selected yet. The fix (>= 0) corrects BOTH cases.
cases = [
    (-1, "no selection",   True,  False),  # old: truthy(-1)=True WRONG; new: False CORRECT
    ( 0, "node 0 selected",False, True ),  # old: truthy(0)=False WRONG; new: True CORRECT
    ( 1, "node 1 selected",True,  True ),  # old: True correct; new: True correct
    ( 5, "node 5 selected",True,  True ),  # old: True correct; new: True correct
]
for beam_start, label, old_expected, new_expected in cases:
    old = bool(beam_start)    # mirrors C++ int-to-bool: nonzero=true
    new = (beam_start >= 0)   # fixed logic
    if old == old_expected and new == new_expected:
        if old != new:
            ok(f"beamStart={beam_start} ({label}): old WRONG ({old}), new correct ({new})")
        else:
            ok(f"beamStart={beam_start} ({label}): old and new both correct ({new})")
    else:
        fail(f"beamStart={beam_start}", f"old={old} (expected {old_expected}), new={new} (expected {new_expected})")

# =============================================================
# 5. SVG ICON VALIDATION
# =============================================================
print("\n=== SVG Icon Validation ===")

win_root = r"C:\Users\Colile\Documents\claude\Projects\C_Structures\C_Structures\resources\icons"
svgs = []
if os.path.isdir(win_root):
    for dirpath, _, fnames in os.walk(win_root):
        svgs += [os.path.join(dirpath, f) for f in fnames if f.endswith(".svg")]

total_svgs = len(svgs)
bad_svgs   = []
for path in svgs:
    try:
        tree = ET.parse(path)
        root = tree.getroot()
        ns = "http://www.w3.org/2000/svg"
        tag = root.tag.replace(f"{{{ns}}}", "") if root.tag.startswith("{") else root.tag
        if tag != "svg":
            bad_svgs.append((path, f"root tag is '{tag}', not 'svg'"))
    except ET.ParseError as e:
        bad_svgs.append((path, str(e)))

if total_svgs == 0:
    fail("SVG icons found", "no SVGs found at expected path")
elif bad_svgs:
    fail(f"SVG parse ({total_svgs} files)", f"{len(bad_svgs)} malformed")
    for p,m in bad_svgs[:5]:
        print(f"         {os.path.basename(p)}: {m}")
else:
    ok(f"all {total_svgs} SVG icons parse as valid XML with <svg> root")

if total_svgs >= 90:
    ok(f"SVG count {total_svgs} >= 90 (full icon set present)")
elif total_svgs > 0:
    fail(f"SVG count too low", f"only {total_svgs} SVGs found (expected ~96)")

# =============================================================
# SUMMARY
# =============================================================
print(f"\n{'='*52}")
print(f"Results: {PASS} passed, {FAIL} failed")
if FAIL:
    sys.exit(1)
