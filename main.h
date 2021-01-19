


//To compile with multithreading
//emcc main.c -std=c17 -s WASM=1 -s USE_SDL=2 -s -s USE_SDL_IMAGE=2 -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=0 -s SDL2_IMAGE_FORMATS='["png"]' -s TOTAL_MEMORY=536870912 -s USE_PTHREADS=1 -s PTHREAD_POOL_SIZE=4 ^C-preload-file assets -O3 --profiling -o index.js

//To compile without
//emcc main.c -std=c17 -s WASM=1 -s USE_SDL=2 -s -s USE_SDL_IMAGE=2 -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=0 -s SDL2_IMAGE_FORMATS='["png"]' -s TOTAL_MEMORY=536870912  --preload-file assets -O3 --profiling -o index.js


#include <stdbool.h>
#include <math.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <SDL.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengles2.h>

#define IS_FULLSCREEN 0

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })
#define M_PI    3.14159265358979323846264338327950288   /* pi */

#include "draw.h"
#include "gui.h"

#define ENABLE_MULTITHREADING 0
#if ENABLE_MULTITHREADING
#include <pthread.h>
#include <stdatomic.h>
#endif


#define windowSizeX    800
#define windowSizeY    600
#define hudSizeX       200
#define hudSizeY       600
#define rendererSizeX  600
#define rendererSizeY  600

//Map size (fluid and other)
#define MAPW 256
#define MAPH 256

//max foam particle number
#define FOAMSIZE 100000

inline uint32_t toInt2( float fval ) { fval += 1<<23; return ((uint32_t)fval) & 0x007FFFFF; }
#define F2INT(fval) (((uint32_t)((fval) + (1<<23)))&0x007FFFF)
#define ROUNDF(x) ((int)(x + 0.5f))

typedef struct{
	float x,y;
}vec2f_t;

typedef struct{
    int x,y;
}vec2i_t;

typedef struct{
	uint16_t x,y;
}vec2i16_t;

typedef struct{
	float x,y,z;
}vec3f_t;



struct{
    int screenX;
    int screenY;
    int worldX;
    int worldY;
    enum{
    	TOOL_WATER = 1,
		TOOL_SAND  = 2,
		TOOL_STONE = 3
    }tool;
    float radius;
    float amount;
}cursor;

typedef struct{
	float x,y;
	float rot;
	float zoom;
	vec2f_t oldpos;
	vec3f_t cursor;
	int visibility[rendererSizeX*rendererSizeY];
}camera_t;
camera_t g_cam;


typedef struct{
	uint32_t size;
	uint32_t counter;
	vec2f_t pos[FOAMSIZE];
//	vec2f_t vel[FOAMSIZE];
	struct{
		uint8_t on :1;
		uint8_t amount :7;
	}status[FOAMSIZE];
//	uint8_t amount[FOAMSIZE]; //total amount of foam in position
//	uint8_t on[FOAMSIZE];
}foam_t;

foam_t foam;


typedef struct{
	float right;
	float down;
	float left;
	float up;
	float depth;
}fluid_t;

struct{
	int w;
    int h;
    float tileWidth; //width of one tile, used for adjusting fluid simulation
    argb_t argb[MAPW*MAPH];
    float shadow[MAPW*MAPH];
    float shadowSoft[MAPW*MAPH];
    float stone[MAPW*MAPH];
    float sand[MAPW*MAPH];
    float height[MAPW*MAPH];
    float susSed[MAPW*MAPH];
    float susSed2[MAPW*MAPH];
    float sandTemp[MAPW*MAPH];
//    float water[5*MAPW*MAPH]; //water
    fluid_t water[MAPW*MAPH];
    vec2f_t waterVel[MAPW*MAPH];//X/Y
    float foamLevel[MAPW*MAPH];
    struct{
    	unsigned int updateShadowMap: 1;
    }flags;
}map;

EM_JS(int, get_browser_width, (), {
  return window.innerWidth;
});

EM_JS(int, get_browser_height, (), {
  return window.innerHeight;
});

