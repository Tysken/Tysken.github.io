

#include <stdbool.h>
#include <math.h>
#include <emscripten.h>
#include <SDL.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengles2.h>


#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

#define windowSizeX  600
#define windowSizeY  800
#define rendererSizeX  600
#define rendererSizeY  600

typedef struct{
	float x,y;
}vec2f_t;

typedef struct{
    int x,y;
}vec2i_t;

typedef struct{
	float x,y,z;
}vec3f_t;


typedef struct{
	uint8_t b,g,r,a;
}argb_t;

struct{
	struct{
        int x;
        int y;
		uint32_t state;
	}mouse;
}input;

struct{
    int screenX;
    int screenY;
    int worldX;
    int worldY;
    float radius;
    float amount;
}cursor;

struct{
	float x,y;
	float rot;
	float zoom;
	vec2f_t oldpos;
	vec3f_t cursor;
	int visibility[rendererSizeX*rendererSizeY];
}cam;

#define MAPW 256
#define MAPH 256

struct{
	int w;
    int h;
    argb_t argb[MAPW*MAPH];
    float shadow[MAPW*MAPH];
    float shadowSoft[MAPW*MAPH];
    float stone[MAPW*MAPH];
    float water[5*MAPW*MAPH]; //water
}map;

//ändra water till att vara en array av struct fluid med medlemmarna height, left, osv

void water_update(float* fluid, float g, float l, float A, float friction, float dTime){
    float* T = map.stone;
    float* W = fluid;
    for(int y=1;y<MAPH-1;y++){
        for(int x=1;x<MAPW-1;x++){
            W[0+x*5+y*MAPW*5] = max(W[0+x*5+y*MAPW*5]*friction + (W[4+x*5+y*MAPW*5]+T[x+y*MAPW]-W[4+(x+1)*5+y*MAPW*5]-T[(x+1)+y*MAPW])*dTime*A*g/l , 0.f); //höger
            W[1+x*5+y*MAPW*5] = max(W[1+x*5+y*MAPW*5]*friction + (W[4+x*5+y*MAPW*5]+T[x+y*MAPW]-W[4+(x)*5+(y-1)*MAPW*5]-T[(x)+(y-1)*MAPW])*dTime*A*g/l , 0.f); //upp
            W[2+x*5+y*MAPW*5] = max(W[2+x*5+y*MAPW*5]*friction + (W[4+x*5+y*MAPW*5]+T[x+y*MAPW]-W[4+(x-1)*5+y*MAPW*5]-T[(x-1)+y*MAPW])*dTime*A*g/l , 0.f); //vänster
            W[3+x*5+y*MAPW*5] = max(W[3+x*5+y*MAPW*5]*friction + (W[4+x*5+y*MAPW*5]+T[x+y*MAPW]-W[4+(x)*5+(y+1)*MAPW*5]-T[(x)+(y+1)*MAPW])*dTime*A*g/l , 0.f); //ner
            
            
            //limit
            if(W[4+x*5+y*MAPW*5] - (W[0+x*5+y*MAPW*5]+W[1+x*5+y*MAPW*5]+W[2+x*5+y*MAPW*5]+W[3+x*5+y*MAPW*5]) < 0){
                float K = min(W[4+x*5+y*MAPW*5]*l*l/((W[0+x*5+y*MAPW*5]+W[1+x*5+y*MAPW*5]+W[2+x*5+y*MAPW*5]+W[3+x*5+y*MAPW*5])*dTime) , 1.f);
                W[0+x*5+y*MAPW*5] *= K;
                W[1+x*5+y*MAPW*5] *= K;
                W[2+x*5+y*MAPW*5] *= K;
                W[3+x*5+y*MAPW*5] *= K;
            }
        }
    }
    //border conditions
    for(int y=0;y<MAPH;y++){
        W[0+(MAPW-2)*5+y*MAPW*5] = 0;
        W[2+(2)*5+y*MAPW*5] = 0;
    }
    for(int x=0;x<MAPW;x++){
        W[1+(x)*5+(2)*MAPW*5] = 0;
        W[3+(x)*5+(MAPH-2)*MAPW*5] = 0;
    }
    
    //update depth
    for(int y=1;y<MAPH-1;y++){
        for(int x=1;x<MAPW-1;x++){
            float deltaV = (W[0+(x-1)*5+y*MAPW*5]+W[1+x*5+(y+1)*MAPW*5]+W[2+(x+1)*5+y*MAPW*5]+W[3+x*5+(y-1)*MAPW*5] - (W[0+x*5+y*MAPW*5]+W[1+x*5+y*MAPW*5]+W[2+x*5+y*MAPW*5]+W[3+x*5+y*MAPW*5]))*dTime;
            W[4+(x)*5+(y)*MAPW*5] = max(W[4+(x)*5+(y)*MAPW*5] + deltaV/(l*l), 0.f);
        }
    }
    
}

Uint32 getpixel(SDL_Surface *surface, int x, int y);

