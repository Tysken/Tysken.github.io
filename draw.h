/*
 * draw.h
 *
 *  Created on: Jan 10, 2021
 *      Author: david
 */

#ifndef DRAW_H_
#define DRAW_H_

typedef struct{
	uint8_t b,g,r,a;
}argb_t;

typedef struct{
	SDL_Texture * Texture;
	void * mPixels;
	uint32_t * pixels;
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


#endif /* DRAW_H_ */
