import math
import random

PLANETS = [
        {
            "nome": "Sol", "tex": "sol.jpg", "scale": 4.0, "dist": 0, "orb": 0, "rot": 500,
            "colors": {"diff": '0" G="0" B="0', "amb": '0" G="0" B="0', "spec": '0" G="0" B="0', "emi": '255" G="220" B="50', "shin": '0'}
        },
        {"nome": "Mercurio", "tex": "mercurio.jpg", "scale": 0.15, "dist": 6, "orb": 20, "rot": 1000},
        {"nome": "Venus", "tex": "venus.jpg", "scale": 0.3, "dist": 9, "orb": 50, "rot": 2400},
        {
            "nome": "Terra", "tex": "terra.jpg", "scale": 0.35, "dist": 13, "orb": 80, "rot": 20,
            "colors": {"spec": '100" G="100" B="100', "shin": '50'},
            "moons": [ {"nome": "Lua", "tex": "lua.jpg", "scale": 0.08, "dist": 1.0, "orb": 2.7} ]
        },
        {
            "nome": "Marte", "tex": "marte.jpg", "scale": 0.25, "dist": 17, "orb": 150, "rot": 20,
            "colors": {"diff": '200" G="100" B="100', "amb": '50" G="25" B="25'},
            "moons": [
                {"nome": "Fobos", "tex": "fobos.jpg", "scale": [0.03, 0.02, 0.025], "dist": 0.5, "orb": 0.8},
                {"nome": "Deimos", "tex": "deimos.jpg", "scale": [0.02, 0.015, 0.022], "dist": 0.8, "orb": 1.5}
            ]
        },
        {
            "nome": "Jupiter", "tex": "jupiter.jpg", "scale": 1.2, "dist": 30, "orb": 300, "rot": 8,
            "moons": [
                {"nome": "Io", "tex": "europa.jpg", "scale": 0.08, "dist": 2.0, "orb": 1.8},
                {"nome": "Europa", "tex": "europa.jpg", "scale": 0.07, "dist": 2.5, "orb": 3.6},
                {"nome": "Ganimedes", "tex": "europa.jpg", "scale": 0.12, "dist": 3.2, "orb": 7.2},
                {"nome": "Calisto", "tex": "europa.jpg", "scale": 0.1, "dist": 4.0, "orb": 16.7}
            ]
        },
        {
            "nome": "Saturno", "tex": "saturno.jpg", "scale": 1.0, "dist": 40, "orb": 750, "rot": 9, "tilt": 25,
            "ring": {"file": "anel_saturno.3d", "tex": "anel_saturno.jpg"},
            "moons": [
                {"nome": "Tita", "tex": "lua.jpg", "scale": 0.15, "dist": 3.0, "orb": 16},
                {"nome": "Encelado", "tex": "lua.jpg", "scale": 0.04, "dist": 1.6, "orb": 1.4},
                {"nome": "Reia", "tex": "lua.jpg", "scale": 0.07, "dist": 2.2, "orb": 4.5}
            ]
        },
        {
            "nome": "Urano", "tex": "urano.jpg", "scale": 0.7, "dist": 50, "orb": 1500, "rot": 14, "tilt": 98,
            "ring": {"file": "anel_urano.3d", "tex": "anel_urano.jpg"},
            "moons": [
                {"nome": "Titania", "tex": "lua.jpg", "scale": 0.06, "dist": 1.8, "orb": 8.7},
                {"nome": "Oberon", "tex": "lua.jpg", "scale": 0.05, "dist": 2.4, "orb": 13.5}
            ]
        },
        {
            "nome": "Neptuno", "tex": "neptuno.jpg", "scale": 0.65, "dist": 60, "orb": 3000, "rot": 13,
            "moons": [ {"nome": "Tritao", "tex": "lua.jpg", "scale": 0.07, "dist": 1.5, "orb": 5.8} ]
        }
    ]


def catmull_rom_points(radius, y_offset=0.0):
    """Gera 8 pontos para uma órbita circular via Catmull-Rom"""
    points = []
    for i in range(8):
        angle = 2 * math.pi * i / 8
        x = round(radius * math.cos(angle), 3)
        z = round(radius * math.sin(angle), 3)
        points.append(f'                    <point x="{x}" y="{y_offset}" z="{z}" />')
    return "\n".join(points)

