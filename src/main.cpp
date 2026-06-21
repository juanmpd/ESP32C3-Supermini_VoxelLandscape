/*
Originally based on Landscape.pas which came with MSDOS era SWAG Pascal library.
(SWAG was an extensive collection of source code, tricks, demos and examples in Pascal programming language)
  Category: SWAG Title: GRAPHICS ROUTINES
  Original name: 0119.PAS
  Description: Landscape
  Author: MARCIN BORKOWSKI
  Date: 08-24-94  13:50
Then I converted it into a Java applet, and now converted into C++ for the ESP32-C3

Finally, added ideas from SEBASTIAN MACKE's https://github.com/s-macke/VoxelSpace/blob/master/VoxelSpace.html
*/

#include <Arduino.h>
#include <LGFX.hpp>       // Hardware-specific library
#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>

// Color palette for the terrain indexed colors
#include "color_palette.h"
// Terrain heights and colors (indexed)
#define TERRAIN_WIDTH 1024
#define TERRAIN_HEIGHT 1024
extern const uint8_t terrainHeights[] __attribute__((aligned(4)));
extern const uint8_t terrainColors[] __attribute__((aligned(4)));


LGFX tft;


void printMemStatistics() {
  Serial.printf("Free Heap          : %lu bytes\n", (unsigned long)ESP.getFreeHeap());
  Serial.printf("Max Contiguous Heap: %lu bytes\n", (unsigned long)ESP.getMaxAllocHeap());
}

#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240
#define WINDOW_WIDTH 312
#define WINDOW_HEIGHT 236

// Buffer donde dibujamos
uint8_t *pixels = NULL; // [WINDOW_WIDTH * WINDOW_HEIGHT];

#define rgbBufferSize (WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(uint16_t))
#define pixelBufferSize (WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(uint8_t))
#define terrainWidthBufferSize (TERRAIN_WIDTH * sizeof(uint8_t))

// Para ayudar en el dibujado, mientras vamos mirando de lejos a cerca
uint8_t lineaScan[WINDOW_WIDTH]; 

// DISPLAY
static uint16_t *rgbBuffer = NULL; // [WINDOW_WIDTH * WINDOW_HEIGHT];

const double DOS_PI = 2.0*PI;
const double PASO_GIRO = DOS_PI / 36.0;  // Incremento de angulo al girar por teclado
const uint8_t PASO_AVANCE = 4;           // Incremento de coordenada al avanzar/retroceder

// Posicion y direccion
int x=0, y=0;
double direccion = 0.0;

/**
 * Fixed Point helpers
 */

typedef int32_t fixedPointNumber;
#define FIXED_POINT_DECIMAL_DIGITS 12
#define FIXED_POINT_FLOAT_TO_INT_FACTOR (1 << FIXED_POINT_DECIMAL_DIGITS)
#define FROM_FLOAT_TO_FIXED_POINT(f) ((fixedPointNumber)std::round(FIXED_POINT_FLOAT_TO_INT_FACTOR*(f)))
#define FROM_INTEGER_TO_FIXED_POINT(i) ((i)<<FIXED_POINT_DECIMAL_DIGITS)
#define FROM_UINT8_TO_FIXED_POINT(ui) (static_cast<fixedPointNumber>(((uint32_t)(ui))<<FIXED_POINT_DECIMAL_DIGITS))
#define FROM_FIXED_POINT_TO_INTEGER(fp) ((fp)>>FIXED_POINT_DECIMAL_DIGITS)
#define FROM_FIXED_POINT_TO_FLOAT(fp) ((fp)/((float)FIXED_POINT_FLOAT_TO_INT_FACTOR))
#define FIXED_POINT_MULTIPLICATION(fp1, fp2) (static_cast<fixedPointNumber>(((static_cast<int64_t>(fp1))*(fp2))>>FIXED_POINT_DECIMAL_DIGITS))
#define FIXED_POINT_DIVISION(fp1, fp2) (static_cast<fixedPointNumber>(((static_cast<int64_t>(fp1))<<FIXED_POINT_DECIMAL_DIGITS)/(fp2)))

