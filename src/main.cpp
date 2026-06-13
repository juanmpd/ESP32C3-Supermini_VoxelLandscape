/*
Basado originalmente en Landscape.pas que venía con SWAG de la época de MSDOS
(SWAG era una colección muy extensa de códigos fuente, trucos y ejemplos en Pascal)
  Category: SWAG Title: GRAPHICS ROUTINES
  Original name: 0119.PAS
  Description: Landscape
  Author: MARCIN BORKOWSKI
  Date: 08-24-94  13:50
Después pasado a applet Java y ahora adaptado a C++ para el ESP32C3
*/

#include <Arduino.h>
#include <LGFX.hpp>       // Hardware-specific library

LGFX tft;

#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>


void printMemStatistics() {
  Serial.printf("Free Heap          : %lu bytes\n", (unsigned long)ESP.getFreeHeap());
  Serial.printf("Max Contiguous Heap: %lu bytes\n", (unsigned long)ESP.getMaxAllocHeap());
}

// RGB565
static const uint16_t infoPaleta [] = {
  0,50712,21,117,2230,2294,2358,4471,4535,6648,6712,8825,8889,10970,11034,13147,13211,15324,15388,17501,17501,17565,19678,19742,21855,
  21919,11232,11232,11232,11264,13312,13313,13345,13345,15393,15393,15425,15426,15458,17506,17506,17538,17538,17539,19619,19651,19652,
  21732,21764,21765,23845,23877,23878,25958,25990,25991,26023,28103,28104,28136,30185,30217,30249,32298,32330,32362,34411,34443,34475,
  36524,36556,36588,38637,38669,38701,40749,40749,42797,44845,44845,46893,46893,48941,48941,50989,53037,53037,55085,55085,57133,59181,
  59181,61229,61229,61228,61228,61195,61163,61130,61097,61033,61000,61000,60967,60935,60902,60870,60837,60805,60805,60773,60740,58660,
  58628,58595,58563,56482,56418,56354,56289,33808,65535,65535,65535,65535,50712,65535,65535
};

// DIMENSIONES DE LA VENTANA. No podemos aprovechar el TFT al 100% porque el ESP32C3 tiene una
// RAM limitada y necesitamos más de la que nos da
// FIXME SEGURAMENTE LUEGO PODREMOS
#define ANCHO_TFT 320
#define ALTO_TFT 240
#define ANCHO_VENTANA 312
#define ALTO_VENTANA 236

// PALETA DE COLORES
const uint8_t NUMERO_COLORES = sizeof(infoPaleta) / sizeof(infoPaleta[0]);

// MAPA DEL TERRENO
#define ANCHO_TERRENO 1024
#define ALTO_TERRENO 1024
extern const uint8_t terreno[] __attribute__((aligned(4)));

// Buffer donde dibujamos
uint8_t *pixels = NULL; // [ANCHO_VENTANA * ALTO_VENTANA];

#define rgbBufferSize (ANCHO_VENTANA * ALTO_VENTANA * sizeof(uint16_t))
#define pixelBufferSize (ANCHO_VENTANA * ALTO_VENTANA * sizeof(uint8_t))
#define terrainWidthBufferSize (ANCHO_TERRENO * sizeof(uint8_t))

// Para ayudar en el dibujado, mientras vamos mirando de lejos a cerca
uint8_t lineaScan[ANCHO_VENTANA]; 

// DISPLAY
static uint16_t *rgbBuffer = NULL; // [ANCHO_VENTANA * ALTO_VENTANA];

const double DOS_PI = 2.0*PI;
const double PASO_GIRO = DOS_PI / 36.0;  // Incremento de angulo al girar por teclado
const uint8_t PASO_AVANCE = 4;           // Incremento de coordenada al avanzar/retroceder

// Posicion y direccion
int x=0, y=0;
double direccion = 0.0;
const int ALTURA_OBSERVADOR = 100;

void initTerrenoConPlasma();
uint8_t ncol(int mc, int n, int dvd);
void aplicarPlasmaEnTerreno(int x1, int y1, int x2, int y2);

//
// Inicialización
//
void initLandVoxel() {
    // Inicializar un mapa del terreno
    initTerrenoConPlasma();
    // Parametros iniciales para el movimiento
    x = y = 0;
    direccion = 0;
    direccion=0.0;
}

