 #include <stdio.h>
 #include <stdbool.h>
 #include <math.h>
 #include <SDL2/SDL.h>
 #include <SDL2/SDL_image.h>
 
 /* Dimensiones de la ventana */
 #define WINDOW_WIDTH 800
 #define WINDOW_HEIGHT 600
 
 /* Variables globales */
 SDL_Window* window = NULL;
 SDL_Renderer* renderer = NULL;
 SDL_Texture* backgroundTexture = NULL;
 
 /* Parámetros de transformación */
 float rotation = 0.0f;     
 float scale = 1.0f;        
 int offsetX = 0;           
 int offsetY = 0;           
 
 /**
  * Inicializa SDL y crea la ventana
  */
 bool initialize() {
     if (SDL_Init(SDL_INIT_VIDEO) < 0) {
         printf("Error al inicializar SDL: %s\n", SDL_GetError());
         return false;
     }
 
     int imgFlags = IMG_INIT_PNG;
     if (!(IMG_Init(imgFlags) & imgFlags)) {
         printf("Error al inicializar SDL_image: %s\n", IMG_GetError());
         return false;
     }
 
     window = SDL_CreateWindow(
         "Basic Mode 7",
         SDL_WINDOWPOS_UNDEFINED,
         SDL_WINDOWPOS_UNDEFINED,
         WINDOW_WIDTH,
         WINDOW_HEIGHT,
         SDL_WINDOW_SHOWN
     );
     if (window == NULL) {
         printf("Error al crear ventana: %s\n", SDL_GetError());
         return false;
     }
 
     /* Crear renderer */
     renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
     if (renderer == NULL) {
         printf("Error al crear renderer: %s\n", SDL_GetError());
         return false;
     }
 
     /* Establecer color de fondo */
     SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
 
     return true;
 }
 
 /**
  * Carga una imagen desde un archivo
  */
 bool loadBackground(const char* path) {
     /* Cargar imagen como superficie */
     SDL_Surface* loadedSurface = IMG_Load(path);
     if (loadedSurface == NULL) {
         printf("Error al cargar imagen %s: %s\n", path, IMG_GetError());
         return false;
     }
 
     /* Crear textura desde superficie */
     backgroundTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
     if (backgroundTexture == NULL) {
         printf("Error al crear textura: %s\n", SDL_GetError());
         SDL_FreeSurface(loadedSurface);
         return false;
     }
 
     /* Liberar superficie */
     SDL_FreeSurface(loadedSurface);
 
     return true;
 }
 
 /**
  * Renderiza el fondo con transformaciones
  */
 void renderBackground() {
     /* Limpiar pantalla */
     SDL_RenderClear(renderer);
 
     /* Obtener dimensiones de la textura */
     int width, height;
     SDL_QueryTexture(backgroundTexture, NULL, NULL, &width, &height);
 
     /* Definir rectángulo de destino (centrado en la pantalla) */
     SDL_Rect destRect;
     destRect.w = width * scale;
     destRect.h = height * scale;
     destRect.x = (WINDOW_WIDTH - destRect.w) / 2 + offsetX;
     destRect.y = (WINDOW_HEIGHT - destRect.h) / 2 + offsetY;
 
     /* Crear punto central para la rotación */
     SDL_Point center;
     center.x = destRect.w / 2;
     center.y = destRect.h / 2;
 
     /* Renderizar textura con rotación y escala */
     SDL_RenderCopyEx(
         renderer,          
         backgroundTexture, 
         NULL,              
         &destRect,         
         rotation,          
         &center,           
         SDL_FLIP_NONE      
     );
 
     /* Actualizar pantalla */
     SDL_RenderPresent(renderer);
 }
 
 /**
  * Muestra instrucciones en la consola
  */
 void showInstructions() {
     printf("Controles:\n");
     printf("  Flechas izquierda/derecha: Rotar imagen\n");
     printf("  Flechas arriba/abajo: Zoom in/out\n");
     printf("  W/A/S/D: Mover imagen\n");
     printf("  R: Restablecer transformaciones\n");
     printf("  ESC: Salir\n");
 }
 
 /**
  * Libera recursos y cierra SDL
  */
 void cleanup() {
     
     if (backgroundTexture != NULL) {
         SDL_DestroyTexture(backgroundTexture);
         backgroundTexture = NULL;
     }
 
     if (renderer != NULL) {
         SDL_DestroyRenderer(renderer);
         renderer = NULL;
     }
 
     if (window != NULL) {
         SDL_DestroyWindow(window);
         window = NULL;
     }
     IMG_Quit();
     SDL_Quit();
 }
 
 
 int main(int argc, char* argv[]) {
     
     if (argc < 2) {
         printf("Uso: %s <ruta_imagen>\n", argv[0]);
         return 1;
     }
 
     if (!initialize()) {
         printf("Error en la inicialización.\n");
         cleanup();
         return 1;
     }
 
     if (!loadBackground(argv[1])) {
         printf("Error al cargar el fondo.\n");
         cleanup();
         return 1;
     }
 
     showInstructions();
 
     bool quit = false;
     SDL_Event e;
 
     while (!quit) {
         while (SDL_PollEvent(&e) != 0) {
             if (e.type == SDL_QUIT) {
                 quit = true;
             }
             else if (e.type == SDL_KEYDOWN) {
                 switch (e.key.keysym.sym) {
                     case SDLK_ESCAPE:
                         quit = true;
                         break;
                     case SDLK_LEFT:
                         rotation -= 2.0f; 
                         break;
                     case SDLK_RIGHT:
                         rotation += 2.0f; 
                         break;
                     case SDLK_UP:
                         scale += 0.05f; 
                         break;
                     case SDLK_DOWN:
                         scale -= 0.05f; 
                         if (scale < 0.05f) scale = 0.05f; 
                         break;
                     case SDLK_w:
                         offsetY -= 2; 
                         break;
                     case SDLK_s:
                         offsetY += 2; 
                         break;
                     case SDLK_a:
                         offsetX -= 2; 
                         break;
                     case SDLK_d:
                         offsetX += 2; 
                         break;
                     case SDLK_r:
                         rotation = 0.0f;
                         scale = 1.0f;
                         offsetX = 0;
                         offsetY = 0;
                         break;
                 }
             }
         }
 
         renderBackground();
 
         SDL_Delay(16);
     }
 
     cleanup();
     return 0;
 }