float lerp(float s, float e, float t){return s+(e-s)*t;}
float blerp(float c00, float c10, float c01, float c11, float tx, float ty){
    return lerp(lerp(c00, c10, tx), lerp(c01, c11, tx), ty);
}
void water_update(fluid_t* fluid, float g, float l, float A, float friction, float dTime){
    float* T = map.stone;
    fluid_t* f = fluid; //shorter name

    for(int y=1;y<MAPH-2;y++){
        for(int x=1;x<MAPW-2;x++){
        	int yw = y*MAPW;
        	//if fluid depth is really low then the remaining fluid is cleared and the rest of the calculations skipped
        	if(f[x+yw].depth < 0.0001f){
        		f[x+yw].depth = 0.f;
        		f[x+yw].down = 0.f;
        		f[x+yw].left = 0.f;
        		f[x+yw].right = 0.f;
        		f[x+yw].up = 0.f;
        		continue;
        	}else{
        		f[x+yw].depth -= 0.0001f;
        		f[x+yw].down  -= 0.0001f;
        		f[x+yw].left  -= 0.0001f;
        		f[x+yw].right -= 0.0001f;
        		f[x+yw].up    -= 0.0001f;
        	}

            f[x+yw].right = max(f[x+yw].right*friction + (f[x+yw].depth+T[x+y*MAPW]+map.sand[x+y*MAPW] -f[(x+1)+(yw)].depth     -T[(x+1)+y*MAPW]-map.sand[(x+1)+y*MAPW])     *dTime*A*g/l , 0.f); //höger
            f[x+yw].down  = max(f[x+yw].down*friction  + (f[x+yw].depth+T[x+y*MAPW]+map.sand[x+y*MAPW] -f[x+(yw-1*MAPW)].depth  -T[(x)+(y-1)*MAPW]-map.sand[(x)+(y-1)*MAPW]) *dTime*A*g/l , 0.f); //upp
            f[x+yw].left  = max(f[x+yw].left*friction  + (f[x+yw].depth+T[x+y*MAPW]+map.sand[x+y*MAPW] -f[(x-1)+yw].depth       -T[(x-1)+y*MAPW]-map.sand[(x-1)+y*MAPW])     *dTime*A*g/l , 0.f); //vänster
            f[x+yw].up    = max(f[x+yw].up*friction    + (f[x+yw].depth+T[x+y*MAPW]+map.sand[x+y*MAPW] -f[x+(yw+1*MAPW)].depth  -T[(x)+(y+1)*MAPW]-map.sand[(x)+(y+1)*MAPW]) *dTime*A*g/l , 0.f); //ner
            
            
            //make sure flow out of cell isn't greater than inflow + existing fluid
            if(f[x+yw].depth - (f[x+yw].right+f[x+yw].down+f[x+yw].left+f[x+yw].up) < 0){
                float K = min(f[x+yw].depth*l*l/((f[x+yw].right+f[x+yw].down+f[x+yw].left+f[x+yw].up)*dTime) , 1.f);
                f[x+yw].right *= K;
                f[x+yw].down *= K;
                f[x+yw].left *= K;
                f[x+yw].up *= K;
            }
        }
    }
    //border conditions
    for(int y=0;y<MAPH;y++){
    	f[(MAPW-2)+y*MAPW].right = 0;
    	f[2+y*MAPW].left = 0;
    }
    for(int x=0;x<MAPW;x++){
    	f[x+2*MAPW].down = 0;
    	f[x+(MAPH-2)*MAPW].up = 0;
    }
    
    //update depth
    for(int y=1;y<MAPH-2;y++){
        for(int x=1;x<MAPW-2;x++){
            float deltaV = (f[(x-1)+(y)*MAPW].right+f[(x)+(y+1)*MAPW].down+f[(x+1)+(y)*MAPW].left+f[(x)+(y-1)*MAPW].up - (f[(x)+(y)*MAPW].right+f[(x)+(y)*MAPW].down+f[(x)+(y)*MAPW].left+f[(x)+(y)*MAPW].up))*dTime;
            if(deltaV < 0.0001f && deltaV > -0.0001) continue;
            f[(x)+(y)*MAPW].depth = max(f[(x)+(y)*MAPW].depth + deltaV/(l*l), 0.f);

            //calculate velocity
			map.waterVel[x+y*map.w].x = ( f[(x-1)+(y)*MAPW].right - f[(x)+(y)*MAPW].left + f[(x)+(y)*MAPW].right - f[(x+1)+(y)*MAPW].left) / (2); //X
			map.waterVel[x+y*map.w].y = ( f[(x-1)+(y)*MAPW].down - f[(x)+(y)*MAPW].up + f[(x)+(y)*MAPW].down - f[(x+1)+(y)*MAPW].up) / (2); //Y

        }
    }
    
    //erosion/deposition of sediment
    // https://ranmantaru.com/blog/2011/10/08/water-erosion-on-heightmap-terrain/
    // https://old.cescg.org/CESCG-2011/papers/TUBudapest-Jako-Balazs.pdf
    // https://hal.inria.fr/file/index/docid/402079/filename/FastErosion_PG07.pdf chapter 3.3
    memcpy(map.sandTemp,map.sand,sizeof(map.sand));
    for(int y=1;y<MAPH-2;y++){
		for(int x=1;x<MAPW-2;x++){
			//calculate tilt https://math.stackexchange.com/questions/1044044/local-tilt-angle-based-on-height-field
			float tiltX = (T[(x+1)+y*map.w]+map.sand[(x+1)+y*map.w] - T[(x-1+y*map.w)]-map.sand[(x-1+y*map.w)]) / 2.f;
			float tiltY = (T[(x)+(y+1)*map.w]+map.sand[(x)+(y+1)*map.w] - T[(x+(y-1)*map.w)]-map.sand[(x+(y-1)*map.w)]) / 2.f;
			float sinTheta = (sqrt(tiltX*tiltX + tiltY*tiltY)) / (sqrt(1 + tiltX*tiltX + tiltY*tiltY));
			float vel = sqrt(map.waterVel[x+y*map.w].x*map.waterVel[x+y*map.w].x + map.waterVel[x+y*map.w].y*map.waterVel[x+y*map.w].y);
			float Kc = 0.1f; //sediment capacity constant
			float Ks = 0.1f; //sediment dissolving constant
			float Kd = 0.02f; //sediment deposition constant

			//sediment transport capacity
			float C = Kc * sinTheta * vel;


			if(C > map.susSed[x+y*map.w]){//Pickup sand
				//Make sure the sand picked up is not more than any nearby tile, creating a pit
				float localMin = T[x+y*map.w]+map.sand[x+y*map.w];
				for(int i=-1;i<2;i++){
					for(int j=-1;j<2;j++){
						localMin = min(localMin, T[(x+j)+(y+i)*map.w]+map.sand[(x+j)+(y+i)*map.w]);
					}
				}
				float pickedUp = min(Ks*(C-map.susSed[x+y*map.w]), map.sand[x+y*map.w]);
				map.sandTemp[x+y*map.w] -= pickedUp;
				map.susSed[x+y*map.w] += pickedUp;
				map.water[x+y*map.w].depth += pickedUp;
			}else if(C <= map.susSed[x+y*map.w]){//Drop sand
				//Make sure dropped sand does not create a peak by not allowing to drop on the local highest point
				float localMax = T[x+y*map.w]+map.sand[x+y*map.w];
				for(int i=-1;i<2;i++){
					for(int j=-1;j<2;j++){
						localMax = max(localMax, T[(x+j)+(y+i)*map.w]+map.sand[(x+j)+(y+i)*map.w]);
					}
				}
				if(localMax - T[x+y*map.w]+map.sand[x+y*map.w] < 0.001f) continue;
				float dropped = Kd*(map.susSed[x+y*map.w]-C);
				map.sandTemp[x+y*map.w] += dropped;
				map.susSed[x+y*map.w] -= dropped;
				map.water[x+y*map.w].depth -= dropped;
				map.flags.updateShadowMap = 1;
			}
			if(map.water[x+y*map.w].depth < 0.001f){
				float dropped = (map.susSed[x+y*map.w]);
				map.sandTemp[x+y*map.w] += dropped;
				map.susSed[x+y*map.w] = 0;
				map.water[x+y*map.w].depth = 0;
				map.flags.updateShadowMap = 1;
			}

		}
    }

    //Make sand move towards a max slope
    int jMax, jMin, iMax, iMin;
    for(int y=2;y<MAPH-1;y++){
    	iMin = (y==2) ? 0 : -1;
		iMax = (y==MAPW-2) ? 0 : 1;
		for(int x=2;x<MAPW-1;x++){
			jMin = (x==2) ? 0 : -1;
			jMax = (x==MAPW-2) ? 0 : 1;
			for(int i= iMin; i <= iMax; i++){
				for(int j = jMin; j <= jMax; j++){
					float slope = T[x+y*map.w]+map.sandTemp[x+y*map.w] - T[(x+j)+(y+i)*map.w]-map.sandTemp[(x+j)+(y+i)*map.w];
					if(slope > 1.f){
						float sandDiff = min(slope, map.sandTemp[x+y*map.w]);//map.sandTemp[x+y*map.w] - map.sandTemp[(x+j)+(y+i)*map.w];
//						if(map.water[(x+j)+(y+i)*map.w].depth < 0.1f || sandDiff > 8.f){
							map.sandTemp[(x+j)+(y+i)*map.w] += sandDiff/2.f;
							map.sandTemp[x+y*map.w] -= sandDiff/2.f;
//						}
					}
				}

			}
		}
	}
    memcpy(map.sand,map.sandTemp,sizeof(map.sand));
    memset(map.susSed2,0,sizeof(map.susSed2));
    for(int y=1;y<MAPH-2;y++){
		for(int x=1;x<MAPW-2;x++){
			if(map.water[x+y*map.w].depth < 0.001f) continue;
			//transport
			float dX = -(map.waterVel[x+y*map.w].x*dTime);
			float dY = (map.waterVel[x+y*map.w].y*dTime);
			float foamTransported = 0;
			if     (dX >= 0 && dY >= 0) foamTransported = blerp(map.susSed[(x+(int)dX)+(y+(int)dY)*map.w] ,map.susSed[(x+(int)dX+1)+(y+(int)dY)*map.w] ,map.susSed[(x+(int)dX)+(y+(int)dY+1)*map.w] ,map.susSed[(x+(int)dX+1)+(y+(int)dY+1)*map.w] ,(dX-(int)dX),(dY-(int)dY));
			else if(dX >= 0 && dY <  0) foamTransported = blerp(map.susSed[(x+(int)dX)+(y+(int)dY)*map.w] ,map.susSed[(x+(int)dX+1)+(y+(int)dY)*map.w] ,map.susSed[(x+(int)dX)+(y+(int)dY-1)*map.w] ,map.susSed[(x+(int)dX+1)+(y+(int)dY-1)*map.w] ,(dX-(int)dX),((int)dY-dY));
			else if(dX <  0 && dY >= 0) foamTransported = blerp(map.susSed[(x+(int)dX)+(y+(int)dY)*map.w] ,map.susSed[(x+(int)dX-1)+(y+(int)dY)*map.w] ,map.susSed[(x+(int)dX)+(y+(int)dY+1)*map.w] ,map.susSed[(x+(int)dX-1)+(y+(int)dY+1)*map.w] ,((int)dX-dX),(dY-(int)dY));
			else if(dX <  0 && dY <  0) foamTransported = blerp(map.susSed[(x+(int)dX)+(y+(int)dY)*map.w] ,map.susSed[(x+(int)dX-1)+(y+(int)dY)*map.w] ,map.susSed[(x+(int)dX)+(y+(int)dY-1)*map.w] ,map.susSed[(x+(int)dX-1)+(y+(int)dY-1)*map.w] ,((int)dX-dX),((int)dY-dY));

			map.susSed2[(x)+(y)*map.w] += foamTransported;
		}
    }
    memcpy(map.susSed,map.susSed2,sizeof(map.susSed));

}