//
// PLASMA
//
const uint8_t VALOR_MAX_PLASMA = NUMERO_COLORES-1;
const uint8_t VALOR_FIJADO_PLASMA = 5;
const uint8_t VALOR_MIN_PLASMA = 5; // DEBE SER DISTINTO DE 0
void initTerrenoConPlasma(){
/*
    for (uint16_t y=0; y <ALTO_TERRENO; y++) {
        memset(terreno[y], 0, terrainWidthBufferSize);
    }
    terreno[0][0]=(uint8_t)(VALOR_FIJADO_PLASMA);
    aplicarPlasmaEnTerreno(0,0,ANCHO_TERRENO,ALTO_TERRENO);
*/
}
uint8_t ncol(int mc, int n, int dvd) {
    int loc;
    double random = rand() / (double)RAND_MAX;
    loc = (mc+n-(int)(2*n*random)) / dvd;
    if (loc>VALOR_MAX_PLASMA) loc=VALOR_MAX_PLASMA;
    else if (loc<VALOR_MIN_PLASMA) loc=VALOR_MIN_PLASMA;
    return (uint8_t)loc;
}
void aplicarPlasmaEnTerreno(int x1, int y1, int x2, int y2) {
/*
    // NOTA: para dar impresion de continuidad, jugamos con X2/Y2, de
    // forma que x2=ANCHO_TERRENO equivale a 0, y y2=ALTO_TERRENO equivale
    // a 0 (además, solo se puede indexar de 0 a ANCHOoALTO-1)

    // Para dar impresion de continuidad:
    int x2b = x2, y2b = y2;
    if (x2b==ANCHO_TERRENO) x2b = 0;
    if (y2b==ALTO_TERRENO) y2b = 0;
    // Proceso en sí:
    int xn, yn, dxy, p1, p2, p3, p4;
    if ((x2-x1<2) & (y2-y1<2)) return;
    p1=terreno[y1][x1]; p2=terreno[y2b][x1];
    p3=terreno[y1][x2b]; p4=terreno[y2b][x2b];
    xn=(x2+x1)>>1; yn=(y2+y1)>>1; dxy=5*(x2-x1+y2-y1)/3;
    if (terreno[y1][xn]==(uint8_t)0) terreno[y1][xn]=ncol(p1+p3,dxy,2);
    if (terreno[yn][x1]==(uint8_t)0) terreno[yn][x1]=ncol(p1+p2,dxy,2);
    if (terreno[yn][x2b]==(uint8_t)0) terreno[yn][x2b]=ncol(p3+p4,dxy,2);
    if (terreno[y2b][xn]==(uint8_t)0) terreno[y2b][xn]=ncol(p2+p4,dxy,2);
    terreno[yn][xn]=ncol(p1+p2+p3+p4,dxy,4);
    aplicarPlasmaEnTerreno(x1,y1,xn,yn); aplicarPlasmaEnTerreno(xn,y1,x2,yn);
    aplicarPlasmaEnTerreno(x1,yn,xn,y2); aplicarPlasmaEnTerreno(xn,yn,x2,y2);
*/
}

#define OPTIMIZ_ANG_LOG2 8
#define OPTIMIZ_ANG (1 << OPTIMIZ_ANG_LOG2)

