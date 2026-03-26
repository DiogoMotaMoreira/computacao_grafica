import random
import math

def gerar_sistema_solar():
    print("🚀 A iniciar o Gerador do Sistema Solar...")
    
    planetas = [
        {"nome": "Mercurio", "dist_sol": 57.9, "raio": 2439.7, "luas": [], "aneis": None},
        {"nome": "Venus", "dist_sol": 108.2, "raio": 6051.8, "luas": [], "aneis": None},
        {"nome": "Terra", "dist_sol": 149.6, "raio": 6371.0, "aneis": None, "luas": [
            {"nome": "Lua", "dist": 0.38, "raio": 1737.4}
        ]},
        {"nome": "Marte", "dist_sol": 227.9, "raio": 3389.5, "aneis": None, "luas": [
            {"nome": "Fobos", "dist": 0.009, "raio": 11.2},
            {"nome": "Deimos", "dist": 0.023, "raio": 6.2}
        ]},
        {"nome": "Jupiter", "dist_sol": 778.5, "raio": 69911.0, "aneis": None, "luas": [
            {"nome": "Io", "dist": 0.42, "raio": 1821.6},
            {"nome": "Europa", "dist": 0.67, "raio": 1560.8},
            {"nome": "Ganimedes", "dist": 1.07, "raio": 2634.1}
        ]},
        {"nome": "Saturno", "dist_sol": 1432.0, "raio": 58232.0, "aneis": {"in": 0.3, "out": 2.5, "tilt": 25}, "luas": [
            {"nome": "Tita", "dist": 1.22, "raio": 2574.7}
        ]},
        {"nome": "Urano", "dist_sol": 2867.0, "raio": 25362.0, "aneis": {"in": 0.1, "out": 1.5, "tilt": 85}, "luas": []},
        {"nome": "Neptuno", "dist_sol": 4515.0, "raio": 24622.0, "luas": [], "aneis": None}
    ]

    terra_raio = 6371.0
    terra_dist = 149.6
    lua_raio = 1737.4
    lua_dist = 0.38
    DIST_MULT = 15.0 
    SIZE_MULT = 0.4  

    comandos_cmd = set()
    comandos_cmd.add(".\\generator.exe sphere 1 30 30 esfera.3d")

    targets = [{"nome": "Sol", "x": 0.0, "y": 0.0, "z": 0.0, "dist": 0.0, "radius": 4.0}]

    xml_grupos = "" 

    for i, p in enumerate(planetas):
        nome = p['nome']
        dist_vis = round(((p['dist_sol'] / terra_dist) ** 0.6) * DIST_MULT)
        if dist_vis <= 5: dist_vis = 6
        size_vis = ((p['raio'] / terra_raio) ** 0.55) * SIZE_MULT
        
        angulo_orbita = (i * 45 + dist_vis * 7) % 360 

        rad_p = math.radians(angulo_orbita)
        px = dist_vis * math.cos(rad_p)
        pz = -dist_vis * math.sin(rad_p)
        
        # Guardamos a coordenada absoluta E o tamanho do astro
        targets.append({"nome": nome, "x": px, "y": 0.0, "z": pz, "dist": dist_vis, "radius": size_vis})

        comandos_cmd.add(f".\\generator.exe orbit {dist_vis} 150 orbita_{dist_vis}.3d")

        xml_grupos += f'        \n'
        xml_grupos += '        <group>\n'
        xml_grupos += f'            <models><model file="orbita_{dist_vis}.3d" type="line" /></models>\n'
        xml_grupos += '            <group>\n'
        xml_grupos += '                <transform>\n'
        xml_grupos += f'                    <rotate angle="{angulo_orbita}" x="0" y="1" z="0" />\n'
        xml_grupos += f'                    <translate x="{dist_vis}" y="0" z="0" />\n'
        xml_grupos += '                </transform>\n'

        if p['aneis']:
            tilt = p['aneis']['tilt']
            r_in = p['aneis']['in']
            r_out = p['aneis']['out']
            anel_file = f"anel_{nome.lower()}.3d"
            comandos_cmd.add(f".\\generator.exe torus {r_in} {r_out} 15 40 {anel_file}")
            
            xml_grupos += '                <group>\n'
            xml_grupos += f'                    <transform><rotate angle="{tilt}" x="0" y="0" z="1" /></transform>\n'
            xml_grupos += f'                    <group><transform><scale x="{size_vis:.3f}" y="{size_vis:.3f}" z="{size_vis:.3f}" /></transform><models><model file="esfera.3d" /></models></group>\n'
            xml_grupos += f'                    <group><transform><scale x="1.0" y="0.05" z="1.0" /></transform><models><model file="{anel_file}" /></models></group>\n'
            xml_grupos += '                </group>\n'
        else:
            xml_grupos += f'                <group><transform><scale x="{size_vis:.3f}" y="{size_vis:.3f}" z="{size_vis:.3f}" /></transform><models><model file="esfera.3d" /></models></group>\n'

        for lua in p['luas']:
            nome_lua = lua['nome']
            dist_lua_vis = round(((lua['dist'] / lua_dist) ** 0.4) * 2.0, 1)
            if dist_lua_vis < size_vis + 0.5: dist_lua_vis = round(size_vis + 0.8, 1)
            size_lua_vis = ((lua['raio'] / lua_raio) ** 0.5) * 0.1
            
            angulo_lua = hash(nome_lua) % 360
            
            rad_m = math.radians(angulo_lua)
            mx_local = dist_lua_vis * math.cos(rad_m)
            mz_local = -dist_lua_vis * math.sin(rad_m)
            mx_pre = mx_local + dist_vis
            mz_pre = mz_local
            mx_final = mx_pre * math.cos(rad_p) + mz_pre * math.sin(rad_p)
            mz_final = -mx_pre * math.sin(rad_p) + mz_pre * math.cos(rad_p)
            
            dist_sun = math.sqrt(mx_final**2 + mz_final**2)
            targets.append({"nome": nome_lua, "x": mx_final, "y": 0.0, "z": mz_final, "dist": dist_sun, "radius": size_lua_vis})

            comandos_cmd.add(f".\\generator.exe orbit {dist_lua_vis} 60 orbita_lua_{dist_lua_vis}.3d")

            xml_grupos += '                <group>\n'
            xml_grupos += f'                    <models><model file="orbita_lua_{dist_lua_vis}.3d" type="line" /></models>\n'
            xml_grupos += '                    <group>\n'
            xml_grupos += '                        <transform>\n'
            xml_grupos += f'                            <rotate angle="{angulo_lua}" x="0" y="1" z="0" />\n'
            xml_grupos += f'                            <translate x="{dist_lua_vis}" y="0" z="0" />\n'
            xml_grupos += f'                            <scale x="{size_lua_vis:.3f}" y="{size_lua_vis:.3f}" z="{size_lua_vis:.3f}" />\n'
            xml_grupos += '                        </transform>\n'
            xml_grupos += '                        <models><model file="esfera.3d" /></models>\n'
            xml_grupos += '                    </group>\n'
            xml_grupos += '                </group>\n'

        xml_grupos += '            </group>\n'
        xml_grupos += '        </group>\n\n'

    targets = sorted(targets, key=lambda t: t['dist'])

    xml = "<world>\n"
    xml += '    <window width="1024" height="768" />\n'
    xml += '    <camera>\n'
    xml += '        <position x="0" y="80" z="130" />\n'
    xml += '        <lookAt x="0" y="0" z="0" />\n'
    xml += '        <up x="0" y="1" z="0" />\n'
    xml += '        <projection fov="60" near="0.1" far="2000" />\n'
    xml += '        \n'
    xml += '        <waypoints>\n'
    for t in targets:
        xml += f'            <target name="{t["nome"]}" x="{t["x"]:.2f}" y="{t["y"]:.2f}" z="{t["z"]:.2f}" radius="{t["radius"]:.3f}" />\n'
    xml += '        </waypoints>\n'
    xml += '    </camera>\n\n'
    xml += '    <group>\n'
    
    xml += '        <group>\n'
    xml += '            <transform><scale x="4.0" y="4.0" z="4.0" /></transform>\n'
    xml += '            <models><model file="esfera.3d" /></models>\n'
    xml += '        </group>\n\n'

    xml += xml_grupos 

    xml += '        <group>\n'
    comandos_cmd.add(".\\generator.exe sphere 1 4 4 asteroide.3d")
    for i in range(800):
        r = random.uniform(23.0, 35.0) 
        angle = random.uniform(0.0, 360.0)
        y_offset = random.uniform(-0.6, 0.6)
        scale = random.uniform(0.01, 0.04)
        xml += '            <group>\n'
        xml += '                <transform>\n'
        xml += f'                    <rotate angle="{angle:.1f}" x="0" y="1" z="0" />\n'
        xml += f'                    <translate x="{r:.2f}" y="{y_offset:.2f}" z="0" />\n'
        xml += f'                    <scale x="{scale:.3f}" y="{scale:.3f}" z="{scale:.3f}" />\n'
        xml += '                </transform>\n'
        xml += '                <models><model file="asteroide.3d" /></models>\n' 
        xml += '            </group>\n'
    xml += '        </group>\n\n'

    xml += '    </group>\n'
    xml += '</world>\n'

    nome_ficheiro = "sistema_solar.xml"
    with open(nome_ficheiro, "w", encoding="utf-8") as f:
        f.write(xml)

    print(f"\n✅ SUCESSO! Ficheiro '{nome_ficheiro}' gerado!")
    for cmd in sorted(comandos_cmd):
        print(cmd)

if __name__ == "__main__":
    gerar_sistema_solar()