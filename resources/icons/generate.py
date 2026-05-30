# gen_icons.py : generates the C_Structures icon library.
# Produces three coordinated variants per component:
#   symbols/    -> traditional civil-engineering line symbols (schematic, 2D)
#   realistic/2d/ -> shaded flat-elevation renders (material colours, depth shading)
#   realistic/3d/ -> isometric shaded renders (pseudo-3D, light/shadow faces)
# Categories: joints (supports/connections), beams (sections + structural types), forces.

import os, math

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)))  
CATS = ["joints", "beams", "forces"]
VB = 128

INK="#1f2a37"; INK2="#475569"; HATCH="#1f2a37"
STEEL_HI="#e8edf2"; STEEL="#9aa7b4"; STEEL_LO="#5b6675"; STEEL_DK="#3d4654"
ACCENT="#c0392b"; ACCENT2="#2563eb"; MOMENT="#7c3aed"; GROUND="#64748b"

def svg_open(title):
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {VB} {VB}" '
            f'width="128" height="128" role="img" aria-label="{title}"><title>{title}</title>')
def svg_close(): return "</svg>"
def defs(*b): return "<defs>"+"".join(b)+"</defs>"
def lin_grad(g,c1,c2,x1=0,y1=0,x2=0,y2=1):
    return (f'<linearGradient id="{g}" x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}">'
            f'<stop offset="0" stop-color="{c1}"/><stop offset="1" stop-color="{c2}"/></linearGradient>')
def rad_grad(g,c1,c2):
    return (f'<radialGradient id="{g}" cx="0.35" cy="0.30" r="0.85">'
            f'<stop offset="0" stop-color="{c1}"/><stop offset="1" stop-color="{c2}"/></radialGradient>')
def hatch_pattern(p,stroke=HATCH):
    return (f'<pattern id="{p}" width="7" height="7" patternUnits="userSpaceOnUse" patternTransform="rotate(45)">'
            f'<line x1="0" y1="0" x2="0" y2="7" stroke="{stroke}" stroke-width="1.4"/></pattern>')
def line(x1,y1,x2,y2,col=INK,w=3,cap="round",dash=None):
    d=f' stroke-dasharray="{dash}"' if dash else ""
    return (f'<line x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" '
            f'stroke="{col}" stroke-width="{w}" stroke-linecap="{cap}"{d}/>')
def circle(cx,cy,r,fill="none",stroke=INK,w=3):
    return f'<circle cx="{cx:.1f}" cy="{cy:.1f}" r="{r:.1f}" fill="{fill}" stroke="{stroke}" stroke-width="{w}"/>'
def poly(points,fill=INK,stroke="none",w=2,closed=True):
    pts=" ".join(f"{x:.1f},{y:.1f}" for x,y in points)
    tag="polygon" if closed else "polyline"
    return f'<{tag} points="{pts}" fill="{fill}" stroke="{stroke}" stroke-width="{w}" stroke-linejoin="round"/>'
def path(d,fill="none",stroke=INK,w=3,cap="round"):
    return f'<path d="{d}" fill="{fill}" stroke="{stroke}" stroke-width="{w}" stroke-linecap="{cap}" stroke-linejoin="round"/>'
def rect(x,y,w,h,fill=INK,stroke="none",sw=2,rx=0):
    return (f'<rect x="{x:.1f}" y="{y:.1f}" width="{w:.1f}" height="{h:.1f}" rx="{rx}" '
            f'fill="{fill}" stroke="{stroke}" stroke-width="{sw}"/>')
def text(x,y,s,size=11,col=INK2,anchor="middle",weight="600"):
    return (f'<text x="{x}" y="{y}" font-family="Segoe UI,Arial,sans-serif" font-size="{size}" '
            f'font-weight="{weight}" fill="{col}" text-anchor="{anchor}">{s}</text>')
def arrow(x1,y1,x2,y2,col=ACCENT,w=4,head=9):
    ang=math.atan2(y2-y1,x2-x1)
    hx1=x2-head*math.cos(ang-0.42); hy1=y2-head*math.sin(ang-0.42)
    hx2=x2-head*math.cos(ang+0.42); hy2=y2-head*math.sin(ang+0.42)
    return line(x1,y1,x2,y2,col,w)+poly([(x2,y2),(hx1,hy1),(hx2,hy2)],fill=col)
def iso(x,y,z,ox=64,oy=78,s=1.0):
    a=math.radians(30)
    return (ox+(x-z)*math.cos(a)*s, oy-(y+(x+z)*math.sin(a))*s)
