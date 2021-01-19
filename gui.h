/*
 * gui.h
 *
 *  Created on: Jan 9, 2021
 *      Author: david
 */

#ifndef GUI_H_
#define GUI_H_

#include "draw.h"

#define MAX_NO_WIDGETS 32

enum keyState{
	KEY_PRESSED,
	KEY_HELD,
	KEY_RELEASED
};


typedef struct{
	struct{
        int x;
        int y;
        int offsetX; //an offset is needed if the html canvas size isn't 1:1 with the SDL window size
        int offsetY;

        enum keyState left;
		enum keyState right;
	}mouse;
}input_t;

enum colorEnum{
	ARGB_DARK_BLUE       =  4,
	ARGB_DARK_BLUE_LIGHT =  5,
	ARGB_GREEN           =  6,
	ARGB_GREEN_LIGHT     =  7,
	ARGB_BLUE            =  8,
	ARGB_BLUE_LIGHT      =  9,
	ARGB_RED             = 10,
	ARGB_RED_LIGHT       = 11
};

//https://flatuicolors.com/palette/defo
argb_t colorScheme[13] =  {{ (Uint8)208,  (Uint8)178, (Uint8)131, 0},  //Blue
						   { (Uint8)182,  (Uint8)218, (Uint8)149, 0},  //Green
						   { (Uint8)117,  (Uint8)230, (Uint8)242, 0},  //Yellow
						   { (Uint8)128,  (Uint8)133, (Uint8)220, 0},  //Red
						   {  (Uint8)80,   (Uint8)62,  (Uint8)44, 0},  //Dark blue   ---Background
						   {  (Uint8)94,   (Uint8)73,  (Uint8)52, 0},  //Dark blue light
						   {  (Uint8)96,  (Uint8)174,  (Uint8)39, 0},  //Green
						   { (Uint8)113,  (Uint8)204,  (Uint8)46, 0},  //Green light
						   { (Uint8)185,  (Uint8)128,  (Uint8)41, 0},  //Blue
						   { (Uint8)219,  (Uint8)152,  (Uint8)52, 0},  //Blue light
						   {  (Uint8)43,   (Uint8)57, (Uint8)192, 0},  //Red
						   {  (Uint8)60,   (Uint8)76, (Uint8)231, 0}, //Red light
						   { (Uint8)199,  (Uint8)195, (Uint8)189, 0}}; //light gray ---Highlights



typedef struct{
	int x,y,w,h;
	char text[20];
	argb_t color;
	enum colorEnum colorFirst;
	enum colorEnum colorSecond;
	enum{
		PRESSBUTTON_IDLE,
		PRESSBUTTON_HOVER,
		PRESSBUTTON_PRESSED,
		PRESSBUTTON_RELEASED
	}state;
	void (*callbackFunction)(void);
}pressButton_t;

typedef struct{
	int x,y,w,h;
	argb_t emptyColor;
	argb_t fillColor;
	argb_t grabColor;
	enum{
		SLIDER_TYPE_FLOAT,
		SLIDER_TYPE_INT
	}type;
	enum{
		SLIDER_IDLE,
		SLIDER_HELD
	}state;
	int maxi, mini, defaulti;
	float maxf, minf, defaultf;
	float* floatPtr;
	int* intPtr;
	char text[20];
}slider_t;

typedef enum{
	PRESSBUTTON,
	SLIDER,
	OTHER
}widgetType;

typedef struct{
	widgetType type;
	void* widgetPtr;
}widget_t;


struct{
	uint32_t nWidgets;
	widget_t widgetList[MAX_NO_WIDGETS];
}gui;