def write_astro(p):
    xml = f"        \n"
    xml += "        <group>\n"
    
    # 1. Órbita em redor do Sol
    if p['dist'] > 0:
        xml += "            <transform>\n"
        xml += f'                <translate time="{p["orb"]}">\n'
        xml += catmull_rom_points(p['dist']) + "\n"
        xml += "                </translate>\n"
        xml += "            </transform>\n"
        
    # 2. Grupo de Inclinação (Afeta Planeta e Anéis)
    xml += "            <group>\n"
    if 'tilt' in p and p['tilt'] != 0:
        xml += "                <transform>\n"
        xml += f'                    <rotate angle="{p["tilt"]}" x="0" y="0" z="1" />\n'
        xml += "                </transform>\n"
        
    # 3. O Planeta (Rotação própria + Escala)
    xml += "                <group>\n"
    xml += "                    <transform>\n"
    if p['rot'] > 0:
        xml += f'                        <rotate time="{p["rot"]}" x="0" y="1" z="0" />\n'
        
    s = p['scale']
    if isinstance(s, list): # Suporte para a escala "Batata" (Irregular)
        xml += f'                        <scale x="{s[0]}" y="{s[1]}" z="{s[2]}" />\n'
    else:
        xml += f'                        <scale x="{s}" y="{s}" z="{s}" />\n'
    xml += "                    </transform>\n"
    
    xml += "                    <models>\n"
    xml += f'                        <model file="esfera.3d" name="{p["nome"]}">\n'
    xml += f'                            <texture file="{p["tex"]}" />\n'
    xml += "                            <color>\n"
    
    # Extração das cores corrigida (sem conflitos de aspas)
    c = p.get('colors', {})
    diff = c.get("diff", '200" G="200" B="200')
    amb = c.get("amb", '50" G="50" B="50')
    spec = c.get("spec", '10" G="10" B="10')
    emi = c.get("emi", '0" G="0" B="0')
    shin = c.get("shin", '5')

    xml += f'                                <diffuse R="{diff}" />\n'
    xml += f'                                <ambient R="{amb}" />\n'
    xml += f'                                <specular R="{spec}" />\n'
    xml += f'                                <emissive R="{emi}" />\n'
    xml += f'                                <shininess value="{shin}" />\n'
    
    xml += "                            </color>\n"
    xml += "                        </model>\n"
    xml += "                    </models>\n"
    xml += "                </group>\n"
    
    # 4. Anéis (Dentro da inclinação, fora da rotação própria)
    if 'ring' in p:
        r = p['ring']
        xml += f"                \n"
        xml += "                <group>\n"
        xml += "                    <transform>\n"
        # A escala do anel segue a escala do planeta (mas achatado no Y)
        xml += f'                        <scale x="{s}" y="0.005" z="{s}" />\n'
        xml += "                    </transform>\n"
        xml += "                    <models>\n"
        xml += f'                        <model file="{r["file"]}" name="Aneis {p["nome"]}" type="ring">\n'
        xml += f'                            <texture file="{r["tex"]}" />\n'
        xml += "                            <color>\n"
        xml += '                                <diffuse R="200" G="200" B="255" />\n'
        xml += '                                <ambient R="50" G="50" B="50" />\n'
        xml += "                            </color>\n"
        xml += "                        </model>\n"
        xml += "                    </models>\n"
        xml += "                </group>\n"
        
    xml += "            </group>\n" # Fim da Inclinação
    
    # 5. Luas
    for m in p.get('moons', []):
        xml += write_moon(m)
        
    xml += "        </group>\n\n"
    return xml

def write_moon(m):
    xml = f"            \n"
    xml += "            <group>\n"
    xml += "                <transform>\n"
    xml += f'                    <translate time="{m["orb"]}">\n'
    xml += catmull_rom_points(m['dist']) + "\n"
    xml += "                    </translate>\n"
    s = m['scale']
    if isinstance(s, list):
        xml += f'                    <scale x="{s[0]}" y="{s[1]}" z="{s[2]}" />\n'
    else:
        xml += f'                    <scale x="{s}" y="{s}" z="{s}" />\n'
    xml += "                </transform>\n"
    xml += "                <models>\n"
    xml += f'                    <model file="esfera.3d" name="{m["nome"]}">\n'
    xml += f'                        <texture file="{m["tex"]}" />\n'
    xml += "                        <color>\n"
    xml += '                            <diffuse R="150" G="150" B="150" />\n'
    xml += '                            <ambient R="50" G="50" B="50" />\n'
    xml += '                            <specular R="0" G="0" B="0" />\n'
    xml += '                            <emissive R="0" G="0" B="0" />\n'
    xml += '                            <shininess value="0" />\n'
    xml += "                        </color>\n"
    xml += "                    </model>\n"
    xml += "                </models>\n"
    xml += "            </group>\n"
    return xml