def hatch_def(p): return defs(hatch_pattern(p))
def bar3d(x,y,z,w,h,d,grad):
    p000=iso(x,y,z);p100=iso(x+w,y,z);p010=iso(x,y+h,z);p110=iso(x+w,y+h,z)
    p001=iso(x,y,z+d);p101=iso(x+w,y,z+d);p011=iso(x,y+h,z+d);p111=iso(x+w,y+h,z+d)
    top=poly([p011,p111,p110,p010],fill=f"url(#{grad})",stroke=STEEL_DK)
    left=poly([p010,p110,p100,p000],fill=STEEL,stroke=STEEL_DK)
    right=poly([p110,p111,p101,p100],fill=STEEL_LO,stroke=STEEL_DK)
    return top+right+left
def column3d(x,y,z,w,h): return bar3d(x-w/2,y,z,w,h,w,"b2")

# ---- JOINTS
def joint_free():
    t="Free node (internal joint)"
    sym=svg_open(t)+line(28,64,100,64,INK2,2,dash="5 5")+circle(64,64,11,fill="#ffffff")+circle(64,64,4,fill=INK)+svg_close()
    r2=svg_open(t)+defs(rad_grad("fn",STEEL_HI,STEEL_LO))+circle(64,64,18,fill="url(#fn)",stroke=STEEL_DK,w=2)+circle(64,64,6,fill=STEEL_DK)+svg_close()
    bx,by=iso(0,0,0)
    r3=svg_open(t)+defs(rad_grad("fn3",STEEL_HI,STEEL_LO))+f'<ellipse cx="{bx}" cy="{by+3}" rx="18" ry="6" fill="#00000022"/>'+circle(bx,by,16,fill="url(#fn3)",stroke=STEEL_DK,w=2)+svg_close()
    return dict(symbol=sym,r2d=r2,r3d=r3)
def joint_fixed():
    t="Fixed support (encastré)"
    sym=svg_open(t)+hatch_def("hf")+line(64,30,64,84,INK,4)+line(34,84,94,84,INK,4)+rect(34,84,60,11,fill="url(#hf)")+svg_close()
    g=defs(lin_grad("cf","#b9c0c9","#6b7480"),lin_grad("mf",STEEL_HI,STEEL_LO))
    r2=svg_open(t)+g+rect(26,86,76,18,fill="url(#cf)",stroke=STEEL_DK,sw=2,rx=2)+rect(54,30,20,58,fill="url(#mf)",stroke=STEEL_DK,sw=2,rx=2)+svg_close()
    base=[iso(-22,0,-22),iso(22,0,-22),iso(22,0,22),iso(-22,0,22)]
    topf=[iso(-22,8,-22),iso(22,8,-22),iso(22,8,22),iso(-22,8,22)]
    r3=svg_open(t)+defs(lin_grad("b1","#7c8694","#4b5563"),lin_grad("b2",STEEL,STEEL_LO))+poly([topf[0],topf[1],topf[2],topf[3]],fill="url(#b2)",stroke=STEEL_DK)+poly([topf[1],base[1],base[2],topf[2]],fill="url(#b1)",stroke=STEEL_DK)+poly([topf[2],base[2],base[3],topf[3]],fill="#475569",stroke=STEEL_DK)+column3d(0,8,0,10,34)+svg_close()
    return dict(symbol=sym,r2d=r2,r3d=r3)
def joint_pin():
    t="Pinned support (pin / hinge)"
    sym=svg_open(t)+hatch_def("hp")+circle(64,46,4,fill=INK)+poly([(64,46),(44,82),(84,82)],fill="none",stroke=INK,w=3.5)+line(38,82,90,82,INK,4)+rect(38,82,52,11,fill="url(#hp)")+svg_close()
    g=defs(lin_grad("pp",STEEL_HI,STEEL_LO),lin_grad("pb","#b9c0c9","#6b7480"))
    r2=svg_open(t)+g+rect(30,92,68,14,fill="url(#pb)",stroke=STEEL_DK,sw=2,rx=2)+poly([(64,42),(40,92),(88,92)],fill="url(#pp)",stroke=STEEL_DK,w=2)+circle(64,46,7,fill="#cfd6dd",stroke=STEEL_DK,w=2)+circle(64,46,2.5,fill=STEEL_DK)+svg_close()
    r3=svg_open(t)+defs(lin_grad("pp3",STEEL_HI,STEEL_LO))+'<ellipse cx="64" cy="104" rx="30" ry="8" fill="#00000022"/>'+poly([iso(0,30,0),iso(-20,0,-12),iso(-20,0,12)],fill="url(#pp3)",stroke=STEEL_DK)+poly([iso(0,30,0),iso(20,0,-12),iso(20,0,12)],fill=STEEL_LO,stroke=STEEL_DK)+circle(*iso(0,30,0),6,fill="#cfd6dd",stroke=STEEL_DK,w=2)+svg_close()
    return dict(symbol=sym,r2d=r2,r3d=r3)