void loadHeightMap(const char* path){
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
            map.stone[x+y*w]  = 70.f * (float)(r+g+b)/(float)(3 * 255) + 1.f;
            map.argb[x+y*w].r = r;
            map.argb[x+y*w].g = g;
            map.argb[x+y*w].b = b;
        }
    }
    
    SDL_UnlockSurface(bitmap);
    SDL_FreeSurface (bitmap);
}


void loadColorMap(const char* path){
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
            map.argb[x+y*w].r = r;
            map.argb[x+y*w].g = g;
            map.argb[x+y*w].b = b;
        }
    }

    SDL_UnlockSurface(bitmap);
    SDL_FreeSurface (bitmap);
}


vec2f_t world2screen(float x, float y, camera_t* camPtr){
	float sinAlpha = sin(camPtr->rot);
	float cosAlpha = cos(camPtr->rot);

	//scale for zoom level
	float xs = x / camPtr->zoom;
	float ys = y / camPtr->zoom;
	//project

	//rotate 
	float xw =  cosAlpha*(xs+(camPtr->x-614.911)) + sinAlpha*(ys+(camPtr->y-119.936)) - (camPtr->x-614.911);
	float yw = -sinAlpha*(xs+(camPtr->x-614.911)) + cosAlpha*(ys+(camPtr->y-119.936)) - (camPtr->y-119.936);
	//offset for camPtrera position

	xs = xw + camPtr->x;
	ys = yw + camPtr->y;
	
	xw = ((xs-ys)/sqrt(2));
	yw = ((xs+ys)/sqrt(6));

	vec2f_t retVal = {xw,yw};

	return retVal;
}

