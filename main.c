#include <stdbool.h>
#include <emscripten.h>
#include <SDL.h>
#include <SDL2/SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengles2.h>

SDL_Window * window;
SDL_Renderer * renderer;

const int windowSizeX = 600;
const int windowSizeY = 800;
const int rendererSizeX = 600;
const int rendererSizeY = 600;

uint32_t g_time_ms;
uint32_t g_dtime_ms;

struct{
	struct{
		int x;
		int y;
		uint32_t state;
	}mouse;
}input;

typedef struct{
	SDL_Texture * Texture;
	void * mPixels;
	Uint32 * pixels;
	int w; 
	int h;
	int pitch;
}sdlTexture;

uint32_t rendTexturePixels[rendererSizeX*rendererSizeY];
uint32_t hudTexturePixels[windowSizeX*windowSizeY];

sdlTexture rendTexture = {NULL,NULL,rendTexturePixels,rendererSizeX,rendererSizeY,rendererSizeX*sizeof(uint32_t)};
sdlTexture hudTexture = {NULL,NULL,hudTexturePixels,windowSizeX,windowSizeY,windowSizeX*sizeof(uint32_t)};



void render(){
        for(int y=0;y<rendTexture.h;y++){
            for(int x=0;x<rendTexture.w;x++){
                Uint32 rgb = (x*4-g_time_ms/30)^(y*4-g_time_ms/30);
                rendTexture.pixels[x + y*rendTexture.w] = rgb;	
            }
        }

		//draw mouse
		for(int y=-10;y<10;y++){
			for(int x=-10;x<10;x++){
				uint32_t rgb = (255 << 16) | (255 << 8) | (255);
				if(input.mouse.x-10 > 0 && input.mouse.x + 10 < rendTexture.w && 
				   input.mouse.y-10 > 0 && input.mouse.y + 10 < rendTexture.h ){
					rendTexture.pixels[(input.mouse.x+x)+(input.mouse.y+y)*rendTexture.w] = rgb;
				   }
				printf("hej");
			}
		}
}

void renderHud(){
        for(int y=0;y<hudTexture.h;y++){
            for(int x=0;x<hudTexture.w;x++){
                Uint32 rgb = x*1000;
				if(y<600) rgb = 0;
                hudTexture.pixels[x + y*hudTexture.w] = rgb;	
            }
        }
}



void updateInput(){
	input.mouse.state = SDL_GetMouseState(&input.mouse.x,&input.mouse.y);
	
}

void process(){

}

void loop(){
	
		g_dtime_ms = g_time_ms; //backup time
        g_time_ms = SDL_GetTicks(); //get time
		g_dtime_ms = g_time_ms - g_dtime_ms; //get delta time

	updateInput();

	process();

        // Clear the window and make it all black
        SDL_SetRenderDrawColor( renderer, 0, 0, 0, 255 );
        SDL_RenderClear( renderer );
        
//		rendTexture.lock();
		SDL_LockTexture(rendTexture.Texture,NULL,&(rendTexture.mPixels), &(rendTexture.pitch));
		rendTexture.pixels = (uint32_t*)rendTexture.mPixels;
		render();
		SDL_UnlockTexture(rendTexture.Texture);
//    	rendTexture.unlock();

		SDL_LockTexture(hudTexture.Texture,NULL,&(hudTexture.mPixels), &(hudTexture.pitch));
		hudTexture.pixels = (uint32_t*)hudTexture.mPixels;
		renderHud();
		SDL_UnlockTexture(hudTexture.Texture);
        
        SDL_Rect test;
        test.x = 0;
        test.y = 0;
        test.w = 600;
        test.h = 600;

            
        SDL_RenderCopy(renderer,rendTexture.Texture,NULL,&test); //copy screen texture to renderer
        SDL_RenderCopy(renderer,hudTexture.Texture,NULL,NULL); //copy hud texture to renderer

        // Render the changes above
        SDL_RenderPresent( renderer);

 //       SDL_GL_SwapWindow(window);
    
}
void main_loop() { loop(); }

int main()
{

    //SDL_CreateWindowAndRenderer(640, 480, 0, &window, nullptr);



	if ( SDL_Init( SDL_INIT_EVERYTHING ) == -1 )	{}
	

	window = SDL_CreateWindow( "Server", 10, 10, windowSizeX, windowSizeY, 0 );
	if ( window == NULL )	{}

	renderer = SDL_CreateRenderer( window, -1, 0 );
	if ( renderer == NULL )	{}
	

	// Set size of renderer 
	SDL_RenderSetLogicalSize( renderer, windowSizeX, windowSizeY );
	//Set blend mode of renderer
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	// Set color of renderer to black
	SDL_SetRenderDrawColor( renderer, 100, 0, 0, 255 );
	
	//init rendTexture
	rendTexture.Texture = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, rendTexture.w, rendTexture.h);
	//init hudTexture as transparent
	hudTexture.Texture = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, hudTexture.w, hudTexture.h);
	SDL_SetTextureBlendMode(hudTexture.Texture,SDL_BLENDMODE_BLEND);
	


    emscripten_set_main_loop(main_loop, 0, true);

    return EXIT_SUCCESS;
}