def _roller(t,axis):
    sym=svg_open(t)+hatch_def("hr")+circle(64,44,4,fill=INK)+poly([(64,44),(46,74),(82,74)],fill="none",stroke=INK,w=3.5)+circle(53,81,6,fill="none",stroke=INK,w=2.5)+circle(75,81,6,fill="none",stroke=INK,w=2.5)+line(40,90,88,90,INK,4)+rect(40,90,48,10,fill="url(#hr)")+text(64,116,axis,13,INK2)+svg_close()
    g=defs(lin_grad("rp",STEEL_HI,STEEL_LO),lin_grad("rb","#b9c0c9","#6b7480"),rad_grad("rw",STEEL_HI,STEEL_LO))
    r2=svg_open(t)+g+rect(28,96,72,12,fill="url(#rb)",stroke=STEEL_DK,sw=2,rx=2)+poly([(64,40),(44,82),(84,82)],fill="url(#rp)",stroke=STEEL_DK,w=2)+circle(52,90,7,fill="url(#rw)",stroke=STEEL_DK,w=2)+circle(76,90,7,fill="url(#rw)",stroke=STEEL_DK,w=2)+text(64,122,axis,12,INK2)+svg_close()
    r3=svg_open(t)+defs(rad_grad("rw3",STEEL_HI,STEEL_LO),lin_grad("rp3",STEEL_HI,STEEL_LO))+'<ellipse cx="64" cy="108" rx="32" ry="8" fill="#00000022"/>'+poly([iso(0,28,0),iso(-18,4,-10),iso(-18,4,10)],fill="url(#rp3)",stroke=STEEL_DK)+poly([iso(0,28,0),iso(18,4,-10),iso(18,4,10)],fill=STEEL_LO,stroke=STEEL_DK)+circle(*iso(-10,2,0),6,fill="url(#rw3)",stroke=STEEL_DK,w=2)+circle(*iso(12,2,0),6,fill="url(#rw3)",stroke=STEEL_DK,w=2)+text(64,122,axis,12,INK2)+svg_close()
    return dict(symbol=sym,r2d=r2,r3d=r3)
def joint_roller_x(): return _roller("Roller support (X-constrained)","Ux=0")
def joint_roller_y(): return _roller("Roller support (Y-constrained)","Uy=0")
def joint_roller_z(): return _roller("Roller support (Z-constrained)","Uz=0")
def joint_internal_hinge():
    t="Internal hinge"
    sym=svg_open(t)+line(20,64,58,64,INK,4)+line(70,64,108,64,INK,4)+circle(64,64,7,fill="#ffffff",stroke=INK,w=3)+svg_close()
    g=defs(lin_grad("ih",STEEL_HI,STEEL_LO))
    r2=svg_open(t)+g+rect(18,58,40,12,fill="url(#ih)",stroke=STEEL_DK,sw=2,rx=2)+rect(70,58,40,12,fill="url(#ih)",stroke=STEEL_DK,sw=2,rx=2)+circle(64,64,8,fill="#cfd6dd",stroke=STEEL_DK,w=2)+circle(64,64,2.5,fill=STEEL_DK)+svg_close()
    r3=svg_open(t)+defs(lin_grad("ih3",STEEL_HI,STEEL_LO))+bar3d(-30,0,0,26,8,8,"ih3")+bar3d(6,0,0,26,8,8,"ih3")+circle(*iso(0,4,0),7,fill="#cfd6dd",stroke=STEEL_DK,w=2)+svg_close()
    return dict(symbol=sym,r2d=r2,r3d=r3)
def joint_rigid():
    t="Rigid (moment) connection"
    sym=svg_open(t)+line(24,90,24,30,INK,4)+line(24,30,96,30,INK,4)+poly([(24,52),(46,30)],fill="none",stroke=INK2,w=2.5,closed=False)+rect(18,24,12,12,fill=INK)+svg_close()
    g=defs(lin_grad("rg",STEEL_HI,STEEL_LO),lin_grad("rg2",STEEL,STEEL_DK))
    r2=svg_open(t)+g+rect(20,28,16,70,fill="url(#rg)",stroke=STEEL_DK,sw=2,rx=2)+rect(20,28,80,16,fill="url(#rg)",stroke=STEEL_DK,sw=2,rx=2)+poly([(36,44),(52,44),(36,60)],fill="url(#rg2)")+svg_close()
    r3=svg_open(t)+defs(lin_grad("rg3",STEEL_HI,STEEL_LO))+bar3d(-18,0,0,8,44,8,"rg3")+bar3d(-18,36,0,44,8,8,"rg3")+svg_close()
    return dict(symbol=sym,r2d=r2,r3d=r3)

