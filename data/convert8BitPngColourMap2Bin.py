#!/usr/bin/env python3
"""
Convierte un PNG de 8 bits indexado (1024x1024) a:
- binario con los índices (1024x1024 bytes)
- header con la paleta en RGB565 y RGB888

Uso: python convert8BitPngColourMap2Bin.py imagen.png
"""

from PIL import Image
import sys
import os
import struct

def rgb888_to_rgb565(r, g, b):
    """Convierte RGB888 a RGB565 (16 bits)"""
    r5 = (r >> 3) & 0x1F
    g6 = (g >> 2) & 0x3F
    b5 = (b >> 3) & 0x1F
    return (r5 << 11) | (g6 << 5) | b5

def generate_palette_header(palette, output_file, array_name="paleta_colores"):
    """
    Genera archivo header con la paleta en RGB565 y RGB888
    """
    num_colors = len(palette)
    
    with open(output_file, 'w') as f:
        f.write("""// Auto-generated palette from PNG image
#ifndef PALETA_COLORES_H
#define PALETA_COLORES_H

#include <stdint.h>

/** 
 * Paleta en formato RGB565 (16 bits)
 * Util para sistemas embebidos con poca memoria
 */
static const uint16_t paleta_rgb565[] = {
""")
        
        # Escribir paleta RGB565 (16 bits)
        for i, (r, g, b) in enumerate(palette):
            rgb565 = rgb888_to_rgb565(r, g, b)
            if i % 8 == 0:
                f.write("    ")
            f.write(f"0x{rgb565:04X}")
            if i < len(palette) - 1:
                f.write(", ")
            if (i + 1) % 8 == 0:
                f.write("\n")
        
        f.write("""};

/** 
 * Paleta en formato RGB888 (24 bits)
 * Util para depuracion y referencia
 */
static const struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} paleta_rgb888[] = {
""")
        
        # Escribir paleta RGB888 como structs
        for i, (r, g, b) in enumerate(palette):
            if i % 4 == 0:
                f.write("    ")
            f.write(f"{{{r:3d}, {g:3d}, {b:3d}}}")
            if i < len(palette) - 1:
                f.write(", ")
            if (i + 1) % 4 == 0:
                f.write("\n")
        
        f.write("""};

/**
 * Version en array plano RGB888 (para compatibilidad)
 */
static const uint8_t paleta_rgb888_flat[] = {
""")
        
        # Escribir paleta RGB888 como array plano
        for i, (r, g, b) in enumerate(palette):
            if i % 4 == 0:
                f.write("    ")
            f.write(f"{r:3d}, {g:3d}, {b:3d}")
            if i < len(palette) - 1:
                f.write(", ")
            if (i + 1) % 4 == 0:
                f.write("\n")
        
        # IMPORTANTE: Escapar las llaves usando dobles llaves {{ y }}
        f.write(f"""}};

/** 
 * Constantes utiles
 */
#define PALETA_COLORES_TOTAL {num_colors}
#define PALETA_COLORES_BYTES {num_colors}
#define IMAGEN_ANCHO 1024
#define IMAGEN_ALTO 1024
#define IMAGEN_TOTAL_PIXELES 1048576

#endif // PALETA_COLORES_H
""")

