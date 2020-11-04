#include <functional>

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

class texture{
public:
	SDL_Texture * Texture;
	void * mPixels;
	Uint32 * pixels;
	int w; 
	int h;
	int pitch;
	
	texture(int width, int height)
	{
		//setup texture and array of pixels
		w = width;
		h = height;
		pitch = w * sizeof(Uint32);
		pixels = new Uint32[w * h];
		mPixels = NULL;
	}
	
	void set(int x, int y, Uint8 r, Uint8 g, Uint8 b, Uint8 a){
		pixels[x + y * w] = (a << 24) | (r << 16) | (g << 8) | (b);
	}
	
	void init(bool transparent){
		Texture = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
		if(transparent){
			SDL_SetTextureBlendMode(Texture,SDL_BLENDMODE_BLEND);
		}
	}
	
	void lock(void){
		mPixels = NULL;
		SDL_LockTexture(Texture,NULL,&mPixels, &pitch);
		pixels = (Uint32*)mPixels;
	}
	
	void unlock(void){
		SDL_UnlockTexture(Texture);
	}
	
};

texture scrTexture(windowSizeX,windowSizeY);
texture rendTexture(rendererSizeX,rendererSizeY);
texture hudTexture(windowSizeX,windowSizeY);

// an example of something we will control from the javascript side
bool background_is_black = true;

// the function called by the javascript code
extern "C" void EMSCRIPTEN_KEEPALIVE toggle_background_color() { background_is_black = !background_is_black; }

std::function<void()> loop;
void main_loop() { loop(); }

int main()
{

    //SDL_CreateWindowAndRenderer(640, 480, 0, &window, nullptr);



	if ( SDL_Init( SDL_INIT_EVERYTHING ) == -1 )	{}
	

	window = SDL_CreateWindow( "Server", 10, 10, windowSizeX, windowSizeY, 0 );
	if ( window == nullptr )	{}

	renderer = SDL_CreateRenderer( window, -1, 0 );
	if ( renderer == nullptr )	{}
	

	// Set size of renderer 
	SDL_RenderSetLogicalSize( renderer, windowSizeX, windowSizeY );
	//Set blend mode of renderer
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	// Set color of renderer to black
	SDL_SetRenderDrawColor( renderer, 100, 0, 0, 255 );
	
	rendTexture.init(false); //initiate screen texture
	hudTexture.init(true); //initiate HUD texture

    loop = [&]
    {
        // move a vertex
        const uint32_t milliseconds_since_start = SDL_GetTicks();
        const uint32_t milliseconds_per_loop = 3000;

        // Clear the window and make it all black
        SDL_SetRenderDrawColor( renderer, 0, 0, 0, 255 );
        SDL_RenderClear( renderer );
        
		rendTexture.lock();

        for(int y=0;y<100;y++){
            for(int x=0;x<100;x++){
                Uint32 rgb = x^y;
                rendTexture.pixels[x + y*rendTexture.w] = rgb;	
            }
        }
    	rendTexture.unlock();

		rendTexture.lock();

        for(int y=0;y<100;y++){
            for(int x=0;x<100;x++){
                Uint32 rgb = x^y;
                hudTexture.pixels[x + y*rendTexture.w] = rgb;	
            }
        }
    	rendTexture.unlock();
        
        SDL_Rect test;
        test.x = 0;
        test.y = 600;
        test.w = 600;
        test.h = 200;

            
        SDL_RenderCopy(renderer,rendTexture.Texture,NULL,&test); //copy screen texture to renderer
        SDL_RenderCopy(renderer,hudTexture.Texture,NULL,NULL); //copy hud texture to renderer

        // Render the changes above
        SDL_RenderPresent( renderer);

 //       SDL_GL_SwapWindow(window);
    };

    emscripten_set_main_loop(main_loop, 0, true);

    return EXIT_SUCCESS;
}