# ---- SECTIONS
SECTIONS={"i_beam":("I-section (universal beam)","I"),"h_column":("H-section (universal column)","H"),
 "channel_c":("Channel (C / PFC)","C"),"angle_l":("Angle (L)","L"),"t_section":("Tee (T)","T"),
 "box_rhs":("Box / RHS (hollow rect)","BOX"),"pipe_chs":("Pipe / CHS (hollow round)","PIPE"),
 "solid_rect":("Solid rectangular bar","RECT"),"solid_round":("Solid round bar","ROUND")}
SECTIONS_LABEL={"I":"I","H":"H","C":"C","L":"L","T":"T","BOX":"box","PIPE":"pipe","RECT":"bar","ROUND":"round"}
def section_profile(kind):
    cx,cy=64,60
    if kind in("I","H"):
        bf,d,tf,tw=(40,56,11,12) if kind=="H" else (44,56,10,10)
        x0=cx-bf/2;x1=cx+bf/2;y0=cy-d/2;y1=cy+d/2;wx0=cx-tw/2;wx1=cx+tw/2
        return("poly",[(x0,y0),(x1,y0),(x1,y0+tf),(wx1,y0+tf),(wx1,y1-tf),(x1,y1-tf),(x1,y1),(x0,y1),(x0,y1-tf),(wx0,y1-tf),(wx0,y0+tf),(x0,y0+tf)],None)
    if kind=="C":
        bf,d,tf,tw=38,56,10,10;x0=cx-bf/2;x1=cx+bf/2;y0=cy-d/2;y1=cy+d/2
        return("poly",[(x0,y0),(x1,y0),(x1,y0+tf),(x0+tw,y0+tf),(x0+tw,y1-tf),(x1,y1-tf),(x1,y1),(x0,y1)],None)
    if kind=="L":
        s,tw=50,11;x0=cx-25;y0=cy-25
        return("poly",[(x0,y0),(x0+tw,y0),(x0+tw,y0+s-tw),(x0+s,y0+s-tw),(x0+s,y0+s),(x0,y0+s)],None)
    if kind=="T":
        bf,d,tf,tw=50,52,11,11;x0=cx-bf/2;x1=cx+bf/2;y0=cy-d/2
        return("poly",[(x0,y0),(x1,y0),(x1,y0+tf),(cx+tw/2,y0+tf),(cx+tw/2,y0+d),(cx-tw/2,y0+d),(cx-tw/2,y0+tf),(x0,y0+tf)],None)
    if kind=="BOX":
        w,h,t=52,52,9
        return("hollow_rect",[(cx-w/2,cy-h/2),(cx+w/2,cy-h/2),(cx+w/2,cy+h/2),(cx-w/2,cy+h/2)],[(cx-w/2+t,cy-h/2+t),(cx+w/2-t,cy-h/2+t),(cx+w/2-t,cy+h/2-t),(cx-w/2+t,cy+h/2-t)])
    if kind=="PIPE": return("hollow_round",(cx,cy,28),(cx,cy,19))
    if kind=="RECT": return("rect",(cx-21,cy-28,42,56),None)
    if kind=="ROUND": return("round",(cx,cy,28),None)
    return("poly",[],None)
def section_symbol(title,kind):
    shape,a,b=section_profile(kind)
    if shape=="poly": body=poly(a,fill="none",stroke=INK,w=3)
    elif shape=="hollow_rect": body=poly(a,fill="none",stroke=INK,w=3)+poly(b,fill="none",stroke=INK,w=2.5)
    elif shape=="hollow_round": body=circle(a[0],a[1],a[2],fill="none",stroke=INK,w=3)+circle(b[0],b[1],b[2],fill="none",stroke=INK,w=2.5)
    elif shape=="rect": body=rect(*a,fill="none",stroke=INK,sw=3)
    elif shape=="round": body=circle(a[0],a[1],a[2],fill="none",stroke=INK,w=3)
    return svg_open(title)+body+text(64,116,SECTIONS_LABEL.get(kind,""),12,INK2)+svg_close()