void gui_createSlider(char* text, int x, int y, int w, int h, int type, void* maxVal, void* minVal, void* valPtr){

	slider_t* wPtr = (slider_t*)malloc(sizeof(slider_t));
	//copy text
	for(int i=0;i<20;i++){
		wPtr->text[i] = text[i];
	}
	wPtr->x = x;
	wPtr->y = y;
	wPtr->w = w;
	wPtr->h = h;
	wPtr->type = type;
	if(type == SLIDER_TYPE_INT){
		wPtr->maxi   = *((int*)maxVal);
		wPtr->mini   = *((int*)minVal);
		wPtr->intPtr =  ((int*)valPtr);
	}else if(type == SLIDER_TYPE_FLOAT){ //float
		wPtr->maxf     = *((float*)maxVal);
		wPtr->minf     = *((float*)minVal);
		wPtr->floatPtr =  ((float*)valPtr);
	}

	wPtr->emptyColor = colorScheme[12]; //gray
	wPtr->fillColor  = colorScheme[6];
	wPtr->grabColor  = colorScheme[7];
	wPtr->state = SLIDER_IDLE;

	gui.widgetList[gui.nWidgets].widgetPtr = wPtr;
	gui.widgetList[gui.nWidgets].type = SLIDER;
	gui.nWidgets++;
}

void gui_createPressButton(char* text, int x, int y, int w, int h, void (*callback)(void)){

	pressButton_t* wPtr = (pressButton_t*)malloc(sizeof(pressButton_t));
	wPtr->callbackFunction = callback;
	wPtr->x = x;
	wPtr->y = y;
	wPtr->w = w;
	wPtr->h = h;
	if(gui.nWidgets == 0){
		wPtr->colorFirst  = ARGB_BLUE;
		wPtr->colorSecond = ARGB_BLUE_LIGHT;
	}else if(gui.nWidgets == 1){
		wPtr->colorFirst  = ARGB_GREEN;
		wPtr->colorSecond = ARGB_GREEN_LIGHT;
	}else{
		wPtr->colorFirst  = ARGB_RED;
		wPtr->colorSecond = ARGB_RED_LIGHT;
	}
	wPtr->color = colorScheme[wPtr->colorFirst];
	wPtr->state = PRESSBUTTON_IDLE;
	//copy text
	for(int i=0;i<20;i++){
		wPtr->text[i] = text[i];
	}
	gui.widgetList[gui.nWidgets].widgetPtr = wPtr;
	gui.widgetList[gui.nWidgets].type = PRESSBUTTON;


	gui.nWidgets++;

}

void gui_handleGUI(int mouseX, int mouseY, input_t* inputPtr){
//	printf("%d %d \n",mouseX,mouseY);
	for(int i=0;i<MAX_NO_WIDGETS;i++){
		if(gui.widgetList[i].widgetPtr != NULL){
			switch(gui.widgetList[i].type){
			case PRESSBUTTON:
			{
				pressButton_t* buttonPtr = ((pressButton_t*)(gui.widgetList[i].widgetPtr));
				int x = buttonPtr->x;
				int y = buttonPtr->y;
				int w = buttonPtr->w;
				int h = buttonPtr->h;
				//check if mouse is inside button
				if(mouseX > x && mouseX < x+w && mouseY > y && mouseY < y+h){
					buttonPtr->state = PRESSBUTTON_HOVER;
					buttonPtr->color = colorScheme[buttonPtr->colorFirst];
					if(inputPtr->mouse.left == KEY_HELD){
						buttonPtr->state = PRESSBUTTON_PRESSED;
						buttonPtr->color = colorScheme[buttonPtr->colorSecond];
					}else if(inputPtr->mouse.left == KEY_RELEASED){
						buttonPtr->state = PRESSBUTTON_RELEASED;
						buttonPtr->color = colorScheme[buttonPtr->colorFirst];
						buttonPtr->callbackFunction(); //run linked callback function
					}
				}else if(buttonPtr->state == PRESSBUTTON_HOVER){
					buttonPtr->state = PRESSBUTTON_IDLE;
					buttonPtr->color = colorScheme[buttonPtr->colorFirst];
				}else{
					buttonPtr->color = colorScheme[buttonPtr->colorFirst];
				}
			}	break;
			case SLIDER:
			{
				slider_t* sliderPtr = ((slider_t*)(gui.widgetList[i].widgetPtr));
				int x = sliderPtr->x;
				int y = sliderPtr->y;
				int w = sliderPtr->w;
				int h = sliderPtr->h;
				//check if mouse is inside slider
				if(mouseX > x && mouseX < x+w && mouseY > y && mouseY < y+h){
					if(inputPtr->mouse.left == KEY_PRESSED){
						sliderPtr->state = SLIDER_HELD;
					}
				}

				if(inputPtr->mouse.left == KEY_RELEASED && sliderPtr->state == SLIDER_HELD){
					//release slider
					sliderPtr->state = SLIDER_IDLE;
				}

				if(sliderPtr->state == SLIDER_HELD){
					//move slider to pointer xPos
					if(sliderPtr->type == SLIDER_TYPE_INT){
						*(sliderPtr->intPtr)   = (float)(sliderPtr->maxi - sliderPtr->mini) * min(((float)(max(mouseX - x,0)) / (float)(w)),1.f);
					}else if(sliderPtr->type == SLIDER_TYPE_FLOAT){
						*(sliderPtr->floatPtr) = sliderPtr->minf + (float)(sliderPtr->maxf - sliderPtr->minf) * min(((float)(max(mouseX - x,0)) / (float)(w)),1.f);
					}
				}

			}	break;
			default:

				break;
			}
		}
	}
}