void dibujaEnBuffer() {
    // Constantes varias
    const int CAMERA_HEIGHT = 100;
    const uint32_t HEIGHT_SCALE = 30;
    const uint8_t PROFUN_SCAN = 55;
    const int ANCHO_VENTANA_AMPLIADO = (int)std::round(WINDOW_WIDTH * 1.125);
    const int ANCHO_PANTALLA_REDUCIDO = (int)std::round(WINDOW_WIDTH * 0.9375);

    // Variables que usaremos
    int32_t z, zobs, iy1, iyterreno, ixterreno, xpant, ypant, s, i, j, aux, aux2;
    int32_t xterreno = x, yterreno = y;
    uint8_t mpc;
    // Cosenos y senos correspondientes a DIRECCION
    fixedPointNumber fpCsf = FROM_FLOAT_TO_FIXED_POINT(std::cos(direccion));
    fixedPointNumber fpSnf = FROM_FLOAT_TO_FIXED_POINT(std::sin(direccion));
    // Reset de la linea de scan, y del buffer de 
    for (aux=WINDOW_WIDTH-1; aux>=0; aux--) lineaScan[aux]=WINDOW_HEIGHT-1;
    for (aux=WINDOW_WIDTH*WINDOW_HEIGHT-1; aux>=0; aux--) pixels[aux]=(uint8_t)0;
    // Calculos
    zobs = CAMERA_HEIGHT + terrainHeights[yterreno*TERRAIN_WIDTH+xterreno];
    for (aux=0; aux<=PROFUN_SCAN; aux++){
        iy1=1+2*(aux); s=4 + ANCHO_PANTALLA_REDUCIDO/iy1;
        for (aux2=-aux; aux2<=aux; aux2++){
            ixterreno=xterreno + FROM_FIXED_POINT_TO_INTEGER(aux2*fpCsf+aux*fpSnf);
            iyterreno=yterreno + FROM_FIXED_POINT_TO_INTEGER(aux*fpCsf-aux2*fpSnf);
            if (ixterreno<0) ixterreno+=TERRAIN_WIDTH;
            else if (ixterreno>=TERRAIN_WIDTH) ixterreno-=TERRAIN_WIDTH;
            if (iyterreno<0) iyterreno+=TERRAIN_HEIGHT;
            else if (iyterreno>=TERRAIN_HEIGHT) iyterreno-=TERRAIN_HEIGHT;
            xpant=(WINDOW_WIDTH>>1)+ ANCHO_VENTANA_AMPLIADO*aux2/iy1;
            if ((xpant>=0) & (xpant+s<WINDOW_WIDTH)) {
                int32_t offset = iyterreno*TERRAIN_WIDTH+ixterreno;
                z=terrainHeights[offset];
                mpc=terrainColors[offset];
                ypant=(WINDOW_HEIGHT>>1)+(zobs-z)*HEIGHT_SCALE / iy1;
                if ((ypant<WINDOW_HEIGHT) & (ypant>=0)) {
                    for (j=xpant; j<=xpant+s; j++) {
                        for (i=ypant; i<lineaScan[j]; i++) {
                            pixels[WINDOW_WIDTH*i+j]=mpc;
                        }
                        if (ypant<lineaScan[j]) {
                            lineaScan[j]=ypant;
                        }
                    }
                }
            }
        }
    }
}

void moverse() {
    static const int pasos = PASO_AVANCE;
    y = y + (int)std::round((pasos * std::cos(direccion)));
    if (y>=TERRAIN_HEIGHT) y-=TERRAIN_HEIGHT;
    else if (y<0) y+=TERRAIN_HEIGHT;
    x = x + (int)std::round((pasos * std::sin(direccion)));
    if (x>=TERRAIN_WIDTH) x-=TERRAIN_WIDTH;
    else if (x<0) x+=TERRAIN_WIDTH;
    direccion = direccion + (DOS_PI / 360);
    if (direccion > DOS_PI) direccion -= DOS_PI;
}