def section_2d(title,kind):
    shape,a,b=section_profile(kind)
    g=defs(lin_grad("sg",STEEL_HI,STEEL_LO))
    if shape=="poly": body=poly(a,fill="url(#sg)",stroke=STEEL_DK,w=2)
    elif shape=="hollow_rect": body=poly(a,fill="url(#sg)",stroke=STEEL_DK,w=2)+poly(b,fill="#2f3742",stroke=STEEL_DK,w=1.5)
    elif shape=="hollow_round": body=circle(a[0],a[1],a[2],fill="url(#sg)",stroke=STEEL_DK,w=2)+circle(b[0],b[1],b[2],fill="#2f3742",stroke=STEEL_DK,w=1.5)
    elif shape=="rect": body=rect(*a,fill="url(#sg)",stroke=STEEL_DK,sw=2,rx=2)
    elif shape=="round": body=circle(a[0],a[1],a[2],fill="url(#sg)",stroke=STEEL_DK,w=2)
    return svg_open(title)+g+body+svg_close()
def section_3d(title,kind):
    g=defs(lin_grad("eg",STEEL_HI,STEEL),lin_grad("eg2",STEEL,STEEL_LO),lin_grad("eg3",STEEL_LO,STEEL_DK))
    L=46;shape,a,b=section_profile(kind)
    parts=[g,'<ellipse cx="74" cy="104" rx="40" ry="9" fill="#00000020"/>']
    def fp(px,py,depth):
        z=(px-64)*0.9;y=(60-py)*0.9;return iso(depth,y,z)
    if shape in("poly","rect"):
        pts=[(a[0],a[1]),(a[0]+a[2],a[1]),(a[0]+a[2],a[1]+a[3]),(a[0],a[1]+a[3])] if shape=="rect" else a
        front=[fp(px,py,0) for px,py in pts];back=[fp(px,py,L) for px,py in pts]
        for i in range(len(pts)):
            j=(i+1)%len(pts);parts.append(poly([front[i],front[j],back[j],back[i]],fill="url(#eg2)",stroke=STEEL_DK,w=1.2))
        parts.append(poly(back,fill="url(#eg3)",stroke=STEEL_DK,w=1.2));parts.append(poly(front,fill="url(#eg)",stroke=STEEL_DK,w=1.5))
    elif shape=="round":
        cxp,cyp,r=a;f=fp(cxp,cyp,0);bk=fp(cxp,cyp,L);rr=r*0.9
        parts.append(poly([(f[0],f[1]-rr),(bk[0],bk[1]-rr),(bk[0],bk[1]+rr),(f[0],f[1]+rr)],fill="url(#eg2)",stroke=STEEL_DK,w=1))
        parts.append(f'<ellipse cx="{bk[0]:.1f}" cy="{bk[1]:.1f}" rx="{rr*0.5:.1f}" ry="{rr:.1f}" fill="url(#eg3)" stroke="{STEEL_DK}" stroke-width="1"/>')
        parts.append(f'<ellipse cx="{f[0]:.1f}" cy="{f[1]:.1f}" rx="{rr*0.5:.1f}" ry="{rr:.1f}" fill="url(#eg)" stroke="{STEEL_DK}" stroke-width="1.5"/>')
    elif shape=="hollow_round":
        cxp,cyp,r=a;_,_,ri=b;f=fp(cxp,cyp,0);bk=fp(cxp,cyp,L);rr=r*0.9;rri=ri*0.9
        parts.append(poly([(f[0],f[1]-rr),(bk[0],bk[1]-rr),(bk[0],bk[1]+rr),(f[0],f[1]+rr)],fill="url(#eg2)",stroke=STEEL_DK,w=1))
        parts.append(f'<ellipse cx="{f[0]:.1f}" cy="{f[1]:.1f}" rx="{rr*0.5:.1f}" ry="{rr:.1f}" fill="url(#eg)" stroke="{STEEL_DK}" stroke-width="1.5"/>')
        parts.append(f'<ellipse cx="{f[0]:.1f}" cy="{f[1]:.1f}" rx="{rri*0.5:.1f}" ry="{rri:.1f}" fill="#2f3742" stroke="{STEEL_DK}" stroke-width="1"/>')
    elif shape=="hollow_rect":
        pts=a;front=[fp(px,py,0) for px,py in pts];back=[fp(px,py,L) for px,py in pts]
        for i in range(len(pts)):
            j=(i+1)%len(pts);parts.append(poly([front[i],front[j],back[j],back[i]],fill="url(#eg2)",stroke=STEEL_DK,w=1.2))
        parts.append(poly(front,fill="url(#eg)",stroke=STEEL_DK,w=1.5))
        parts.append(poly([fp(px,py,0) for px,py in b],fill="#2f3742",stroke=STEEL_DK,w=1))
    return svg_open(title)+"".join(parts)+svg_close()