void gui_drawGUI(sdlTexture* texturePtr){
	for(int i=0;i<MAX_NO_WIDGETS;i++){
			if(gui.widgetList[i].widgetPtr != NULL){
				switch(gui.widgetList[i].type){
				case PRESSBUTTON:
				{
					pressButton_t* buttonPtr = ((pressButton_t*)(gui.widgetList[i].widgetPtr));
					int bx = buttonPtr->x;
					int by = buttonPtr->y;
					int bw = buttonPtr->w;
					int bh = buttonPtr->h;
					//draw pressbutton
					for(int y=by;y<by+bh;y++){
						for(int x=bx;x<bx+bw;x++){
							texturePtr->pixels[x+y*texturePtr->w] = *((uint32_t*)(&(buttonPtr->color))); //recast argb as uint32
						}
					}
					int txtLength = strlen(buttonPtr->text);
					print(texturePtr, buttonPtr->text, bx+bw/2-(txtLength*8/2), bh/2-4+by);

				}	break;
				case SLIDER:
				{
					slider_t* wPtr = ((slider_t*)(gui.widgetList[i].widgetPtr));
					int sx = wPtr->x;
					int sy = wPtr->y;
					int sw = wPtr->w;
					int sh = wPtr->h;
					int endFill;
					//find what portion is filled
					if(wPtr->type == SLIDER_TYPE_INT){
						endFill = (int)((float)sw * ((float)(*(wPtr->intPtr)   - wPtr->mini) / (float)(wPtr->maxi - wPtr->mini)));
					}else if(wPtr->type == SLIDER_TYPE_FLOAT){
						endFill = (int)((float)sw * ((float)(*(wPtr->floatPtr) - wPtr->minf) / (float)(wPtr->maxf - wPtr->minf)));
					}
					//draw slider
					for(int y=sy;y<sy+sh;y++){
						for(int x=sx;x<sx+endFill;x++){
							texturePtr->pixels[x+y*texturePtr->w] = *((uint32_t*)(&(wPtr->fillColor))); //recast argb as uint32
						}
						for(int x=sx+endFill;x<sx+sw;x++){
							texturePtr->pixels[x+y*texturePtr->w] = *((uint32_t*)(&(wPtr->emptyColor))); //recast argb as uint32
						}
					}
					//draw grabber
					for(int y=sy-4;y<=sy+sh+4;y++){
						for(int x=sx+endFill-4;x<=sx+endFill+4;x++){
							texturePtr->pixels[x+y*texturePtr->w] = *((uint32_t*)(&(wPtr->grabColor))); //recast argb as uint32
						}
					}

					char printBuffer[30];
					if(wPtr->type == SLIDER_TYPE_INT){
						sprintf(printBuffer, "%s: %d", wPtr->text, *(wPtr->intPtr));
					}else if(wPtr->type == SLIDER_TYPE_FLOAT){
						sprintf(printBuffer, "%s: %.2f", wPtr->text, *(wPtr->floatPtr));
					}
					print(texturePtr, printBuffer, sx, sy - sh - 10);

				}	break;
				default:

					break;
				}
			}
		}
}

#endif /* GUI_H_ */