def convert_indexed_png(input_png):
    """
    Funcion principal: procesa PNG indexado y genera archivos
    """
    print(f"Procesando: {input_png}")
    
    # Cargar imagen
    try:
        img = Image.open(input_png)
    except Exception as e:
        print(f"Error: No se pudo abrir {input_png}: {e}")
        return False
    
    print(f"  Modo: {img.mode}")
    print(f"  Tamano: {img.size}")
    
    # Verificar que sea indexada o escala de grises
    if img.mode not in ['P', 'L']:
        print(f"Error: La imagen no es indexada (P) ni escala de grises (L)")
        print(f"  Modo actual: {img.mode}")
        print("  Convierte la imagen a indexada primero.")
        return False
    
    # Verificar dimensiones
    width, height = img.size
    if width != 1024 or height != 1024:
        print(f"Advertencia: La imagen no es 1024x1024 ({width}x{height})")
    
    # Obtener datos indexados
    if img.mode == 'P':
        # Imagen indexada con paleta
        palette = img.getpalette()
        if palette is None:
            print("Error: La imagen indexada no tiene paleta")
            return False
        
        # La paleta es una lista de 768 valores (256 colores * 3)
        palette_rgb = [(palette[i], palette[i+1], palette[i+2]) 
                       for i in range(0, len(palette), 3)]
        
        # Si la paleta tiene mas de 256 colores, truncar
        if len(palette_rgb) > 256:
            print(f"Advertencia: La paleta tiene {len(palette_rgb)} colores, truncando a 256")
            palette_rgb = palette_rgb[:256]
        
        # Datos de pixeles (indices)
        pixel_data = img.tobytes()
        
    else:  # 'L' - Escala de grises
        # Convertir a paleta artificial
        print("  Imagen en escala de grises, creando paleta artificial...")
        palette_rgb = [(i, i, i) for i in range(256)]
        pixel_data = img.tobytes()
    
    # Verificar tamaño de datos
    expected_size = width * height
    actual_size = len(pixel_data)
    
    if actual_size != expected_size:
        print(f"Error: Tamano de datos incorrecto")
        print(f"  Esperado: {expected_size}")
        print(f"  Actual: {actual_size}")
        return False
    
    # Estadisticas
    unique_indices = set(pixel_data)
    num_colors_used = len(unique_indices)
    
    print(f"  Pixeles: {actual_size}")
    print(f"  Colores en paleta: {len(palette_rgb)}")
    print(f"  Colores usados: {num_colors_used}")
    print(f"  Indices: min={min(pixel_data)}, max={max(pixel_data)}")
    
    # Generar nombres de archivo
    base_name = os.path.splitext(input_png)[0]
    output_bin = f"{base_name}_indexed.bin"
    output_header = f"paleta_{os.path.basename(base_name)}.h"
    
    # Guardar datos indexados como binario
    print(f"\nGuardando datos indexados...")
    with open(output_bin, 'wb') as f:
        f.write(pixel_data)
    
    print(f"  ✓ {output_bin} ({actual_size} bytes)")
    
    # Generar header con la paleta
    print(f"Generando header de paleta...")
    generate_palette_header(palette_rgb, output_header)
    print(f"  ✓ {output_header}")
    
    # Guardar paleta como binario (opcional, para uso directo)
    output_palette_bin = f"{base_name}_paleta.bin"
    with open(output_palette_bin, 'wb') as f:
        # Guardar RGB565 (16 bits) en little endian
        for r, g, b in palette_rgb:
            rgb565 = rgb888_to_rgb565(r, g, b)
            f.write(struct.pack('<H', rgb565))
    
    print(f"  ✓ {output_palette_bin} ({len(palette_rgb) * 2} bytes)")
    
    # Mostrar resumen
    print("\n" + "="*60)
    print("RESUMEN DE CONVERSION")
    print("="*60)
    print(f"Archivos generados:")
    print(f"  - {output_bin} (datos indexados, {actual_size} bytes)")
    print(f"  - {output_header} (paleta RGB565 y RGB888)")
    print(f"  - {output_palette_bin} (paleta RGB565 binaria, {len(palette_rgb) * 2} bytes)")
    print(f"\nEstadisticas:")
    print(f"  - Colores en paleta: {len(palette_rgb)}")
    print(f"  - Colores usados: {num_colors_used} de {len(palette_rgb)}")
    print(f"  - Compresion: {actual_size} bytes (indices) + {len(palette_rgb) * 2} bytes (paleta)")
    print(f"  - Total: ~{actual_size + len(palette_rgb) * 2} bytes")
    print("="*60)
    
    return True

def main():
    if len(sys.argv) < 2:
        print("Uso: python convert8BitPngColourMap2Bin.py <imagen.png>")
        print("\nEjemplo:")
        print("  python convert8BitPngColourMap2Bin.py mapa_colores.png")
        print("  python convert8BitPngColourMap2Bin.py C1W.png")
        print("\nRequisitos:")
        print("  - La imagen debe ser PNG de 8 bits indexado (P) o escala de grises (L)")
        print("  - Tamano recomendado: 1024x1024")
        print("  - Maximo 256 colores")
        sys.exit(1)
    
    input_png = sys.argv[1]
    
    if not os.path.exists(input_png):
        print(f"Error: No se encuentra el archivo {input_png}")
        sys.exit(1)
    
    if not input_png.lower().endswith('.png'):
        print("Error: El archivo debe ser PNG")
        sys.exit(1)
    
    # Ejecutar conversión
    success = convert_indexed_png(input_png)
    
    if success:
        print("\n✓ ¡Conversión completada exitosamente!")
    else:
        print("\n✗ Error en la conversión")
        sys.exit(1)

if __name__ == "__main__":
    # Verificar dependencias
    try:
        from PIL import Image
    except ImportError:
        print("Error: Pillow no instalado.")
        print("Instalar con: pip install Pillow")
        sys.exit(1)
    
    main()