# ---- BEAM TYPES
def _support_tri(x,y): return poly([(x,y-12),(x-9,y),(x+9,y)],fill="none",stroke=INK,w=2.5)+line(x-12,y,x+12,y,INK,2.5)
def _support_roller(x,y): return poly([(x,y-12),(x-9,y-3),(x+9,y-3)],fill="none",stroke=INK,w=2.5)+circle(x-5,y+1,3,fill="none",stroke=INK,w=2)+circle(x+5,y+1,3,fill="none",stroke=INK,w=2)+line(x-12,y+5,x+12,y+5,INK,2.5)
def supp_block(x,y): return poly([(x,y-4),(x-10,y+8),(x+10,y+8)],fill="#8a93a0",stroke=STEEL_DK,w=1.5)
def truss_member_2d(x1,y1,x2,y2):
    ang=math.atan2(y2-y1,x2-x1);w=5;dx=math.sin(ang)*w;dy=-math.cos(ang)*w
    return poly([(x1+dx,y1+dy),(x2+dx,y2+dy),(x2-dx,y2-dy),(x1-dx,y1-dy)],fill="url(#bt)",stroke=STEEL_DK,w=1.5)
def beamtype(kind):
    labels={"simply_supported":"Simply supported beam","cantilever":"Cantilever beam","fixed_both":"Fixed-fixed beam","continuous":"Continuous beam","overhanging":"Overhanging beam","truss":"Truss member"}
    t=labels[kind];g=defs(hatch_pattern("htb"));s=svg_open(t)+g;y=60
    if kind=="simply_supported": s+=line(28,y,100,y,INK,5)+_support_tri(34,y+5)+_support_roller(94,y+5)
    elif kind=="cantilever": s+=line(34,y,104,y,INK,5)+line(34,y-16,34,y+16,INK,3)+rect(28,y-16,6,32,fill="url(#htb)")
    elif kind=="fixed_both": s+=line(30,y,98,y,INK,5)+line(30,y-16,30,y+16,INK,3)+rect(24,y-16,6,32,fill="url(#htb)")+line(98,y-16,98,y+16,INK,3)+rect(98,y-16,6,32,fill="url(#htb)")
    elif kind=="continuous": s+=line(20,y,108,y,INK,5)+_support_tri(26,y+5)+_support_roller(64,y+5)+_support_roller(102,y+5)
    elif kind=="overhanging": s+=line(18,y,110,y,INK,5)+_support_tri(40,y+5)+_support_roller(92,y+5)
    elif kind=="truss": s+=poly([(24,86),(64,30),(104,86)],fill="none",stroke=INK,w=4)+line(24,86,104,86,INK,4)+line(64,30,64,86,INK,3)+circle(24,86,4,fill=INK)+circle(104,86,4,fill=INK)+circle(64,30,4,fill=INK)+circle(64,86,4,fill=INK)
    s+=svg_close()
    g2=defs(lin_grad("bt",STEEL_HI,STEEL_LO));r2=svg_open(t)+g2
    if kind=="truss":
        for (x1,y1,x2,y2) in [(28,92,64,34),(64,34,100,92),(28,92,100,92),(64,34,64,92)]: r2+=truss_member_2d(x1,y1,x2,y2)
        for (cx,cy) in [(28,92),(100,92),(64,34),(64,92)]: r2+=circle(cx,cy,5,fill="#cfd6dd",stroke=STEEL_DK,w=2)
    else:
        r2+=rect(26,y-7,76,14,fill="url(#bt)",stroke=STEEL_DK,sw=2,rx=2)
        if kind in("simply_supported","continuous","overhanging"):
            r2+=supp_block(34,y+8)+supp_block(94,y+8)
            if kind=="continuous": r2+=supp_block(64,y+8)
        if kind in("cantilever","fixed_both"):
            r2+=rect(20,y-20,8,40,fill="#6b7480",stroke=STEEL_DK,sw=2,rx=2)
            if kind=="fixed_both": r2+=rect(100,y-20,8,40,fill="#6b7480",stroke=STEEL_DK,sw=2,rx=2)
    r2+=svg_close()
    r3=svg_open(t)+defs(lin_grad("bt3",STEEL_HI,STEEL),lin_grad("bt3b",STEEL,STEEL_LO))+'<ellipse cx="64" cy="100" rx="46" ry="9" fill="#00000020"/>'
    if kind=="truss": r3+=bar3d(-30,0,0,60,7,7,"bt3")+bar3d(-30,0,0,7,40,7,"bt3")
    else:
        r3+=bar3d(-34,6,0,68,10,12,"bt3")
        if kind in("cantilever","fixed_both"): r3+=bar3d(-40,0,-2,8,40,16,"bt3b")
    r3+=svg_close()
    return dict(symbol=s,r2d=r2,r3d=r3)