vec2f_t screen2world(float x,float y, camera_t* camPtr){
	float sinAlpha = sin(camPtr->rot);
	float cosAlpha = cos(camPtr->rot);
	float sq2d2 = sqrt(2)/2;
	float sq6d2 = sqrt(6)/2;
	//transform screen coordinates to world coordinates 
	float xw = sq2d2*x+sq6d2*y;
	float yw = sq6d2*y-sq2d2*x;
	xw = xw - camPtr->x;
	yw = yw - camPtr->y;


	//rotate view
	float xwr = cosAlpha*(xw+(camPtr->x-614.911)) - sinAlpha*(yw+(camPtr->y-119.936)) - (camPtr->x-614.911);
	float ywr = sinAlpha*(xw+(camPtr->x-614.911)) + cosAlpha*(yw+(camPtr->y-119.936)) - (camPtr->y-119.936);
//616 121
	//apply zoom
	xw = xwr*camPtr->zoom;
	yw = ywr*camPtr->zoom;
	
	vec2f_t retVal = {xw,yw};
	//double xw = (cosAlpha*(cam.x+sq2d2*x+sq6d2*y-Map.w/2) - sinAlpha*(cam.y+sq6d2*y-sq2d2*x-Map.h/2)+Map.w/2)*cam.zoom;
	//double yw = (sinAlpha*(cam.x+sq2d2*x+sq6d2*y-Map.w/2) + cosAlpha*(cam.y+sq6d2*y-sq2d2*x-Map.h/2)+Map.h/2)*cam.zoom;
	return retVal;
}