int kk=0;
void dibujaEnBuffer() {
    // Constantes varias
    const uint8_t PROFUN_SCAN = 55;
    const int ANCHO_VENTANA_AMPLIADO = (int)std::round(ANCHO_VENTANA * 1.125);
    const int ANCHO_PANTALLA_REDUCIDO = (int)std::round(ANCHO_VENTANA * 0.9375);

    // Variables que usaremos
    int32_t z, zobs, iy1, iyterreno, ixterreno, xpant, ypant, s, csf, snf, i, j, aux, aux2;
    int32_t xterreno = x, yterreno = y;
    uint8_t mpc;
    // Cosenos y senos correspondientes a DIRECCION
    csf = (int32_t)std::round(OPTIMIZ_ANG*std::cos(direccion));
    snf = (int32_t)std::round(OPTIMIZ_ANG*std::sin(direccion));    
    // Reset de la linea de scan, y del buffer de 
    for (aux=ANCHO_VENTANA-1; aux>=0; aux--) lineaScan[aux]=ALTO_VENTANA-1;
    for (aux=ANCHO_VENTANA*ALTO_VENTANA-1; aux>=0; aux--) pixels[aux]=(uint8_t)0;
    // Calculos
    zobs = ALTURA_OBSERVADOR + terreno[yterreno*ANCHO_TERRENO+xterreno];
    for (aux=0; aux<=PROFUN_SCAN; aux++){
        iy1=1+2*(aux); s=4 + ANCHO_PANTALLA_REDUCIDO/iy1;
        for (aux2=-aux; aux2<=aux; aux2++){
            ixterreno=xterreno + ((aux2*csf+aux*snf) >> OPTIMIZ_ANG_LOG2);
            iyterreno=yterreno + ((aux*csf-aux2*snf) >> OPTIMIZ_ANG_LOG2);
            if (ixterreno<0) ixterreno+=ANCHO_TERRENO;
            else if (ixterreno>=ANCHO_TERRENO) ixterreno-=ANCHO_TERRENO;
            if (iyterreno<0) iyterreno+=ALTO_TERRENO;
            else if (iyterreno>=ALTO_TERRENO) iyterreno-=ALTO_TERRENO;
            xpant=(ANCHO_VENTANA>>1)+ ANCHO_VENTANA_AMPLIADO*aux2/iy1;
            if ((xpant>=0) & (xpant+s<ANCHO_VENTANA)) {
                mpc=terreno[iyterreno*ANCHO_TERRENO+ixterreno];
                z=mpc;
                // Next line was to allow for same sea level with different degrees of blue. Now we do not want this
                // if (z<47) z=46;
                ypant=(ALTO_VENTANA>>1)+(zobs-z)*30 / iy1;
                if ((ypant<ALTO_VENTANA) & (ypant>=0)) {
                    for (j=xpant; j<=xpant+s; j++) {
                        for (i=ypant; i<lineaScan[j]; i++) {
                            pixels[ANCHO_VENTANA*i+j]=mpc;
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
    if (y>=ALTO_TERRENO) y-=ALTO_TERRENO;
    else if (y<0) y+=ALTO_TERRENO;
    x = x + (int)std::round((pasos * std::sin(direccion)));
    if (x>=ANCHO_TERRENO) x-=ANCHO_TERRENO;
    else if (x<0) x+=ANCHO_TERRENO;
    direccion = direccion + (DOS_PI / 360);
    if (direccion > DOS_PI) direccion -= DOS_PI;
}

void vuelcaBufferIndexadoADisplayRGB() {
    // Pasar de indexado a rgb
    uint16_t *pRGBBuffer = rgbBuffer;
    uint8_t *pPixels = pixels;
    for (uint32_t i=ANCHO_VENTANA*ALTO_VENTANA; i>0; i--) {
        *pRGBBuffer++ = infoPaleta[*pPixels++];
    }
    // Vuelca el buffer RGB al display
    tft.pushImageDMA((ANCHO_TFT-ANCHO_VENTANA)/2, (ALTO_TFT-ALTO_VENTANA)/2, ANCHO_VENTANA, ALTO_VENTANA, rgbBuffer);
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
  /*
  for (uint16_t ty=0; ty<ALTO_TERRENO; ty++) {
    uint8_t* _d = terreno[ty] = (uint8_t*) malloc(terrainWidthBufferSize); 
    if (!_d) {
        int y = 0;
        tft.drawString("Could not allocate memory", 0, (y++)*16, 2);  // Draw text using font 2
        tft.drawString(String("Free heap: ") + ESP.getFreeHeap(), 0, (y++)*16, 2);
        tft.drawString(String("Largest block: ") + heap_caps_get_largest_free_block(MALLOC_CAP_8BIT), 0, (y++)*16, 2);
        tft.drawString(String("Filas que faltan: ") + (ALTO_TERRENO-ty), 0, (y++)*16, 2);
        return;
    }
  } 
  */
  // Inicializacion (preparar terreno)
  initLandVoxel();
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
  // Dibujar en buffer 
  dibujaEnBuffer();
  // Volcar buffer indexado a display rgb
  vuelcaBufferIndexadoADisplayRGB();
  // Moverse
  moverse();
}