void loadMap(const char* path){
    int w = MAPW;
    int h = MAPH;
    SDL_Surface* bitmap = IMG_Load(path); 
    if(!bitmap){
        printf("IMG_Load: %s\n", IMG_GetError());
        return;
    }
    SDL_LockSurface(bitmap);

    for(int y = 0; y < h; y++){
        for(int x = 0; x < w; x++){
            Uint8 r,g,b;
            SDL_GetRGB(getpixel(bitmap,x,y),bitmap->format,&r,&g,&b);
            map.stone[x+y*w]  = r/4+10;
            map.argb[x+y*w].r = r;
            map.argb[x+y*w].g = g;
            map.argb[x+y*w].b = b;
        }
    }
    
    SDL_UnlockSurface(bitmap);
    SDL_FreeSurface (bitmap);
}

vec2f_t world2screen(float x, float y){
	float sinAlpha = sin(cam.rot);
	float cosAlpha = cos(cam.rot);

	//scale for zoom level
	float xs = x / cam.zoom;
	float ys = y / cam.zoom;
	//project

	//rotate 
	float xw =  cosAlpha*(xs+(cam.x-614.911)) + sinAlpha*(ys+(cam.y-119.936)) - (cam.x-614.911);
	float yw = -sinAlpha*(xs+(cam.x-614.911)) + cosAlpha*(ys+(cam.y-119.936)) - (cam.y-119.936);
	//offset for camera position

	xs = xw + cam.x;
	ys = yw + cam.y;
	
	xw = ((xs-ys)/sqrt(2));
	yw = ((xs+ys)/sqrt(6));

	vec2f_t retVal = {xw,yw};

	return retVal;
}

vec2f_t screen2world(float x,float y){
	float sinAlpha = sin(cam.rot);
	float cosAlpha = cos(cam.rot);
	float sq2d2 = sqrt(2)/2;
	float sq6d2 = sqrt(6)/2;
	//transform screen coordinates to world coordinates 
	float xw = sq2d2*x+sq6d2*y;
	float yw = sq6d2*y-sq2d2*x;
	xw = xw - cam.x;
	yw = yw - cam.y;


	//rotate view
	float xwr = cosAlpha*(xw+(cam.x-614.911)) - sinAlpha*(yw+(cam.y-119.936)) - (cam.x-614.911);
	float ywr = sinAlpha*(xw+(cam.x-614.911)) + cosAlpha*(yw+(cam.y-119.936)) - (cam.y-119.936);
//616 121
	//apply zoom
	xw = xwr*cam.zoom;
	yw = ywr*cam.zoom;
	
	vec2f_t retVal = {xw,yw};
	//double xw = (cosAlpha*(cam.x+sq2d2*x+sq6d2*y-Map.w/2) - sinAlpha*(cam.y+sq6d2*y-sq2d2*x-Map.h/2)+Map.w/2)*cam.zoom;
	//double yw = (sinAlpha*(cam.x+sq2d2*x+sq6d2*y-Map.w/2) + cosAlpha*(cam.y+sq6d2*y-sq2d2*x-Map.h/2)+Map.h/2)*cam.zoom;
	return retVal;
}

typedef struct{
	SDL_Texture * Texture;
	void * mPixels;
	Uint32 * pixels;
	int w; 
	int h;
	int pitch;
}sdlTexture;




//gets pixel color from a png
Uint32 getpixel(SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        return *p;
        break;

    case 2:
        return *(Uint16 *)p;
        break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;
        break;

    case 4:
        return *(Uint32 *)p;
        break;

    default:
        return 0;       /* shouldn't happen, but avoids warnings */
    }
}

uint8_t fontBitMap[8*95];

void loadFont(const char* path){
    int w = 8;
    int h = 8;
    int n = 95;
    SDL_Surface* bitmap = IMG_Load(path); 
    if(!bitmap){
        printf("IMG_Load: %s\n", IMG_GetError());
        return;
    }
    SDL_LockSurface(bitmap);
    for(int N = 0; N < n; N++){
        for(int y = 0; y < h; y++){
            for(int x = 0; x < w; x++){
                Uint8 r,g,b;
                SDL_GetRGB(getpixel(bitmap,1+x+N*(w+1),y+1),bitmap->format,&r,&g,&b);
                if(r == 252){ //might need adjustment depending on the font
                    fontBitMap[y+N*h] |= (1 << x); //save each 8px row as one byte
                }
            }
        }
    }
    SDL_UnlockSurface(bitmap);
    SDL_FreeSurface (bitmap);
}

void print(sdlTexture* tex, char* str, int x, int y){
    int w = 8;
    int h = 8;
    int n = 95;
    for(int n=0;n<strlen(str);n++){ //iterate over each char in the string and print from fontMap
        for(int i=0;i<h;i++){
            for(int j=0;j<w;j++){
                if(fontBitMap[i+(w*((int)str[n]-32))] & (1 << j)){ //fontMap starts on ! so offset by 33
                    tex->pixels[x+j+n*(w+1)+(y+i+1)*tex->w] = 0xFFFFFFFF; //white
                }
            }
        }
    }
}