void boxBlurT_4(float *source, float *target,int w, int h, int r){
    float iarr = 1 / (float)(r+r+1);
    for(int i=0; i<w; i++) {
        int ti = i, li = ti, ri = ti+r*w;
        float fv = source[ti], lv = source[ti+w*(h-1)], val = (r+1)*fv;
        for(int j=0; j<r; j++) val += source[ti+j*w];
        for(int j=0  ; j<=r ; j++) { val += source[ri] - fv     ;  target[ti] = ROUNDF(val*iarr);  ri+=w; ti+=w; }
        for(int j=r+1; j<h-r; j++) { val += source[ri] - source[li];  target[ti] = ROUNDF(val*iarr);  li+=w; ri+=w; ti+=w; }
        for(int j=h-r; j<h  ; j++) { val += lv      - source[li];  target[ti] = ROUNDF(val*iarr);  li+=w; ti+=w; }
    }
}

void boxBlurH_4(float *source, float *target,int w, int h, int r){
    float iarr = 1 / (float)(r+r+1);
    for(int i=0; i<h; i++) {
        int ti = i*w, li = ti, ri = ti+r;
        float fv = source[ti], lv = source[ti+w-1], val = (r+1)*fv;
        for(int j=0; j<r; j++) val += source[ti+j];
        for(int j=0  ; j<=r ; j++) { val += source[ri++] - fv       ;   target[ti++] = ROUNDF(val*iarr); }
        for(int j=r+1; j<w-r; j++) { val += source[ri++] - source[li++];   target[ti++] = ROUNDF(val*iarr); }
        for(int j=w-r; j<w  ; j++) { val += lv        - source[li++];   target[ti++] = ROUNDF(val*iarr); }
    }
}

void boxBlur_4 (float *source, float *target,int source_lenght, int w, int h, int r) {
    for(int i=0; i<source_lenght; i++) target[i] = source[i];
    boxBlurH_4(target, source, w, h, r);
    boxBlurT_4(source, target, w, h, r);
}

void gaussBlur_4(float *source, float *target,int source_lenght, int w, int h, int r) {
    int n = 3;
	float bxs[n];
	float wIdeal = sqrt((12*r*r/n)+1);  // Ideal averaging filter width
    int wl = floor(wIdeal);  if(wl%2==0) wl--;
    int wu = wl+2;

    float mIdeal = (12*r*r - n*wl*wl - 4*n*wl - 3*n)/(-4*wl - 4);
    float m = ROUNDF(mIdeal);
    // var sigmaActual = std::sqrt( (m*wl*wl + (n-m)*wu*wu - n)/12 );

	for(int i=0; i<n;i++){
		if(i<m){ bxs[i] = wl;}
		else {   bxs[i] = wu;}
	}
    boxBlur_4 (source, target, source_lenght, w, h, (int)(bxs[0]-1)/2.f);
    boxBlur_4 (target, source, source_lenght, w, h, (int)(bxs[1]-1)/2.f);
    boxBlur_4 (source, target, source_lenght, w, h, (int)(bxs[2]-1)/2.f);
}