# ---- FORCES
def force(kind):
    labels={"point_load":"Point load (concentrated force)","udl":"Uniformly distributed load (UDL)","triangular_load":"Triangular / varying distributed load","moment":"Moment (couple)","tension":"Axial tension","compression":"Axial compression","shear":"Shear force","reaction":"Support reaction","self_weight":"Self-weight / gravity load"}
    t=labels[kind]
    beam=lambda col=INK,w=5: line(20,76,108,76,col,w)
    sym=svg_open(t)
    if kind=="point_load": sym+=beam()+arrow(64,20,64,70,ACCENT,5)+text(64,16,"P",12,ACCENT)
    elif kind=="udl":
        sym+=beam()+rect(24,26,80,8,fill="none",stroke=ACCENT,sw=2)
        for x in range(28,105,12): sym+=arrow(x,34,x,70,ACCENT,3,7)
        sym+=text(64,20,"w",12,ACCENT)
    elif kind=="triangular_load":
        sym+=beam()+poly([(24,70),(104,30),(104,70)],fill="none",stroke=ACCENT,w=2)
        for x in range(30,105,12):
            top=70-(x-24)/80*40;sym+=arrow(x,top,x,70,ACCENT,2.5,6)
    elif kind=="moment": sym+=beam()+path("M 44 52 A 22 22 0 1 1 84 52",fill="none",stroke=MOMENT,w=4)+poly([(84,52),(76,46),(88,42)],fill=MOMENT)+text(64,40,"M",12,MOMENT)
    elif kind=="tension": sym+=beam(INK2,4)+arrow(58,76,20,76,ACCENT2,5)+arrow(70,76,108,76,ACCENT2,5)+text(64,40,"Tension",11,ACCENT2)
    elif kind=="compression": sym+=beam(INK2,4)+arrow(20,76,52,76,ACCENT,5)+arrow(108,76,76,76,ACCENT,5)+text(64,40,"Compression",10,ACCENT)
    elif kind=="shear": sym+=line(64,24,64,72,INK2,4)+arrow(40,40,40,72,ACCENT,5)+arrow(88,72,88,40,ACCENT,5)
    elif kind=="reaction": sym+=beam()+_support_tri(64,81)+arrow(64,116,64,86,ACCENT2,5)+text(80,108,"R",12,ACCENT2,anchor="start")
    elif kind=="self_weight":
        sym+=beam()
        for x in range(30,105,16): sym+=arrow(x,30,x,68,GROUND,3,7)
        sym+=text(64,22,"g",12,GROUND)
    sym+=svg_close()
    g=defs(lin_grad("fb",STEEL_HI,STEEL_LO),lin_grad("ar","#e74c3c","#a02b1d"),lin_grad("at","#3b82f6","#1d4ed8"),lin_grad("am","#a78bfa","#6d28d9"))
    def ga(x1,y1,x2,y2,grad):
        ang=math.atan2(y2-y1,x2-x1);head=11
        hx1=x2-head*math.cos(ang-0.45);hy1=y2-head*math.sin(ang-0.45);hx2=x2-head*math.cos(ang+0.45);hy2=y2-head*math.sin(ang+0.45)
        return line(x1,y1,x2,y2,f"url(#{grad})",6)+poly([(x2,y2),(hx1,hy1),(hx2,hy2)],fill=f"url(#{grad})")
    bbeam=rect(20,72,88,12,fill="url(#fb)",stroke=STEEL_DK,sw=2,rx=3)
    r2=svg_open(t)+g+bbeam
    if kind=="point_load": r2+=ga(64,22,64,68,"ar")
    elif kind=="udl":
        r2+=rect(24,24,80,8,fill="#e74c3c",rx=2)
        for x in range(30,101,14): r2+=ga(x,32,x,68,"ar")
    elif kind=="triangular_load":
        r2+=poly([(24,68),(104,26),(104,68)],fill="#e74c3c44",stroke="#a02b1d",w=1.5)
        for x in range(32,101,14):
            top=68-(x-24)/80*42;r2+=ga(x,top,x,68,"ar")
    elif kind=="moment": r2+=path("M 42 50 A 24 24 0 1 1 86 50",fill="none",stroke="url(#am)",w=6)+poly([(86,50),(77,44),(90,38)],fill="#6d28d9")
    elif kind=="tension": r2+=ga(56,78,18,78,"at")+ga(72,78,110,78,"at")
    elif kind=="compression": r2+=ga(18,78,52,78,"ar")+ga(110,78,76,78,"ar")
    elif kind=="shear": r2+=ga(42,38,42,70,"ar")+ga(90,70,90,38,"ar")
    elif kind=="reaction": r2+=ga(64,112,64,86,"at")
    elif kind=="self_weight":
        for x in range(30,101,16): r2+=ga(x,30,x,68,"ar")
    r2+=svg_close()
    r3=svg_open(t)+defs(lin_grad("fb3",STEEL_HI,STEEL),lin_grad("fb3b",STEEL,STEEL_LO),lin_grad("ar3","#e74c3c","#a02b1d"),lin_grad("at3","#3b82f6","#1d4ed8"),lin_grad("am","#a78bfa","#6d28d9"))
    r3+='<ellipse cx="64" cy="104" rx="44" ry="8" fill="#00000020"/>'+bar3d(-34,2,0,68,9,11,"fb3")
    def iso_arrow(x,y1,y2,grad):
        p1=iso(x,y1,0);p2=iso(x,y2,0);return ga(p1[0],p1[1],p2[0],p2[1],grad)
    if kind in("point_load","self_weight","udl","triangular_load"):
        xs=[0] if kind=="point_load" else range(-24,25,12)
        for x in xs:
            top=44 if kind!="triangular_load" else 24+(x+24)/48*24
            r3+=iso_arrow(x,top,14,"ar3")
    elif kind=="moment": r3+=path("M 48 54 A 20 20 0 1 1 84 50",fill="none",stroke="url(#am)",w=6)+poly([(84,50),(76,45),(88,39)],fill="#6d28d9")
    elif kind in("tension","compression"):
        pL=iso(-34,8,0);pR=iso(34,8,0)
        if kind=="tension": r3+=ga(pL[0]+10,pL[1],pL[0]-16,pL[1],"at3")+ga(pR[0]-10,pR[1],pR[0]+16,pR[1],"at3")
        else: r3+=ga(pL[0]-16,pL[1],pL[0]+12,pL[1],"ar3")+ga(pR[0]+16,pR[1],pR[0]-12,pR[1],"ar3")
    elif kind=="shear": r3+=iso_arrow(-12,40,12,"ar3")+iso_arrow(12,12,40,"ar3")
    elif kind=="reaction":
        p=iso(0,-8,0);p2=iso(0,8,0);r3+=ga(p[0],p[1]+20,p2[0],p2[1],"at3")
    r3+=svg_close()
    return dict(symbol=sym,r2d=r2,r3d=r3)