uint16_t skyBackgroundColor(int y) {
    // Sky gradient (dark to light blue)
    float t = static_cast<float>(y) / WINDOW_HEIGHT;
    uint8_t r = static_cast<uint8_t>(9 + 12 * t);
    uint8_t g = static_cast<uint8_t>(24 + 39 * t);
    uint8_t b = static_cast<uint8_t>(19 + 12 * t);
    uint16_t color = (r << 11) | (g << 5) | b;
    return color;
}

void dumpBufferToDisplay() {
    // Convert indexed colors into RGB565
    uint16_t *pRGBBuffer = rgbBuffer;
    uint8_t *pPixels = pixels;
    for (uint32_t y=WINDOW_HEIGHT; y>0; y--) {
        uint16_t skyColor = skyBackgroundColor(WINDOW_HEIGHT-y);
        for (uint32_t x=WINDOW_WIDTH; x>0; x--) {
            uint8_t colorIndex = *pPixels++;
            *pRGBBuffer++ = (colorIndex!=0) ? color_palette[colorIndex] : skyColor;
        }
    }
    // Dump RGB buffer to display
    tft.pushImageDMA((DISPLAY_WIDTH-WINDOW_WIDTH)/2, (DISPLAY_HEIGHT-WINDOW_HEIGHT)/2, WINDOW_WIDTH, WINDOW_HEIGHT, rgbBuffer);
}

bool inicializacionOk = false;
void setup() {
  Serial.begin(115200);
  // Messages to Serial are lost for the first seconds after startup, and I want them all, so let's wait.
  delay(5000);
  Serial.printf("Setup just started...\n");
  printMemStatistics();

  // TFT initialization
  tft.begin();
  tft.setSwapBytes(true);
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  ledcAttach(TFT_BL, 5000, 8);  // Frecuencia 5 kHz, resolución 8 bits (0–255)
  ledcWrite(TFT_BL, 255);        // 0-255 Brightness value for the display

  // Memory allocation. First bigger block, to try to minimize the risk of 
  // not succeeding due to memory fragmentation preventing it.
  Serial.printf("Going to allocate memory (1/2)...\n");
  rgbBuffer = (uint16_t*) malloc(rgbBufferSize);
  Serial.printf("Going to allocate memory (2/2)...\n");
  pixels = (uint8_t*) malloc(pixelBufferSize); 
  if (!rgbBuffer || !pixels) {
    // Memory could not be allocated. Maybe it is fragmented
    int y = 0;
    tft.setTextColor(TFT_BLUE, TFT_BLUE); // Do not plot the background colour
    tft.drawString("Could not allocate memory", 0, (y++)*16, 2);  // Draw text using font 2
    tft.drawString(String("Free heap: ") + ESP.getFreeHeap(), 0, (y++)*16, 2);
    tft.drawString(String("Largest block: ") + heap_caps_get_largest_free_block(MALLOC_CAP_8BIT), 0, (y++)*16, 2);
    tft.drawString(String("rgbBuffer=0x") + String((uint32_t)rgbBuffer, HEX), 0, (y++)*16, 2);
    tft.drawString(String("pixels=0x") + String((uint32_t)pixels, HEX), 0, (y++)*16, 2);
    return;
  }
  Serial.printf("Memory allocated successfully\n");
  memset(rgbBuffer, 0, rgbBufferSize);
  Serial.printf("Memory zeroed (1/2)\n");
  memset(pixels, 0, pixelBufferSize);
  Serial.printf("Memory zeroed (2/2)\n");
  inicializacionOk = true;
  Serial.printf("Setup just completed...\n");
  printMemStatistics();
}

void loop() {
  if (!inicializacionOk) {
    // Memory could not be allocated. Maybe it is fragmented
    delay(100000);  // We'll actually be stuck in the loop here, so this delay is only not to have the CPU working too much...
    return;
  }
  // Dibujar en buffer indexado 
  dibujaEnBuffer();
  // Volcar buffer indexado a display rgb
  dumpBufferToDisplay();
  // Moverse
  moverse();
}