def write_asteroids():
    xml = "        \n        <group>\n"
    xml += '            <transform><rotate time="2000" x="0" y="1" z="0" /></transform>\n'
    # 250 asteroides garantem impacto visual sem destruir o framerate
    for _ in range(250):
        dist = random.uniform(22, 26) # Asteroides espremidos entre Marte e Júpiter
        y = random.uniform(-0.8, 0.8)
        angle = random.uniform(0, 360)
        s = random.uniform(0.01, 0.03)
        rot = random.uniform(0, 360)
        xml += "            <group>\n"
        xml += "                <transform>\n"
        xml += f'                    <rotate angle="{angle:.1f}" x="0" y="1" z="0" />\n'
        xml += f'                    <translate x="{dist:.2f}" y="{y:.2f}" z="0" />\n'
        xml += f'                    <rotate angle="{rot:.1f}" x="1" y="1" z="1" />\n'
        xml += f'                    <scale x="{s:.3f}" y="{s:.3f}" z="{s:.3f}" />\n'
        xml += "                </transform>\n"
        xml += '                <models><model file="teapot.3d">\n'
        xml += '                    <color><diffuse R="120" G="120" B="120" /><ambient R="30" G="30" B="30" /></color>\n'
        xml += '                </model></models>\n'
        xml += "            </group>\n"
    xml += "        </group>\n\n"
    return xml

def write_comet():
    return """        <group>
            <transform>
                <translate time="200" align="True">
                    <point x="0" y="15" z="60" />
                    <point x="30" y="10" z="40" />
                    <point x="45" y="5" z="0" />
                    <point x="30" y="-2" z="-40" />
                    <point x="0" y="-10" z="-60" />
                    <point x="-30" y="-5" z="-40" />
                    <point x="-45" y="0" z="0" />
                    <point x="-30" y="7" z="40" />
                </translate>
                <scale x="0.15" y="0.15" z="0.15" />
            </transform>
            <group>
                <transform><rotate angle="0" time="5" x="1" y="1" z="0" /></transform>
                <models>
                    <model file="teapot.3d" name="Cometa Halley">
                        <color>
                            <diffuse R="200" G="200" B="255" />
                            <ambient R="100" G="100" B="150" />
                            <specular R="255" G="255" B="255" />
                            <shininess value="100" />
                        </color>
                    </model>
                </models>
            </group>
        </group>
"""

def gerar_sistema_solar():
    print("🚀 A gerar o Universo (Escala Melhorada)...")

    xml = "<world>\n"
    xml += '    <window width="1280" height="720" />\n'
    xml += '    <camera>\n'
    # Camara ajustada para nascer mais perto do Sol, olhando para o sistema inteiro!
    xml += '        <position x="0" y="15" z="35" />\n'
    xml += '        <projection fov="60" near="1" far="4000" />\n'
    xml += '    </camera>\n\n'
    xml += '    <lights>\n'
    xml += '        <light type="POINT" posx="0" posy="0" posz="0" />\n'
    xml += '    </lights>\n\n'
    xml += '    <group>\n'
    
    for p in PLANETS:
        xml += write_astro(p)
        
    xml += write_asteroids()
    xml += write_comet()
    
    xml += '    </group>\n'
    xml += '</world>\n'

    with open("sistema_solar.xml", "w", encoding="utf-8") as f:
        f.write(xml)

    print("\n✅ XML Gerado com Sucesso: 'sistema_solar.xml'")

def print_generator_commands():
    print("\n📦 Comandos para o Generator:")
    print("-" * 50)
    
    # Primitivas base
    print(".\\generator.exe sphere 1 20 20 esfera.3d")
    print(".\\generator.exe patch teapot.patch 10 teapot.3d")
    
    # Anéis — lê os planetas com 'ring' e extrai os ficheiros
    for p in PLANETS:
        if 'ring' in p:
            ring_file = p['ring']['file']
            # Saturno: annulus 1.8 3.5, Urano: annulus 2.1 2.8
            if "saturno" in ring_file:
                print(f".\\generator.exe annulus 1.8 3.5 128 {ring_file}")
            elif "urano" in ring_file:
                print(f".\\generator.exe annulus 2.1 2.8 128 {ring_file}")
    
    print("-" * 50)

if __name__ == "__main__":
    gerar_sistema_solar()
    print_generator_commands()