def build_all():
    items={"joints":{},"beams":{},"forces":{}}
    items["joints"]["free"]=joint_free();items["joints"]["fixed"]=joint_fixed();items["joints"]["pin_xy"]=joint_pin()
    items["joints"]["roller_x"]=joint_roller_x();items["joints"]["roller_y"]=joint_roller_y();items["joints"]["roller_z"]=joint_roller_z()
    items["joints"]["internal_hinge"]=joint_internal_hinge();items["joints"]["rigid"]=joint_rigid()
    for key,(title,kind) in SECTIONS.items():
        items["beams"][key]=dict(symbol=section_symbol(title,kind),r2d=section_2d(title,kind),r3d=section_3d(title,kind))
    for k in ["simply_supported","cantilever","fixed_both","continuous","overhanging","truss"]: items["beams"][k]=beamtype(k)
    for k in ["point_load","udl","triangular_load","moment","tension","compression","shear","reaction","self_weight"]: items["forces"][k]=force(k)
    return items

def main():
    items=build_all()
    layout={"symbols":"symbol","realistic/2d":"r2d","realistic/3d":"r3d"};count=0
    for variant,key in layout.items():
        for cat in CATS:
            d=os.path.join(ROOT,variant,cat);os.makedirs(d,exist_ok=True)
            for name,svgs in items[cat].items():
                with open(os.path.join(d,f"{name}.svg"),"w") as f: f.write(svgs[key])
                count+=1
    print("SVG files written:",count)
    for cat in CATS: print(f"  {cat}: {len(items[cat])} components x 3 variants")

if __name__=="__main__": main()
