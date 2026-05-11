import random
import math

def gerar_sistema_solar():
    print("🚀 A iniciar o Gerador do Sistema Solar...")
    
    planetas = [
        {"nome": "Mercurio", "dist_sol": 57.9, "raio": 2439.7, "tempo_orbita": 8.8, "tempo_rotacao": 50, "luas": [], "aneis": None},
        {"nome": "Venus", "dist_sol": 108.2, "raio": 6051.8, "tempo_orbita": 22.5, "tempo_rotacao": 120, "luas": [], "aneis": None},
        {"nome": "Terra", "dist_sol": 149.6, "raio": 6371.0, "tempo_orbita": 36.5, "tempo_rotacao": 1, "aneis": None, "luas": [
            {"nome": "Lua", "dist": 0.38, "raio": 1737.4, "tempo_orbita": 2.7}
        ]},
        {"nome": "Marte", "dist_sol": 227.9, "raio": 3389.5, "tempo_orbita": 68.7, "tempo_rotacao": 1.03, "aneis": None, "luas": [
            {"nome": "Fobos", "dist": 0.009, "raio": 11.2, "tempo_orbita": 0.8},
            {"nome": "Deimos", "dist": 0.023, "raio": 6.2, "tempo_orbita": 1.5}
        ]},
        {"nome": "Jupiter", "dist_sol": 778.5, "raio": 69911.0, "tempo_orbita": 438, "tempo_rotacao": 0.4, "aneis": None, "luas": [
            {"nome": "Io", "dist": 0.42, "raio": 1821.6, "tempo_orbita": 1.8},
            {"nome": "Europa", "dist": 0.67, "raio": 1560.8, "tempo_orbita": 3.6},
            {"nome": "Ganimedes", "dist": 1.07, "raio": 2634.1, "tempo_orbita": 7.2}
        ]},
        {"nome": "Saturno", "dist_sol": 1432.0, "raio": 58232.0, "tempo_orbita": 1058, "tempo_rotacao": 0.45, "aneis": {"in": 0.3, "out": 2.5, "tilt": 25}, "luas": [
            {"nome": "Tita", "dist": 1.22, "raio": 2574.7, "tempo_orbita": 16}
        ]},
        {"nome": "Urano", "dist_sol": 2867.0, "raio": 25362.0, "tempo_orbita": 3066, "tempo_rotacao": 0.7, "aneis": {"in": 0.1, "out": 1.5, "tilt": 85}, "luas": []},
        {"nome": "Neptuno", "dist_sol": 4515.0, "raio": 24622.0, "tempo_orbita": 6022, "tempo_rotacao": 0.67, "luas": [], "aneis": None}
    ]

    terra_raio = 6371.0
    terra_dist = 149.6
    lua_raio = 1737.4
    lua_dist = 0.38
    DIST_MULT = 15.0 
    SIZE_MULT = 0.4  

    comandos_cmd = set()
    comandos_cmd.add(".\\generator.exe sphere 1 30 30 esfera.3d")
    comandos_cmd.add(".\\generator.exe patch teapot.patch 10 teapot.3d")

    targets = [{"nome": "Sol", "x": 0.0, "y": 0.0, "z": 0.0, "dist": 0.0, "radius": 4.0}]

    xml_grupos = "" 

    for i, p in enumerate(planetas):
        nome = p['nome']
        dist_vis = round(((p['dist_sol'] / terra_dist) ** 0.6) * DIST_MULT)
        if dist_vis <= 5: dist_vis = 6
        size_vis = ((p['raio'] / terra_raio) ** 0.55) * SIZE_MULT
        
        # Ponto inicial para manter a posição absoluta do alvo (Waypoint)
        angulo_orbita = (i * 45 + dist_vis * 7) % 360 
        rad_p = math.radians(angulo_orbita)
        px = dist_vis * math.cos(rad_p)
        pz = -dist_vis * math.sin(rad_p)
        targets.append({"nome": nome, "x": px, "y": 0.0, "z": pz, "dist": dist_vis, "radius": size_vis})

        # Geração dos 8 pontos da curva circular
        pts_orbita = []
        for j in range(8):
            ang = math.radians(j * 45)
            x_pt = round(dist_vis * math.cos(ang), 2)
            z_pt = round(dist_vis * math.sin(ang), 2)
            pts_orbita.append((x_pt, 0.0, z_pt))

        xml_grupos += f'\n        <!-- {nome} -->\n'
        xml_grupos += '        <group>\n'
        xml_grupos += '            <transform>\n'
        xml_grupos += f'                <translate time="{p["tempo_orbita"]}">\n'
        for pt in pts_orbita:
            xml_grupos += f'                    <point x="{pt[0]}" y="{pt[1]}" z="{pt[2]}" />\n'
        xml_grupos += '                </translate>\n'
        xml_grupos += '            </transform>\n'

        if p['aneis']:
            tilt = p['aneis']['tilt']
            r_in = p['aneis']['in']
            r_out = p['aneis']['out']
            anel_file = f"anel_{nome.lower()}.3d"
            comandos_cmd.add(f".\\generator.exe torus {r_in} {r_out} 15 40 {anel_file}")
            
            xml_grupos += '            <group>\n'
            xml_grupos += f'                <transform><rotate angle="{tilt}" x="0" y="0" z="1" /></transform>\n'
            xml_grupos += '                <group>\n'
            xml_grupos += f'                    <transform><rotate time="{p["tempo_rotacao"]}" x="0" y="1" z="0" /></transform>\n'
            xml_grupos += '                    <group>\n'
            xml_grupos += f'                        <transform><scale x="{size_vis:.3f}" y="{size_vis:.3f}" z="{size_vis:.3f}" /></transform>\n'
            xml_grupos += '                        <models><model file="esfera.3d" /></models>\n'
            xml_grupos += '                    </group>\n'
            xml_grupos += '                    <group>\n'
            xml_grupos += '                        <transform><scale x="1.0" y="0.05" z="1.0" /></transform>\n'
            xml_grupos += f'                        <models><model file="{anel_file}" /></models>\n'
            xml_grupos += '                    </group>\n'
            xml_grupos += '                </group>\n'
            xml_grupos += '            </group>\n'
        else:
            xml_grupos += '            <group>\n'
            xml_grupos += '                <transform>\n'
            xml_grupos += f'                    <rotate time="{p["tempo_rotacao"]}" x="0" y="1" z="0" />\n'
            xml_grupos += f'                    <scale x="{size_vis:.3f}" y="{size_vis:.3f}" z="{size_vis:.3f}" />\n'
            xml_grupos += '                </transform>\n'
            xml_grupos += '                <models><model file="esfera.3d" /></models>\n'
            xml_grupos += '            </group>\n'

        for lua in p['luas']:
            nome_lua = lua['nome']
            dist_lua_vis = round(((lua['dist'] / lua_dist) ** 0.4) * 2.0, 1)
            if dist_lua_vis < size_vis + 0.5: dist_lua_vis = round(size_vis + 0.8, 1)
            size_lua_vis = ((lua['raio'] / lua_raio) ** 0.5) * 0.1
            
            # Atualização do alvo da Lua (aproximado)
            angulo_lua = hash(nome_lua) % 360
            rad_m = math.radians(angulo_lua)
            mx_local = dist_lua_vis * math.cos(rad_m)
            mz_local = -dist_lua_vis * math.sin(rad_m)
            mx_final = px + mx_local
            mz_final = pz + mz_local
            dist_sun = math.sqrt(mx_final**2 + mz_final**2)
            targets.append({"nome": nome_lua, "x": mx_final, "y": 0.0, "z": mz_final, "dist": dist_sun, "radius": size_lua_vis})

            pts_lua = []
            for j in range(8):
                ang = math.radians(j * 45)
                x_pt = round(dist_lua_vis * math.cos(ang), 2)
                z_pt = round(dist_lua_vis * math.sin(ang), 2)
                pts_lua.append((x_pt, 0.0, z_pt))

            xml_grupos += '            <group>\n'
            xml_grupos += '                <transform>\n'
            xml_grupos += f'                    <translate time="{lua["tempo_orbita"]}">\n'
            for pt in pts_lua:
                xml_grupos += f'                        <point x="{pt[0]}" y="{pt[1]}" z="{pt[2]}" />\n'
            xml_grupos += '                    </translate>\n'
            xml_grupos += f'                    <scale x="{size_lua_vis:.3f}" y="{size_lua_vis:.3f}" z="{size_lua_vis:.3f}" />\n'
            xml_grupos += '                </transform>\n'
            xml_grupos += '                <models><model file="esfera.3d" /></models>\n'
            xml_grupos += '            </group>\n'

        xml_grupos += '        </group>\n'

    targets = sorted(targets, key=lambda t: t['dist'])

    xml = "<world>\n"
    xml += '    <window width="1024" height="768" />\n'
    xml += '    <camera>\n'
    xml += '        <position x="0" y="160" z="350" />\n'
    xml += '        <lookAt x="0" y="0" z="0" />\n'
    xml += '        <up x="0" y="1" z="0" />\n'
    xml += '        <projection fov="60" near="1" far="4000" />\n'
    xml += '        \n'
    xml += '        <waypoints>\n'
    for t in targets:
        xml += f'            <target name="{t["nome"]}" x="{t["x"]:.2f}" y="{t["y"]:.2f}" z="{t["z"]:.2f}" radius="{t["radius"]:.3f}" />\n'
    xml += '        </waypoints>\n'
    xml += '    </camera>\n\n'
    xml += '    <group>\n'
    
    xml += '        <group>\n'
    xml += '            <transform>\n'
    xml += '                <rotate time="25" x="0" y="1" z="0" />\n'
    xml += '                <scale x="5.0" y="5.0" z="5.0" />\n'
    xml += '            </transform>\n'
    xml += '            <models><model file="esfera.3d" /></models>\n'
    xml += '        </group>\n'

    xml += xml_grupos 

    xml += '\n        <!-- Cintura de Asteroides -->\n'
    xml += '        <group>\n'
    xml += '            <transform>\n'
    xml += '                <rotate time="150" x="0" y="1" z="0" />\n'
    xml += '            </transform>\n'
    
    for i in range(10): # Reduzi de 800 para 10 como no exemplo XML final para performance/simplicidade do teapot
        r = random.uniform(24.0, 36.0) 
        angle = random.uniform(0.0, 360.0)
        y_offset = random.uniform(-0.8, 0.8)
        scale = random.uniform(0.03, 0.07)
        xml += '            <group>\n'
        xml += '                <transform>\n'
        xml += f'                    <translate time="0"><point x="{round(r * math.cos(math.radians(angle)), 1)}" y="{round(y_offset, 1)}" z="{round(r * math.sin(math.radians(angle)), 1)}"/></translate>\n'
        xml += f'                    <scale x="{scale:.3f}" y="{scale:.3f}" z="{scale:.3f}" />\n'
        xml += f'                    <rotate angle="{random.randint(0, 360)}" x="{random.randint(0,1)}" y="{random.randint(0,1)}" z="{random.randint(0,1)}" />\n'
        xml += '                </transform>\n'
        xml += '                <models><model file="teapot.3d" /></models>\n' 
        xml += '            </group>\n'
    xml += '        </group>\n'

    xml += '\n        <!-- Cometa Halley (exemplo de órbita elíptica alargada) -->\n'
    xml += '        <group>\n'
    xml += '            <transform>\n'
    xml += '                <translate time="200" align="True"> \n'
    xml += '                    <point x="0" y="30" z="160" />\n'
    xml += '                    <point x="84" y="20" z="112" />\n'
    xml += '                    <point x="120" y="10" z="0" />\n'
    xml += '                    <point x="84" y="-5" z="-112" />\n'
    xml += '                    <point x="0" y="-20" z="-160" />\n'
    xml += '                    <point x="-84" y="-10" z="-112" />\n'
    xml += '                    <point x="-120" y="0" z="0" />\n'
    xml += '                    <point x="-84" y="15" z="112" />\n'
    xml += '                </translate>\n'
    xml += '                <scale x="0.3" y="0.3" z="0.3" />\n'
    xml += '            </transform>\n'
    xml += '            <group>\n'
    xml += '                <transform> <rotate angle="0" x="1" y="1" z="0" /> </transform>\n'
    xml += '                <models>\n'
    xml += '                    <model file="teapot.3d" />\n'
    xml += '                </models>\n'
    xml += '            </group>\n'
    xml += '        </group>\n'

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