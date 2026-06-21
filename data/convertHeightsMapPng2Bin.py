#!/usr/bin/env python3
"""
Convierte PNG de 8 bits (escala de grises) a binario raw
Uso: python convert_png_to_bin.py altura.png altura.bin
"""

from PIL import Image
import sys
import os

def png_to_8bit_bin(png_file, output_bin):
    """
    Convierte PNG a binario de 8 bits (0-255)
    """
    # Abrir imagen y convertir a escala de grises
    img = Image.open(png_file).convert('L')
    
    # Verificar dimensiones
    width, height = img.size
    print(f"Imagen: {width}x{height}")
    print(f"Modo: {img.mode}")
    
    if width != 1024 or height != 1024:
        print(f"Advertencia: La imagen no es 1024x1024, es {width}x{height}")
    
    # Obtener datos como bytes
    pixel_data = img.tobytes()
    
    # Verificar tamaño
    expected_size = width * height
    actual_size = len(pixel_data)
    
    print(f"Tamaño esperado: {expected_size} bytes")
    print(f"Tamaño real: {actual_size} bytes")
    
    # Guardar como binario
    with open(output_bin, 'wb') as f:
        f.write(pixel_data)
    
    # Mostrar estadísticas
    min_val = min(pixel_data)
    max_val = max(pixel_data)
    print(f"Valores: min={min_val}, max={max_val}")
    print(f"✓ Archivo guardado: {output_bin} ({actual_size} bytes)")

def png_to_8bit_bin_optimized(png_file, output_bin):
    """
    Versión optimizada con numpy (más rápida)
    """
    import numpy as np
    
    img = Image.open(png_file).convert('L')
    width, height = img.size
    
    # Convertir a numpy array
    pixels = np.array(img, dtype=np.uint8)
    
    # Guardar como binario
    with open(output_bin, 'wb') as f:
        f.write(pixels.tobytes())
    
    print(f"✓ Guardado: {output_bin} ({len(pixels.tobytes())} bytes)")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Uso: python convert_png_to_bin.py <entrada.png> <salida.bin>")
        print("Ejemplo: python convert_png_to_bin.py heightmap.png heightmap.bin")
        sys.exit(1)
    
    input_png = sys.argv[1]
    output_bin = sys.argv[2]
    
    try:
        png_to_8bit_bin(input_png, output_bin)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)