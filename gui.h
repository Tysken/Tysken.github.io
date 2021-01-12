/*
 * gui.h
 *
 *  Created on: Jan 9, 2021
 *      Author: david
 */

#ifndef GUI_H_
#define GUI_H_

#include "draw.h"

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

typedef enum{
	PRESSBUTTON,
	OTHER
}widgetType;

typedef struct{
	widgetType type;
	void* widgetPtr;
}widget_t;

#define MAX_NO_WIDGETS 32
widget_t widgetList[MAX_NO_WIDGETS];

void createWidget(widgetType type, int x, int y, int w, int h, char* text, void (*callback)(void)){
	static int nWidgets = 0;

	switch(type){
	case PRESSBUTTON:
	{
		pressButton_t* wPtr = (pressButton_t*)malloc(sizeof(pressButton_t));
		wPtr->callbackFunction = callback;
		wPtr->x = x;
		wPtr->y = y;
		wPtr->w = w;
		wPtr->h = h;
		wPtr->colorFirst  = ARGB_BLUE;
		wPtr->colorSecond = ARGB_BLUE_LIGHT;
		wPtr->color = colorScheme[wPtr->colorFirst];
		wPtr->state = PRESSBUTTON_IDLE;
		//copy text
		for(int i=0;i<20;i++){
			wPtr->text[i] = text[i];
		}
		widgetList[nWidgets].widgetPtr = wPtr;
		widgetList[nWidgets].type = PRESSBUTTON;


		nWidgets++;
	}	break;
	default:

		break;
	}
}

void handleGUI(int mouseX, int mouseY, input_t* inputPtr){
//	printf("%d %d \n",mouseX,mouseY);
	for(int i=0;i<MAX_NO_WIDGETS;i++){
		if(widgetList[i].widgetPtr != NULL){
			switch(widgetList[i].type){
			case PRESSBUTTON:
			{
				pressButton_t* buttonPtr = ((pressButton_t*)(widgetList[i].widgetPtr));
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
						buttonPtr->color = colorScheme[ buttonPtr->colorSecond];
					}else if(inputPtr->mouse.left == KEY_RELEASED){
						buttonPtr->state = PRESSBUTTON_RELEASED;
						buttonPtr->callbackFunction(); //run linked callback function
					}
				}else if(buttonPtr->state == PRESSBUTTON_HOVER){
					buttonPtr->state = PRESSBUTTON_IDLE;
				}
			}	break;
			default:

				break;
			}
		}
	}
}

void drawGUI(sdlTexture* texturePtr){
	for(int i=0;i<MAX_NO_WIDGETS;i++){
			if(widgetList[i].widgetPtr != NULL){
				switch(widgetList[i].type){
				case PRESSBUTTON:
				{
					pressButton_t* buttonPtr = ((pressButton_t*)(widgetList[i].widgetPtr));
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
					print(texturePtr, buttonPtr->text, bx+bw/2-(txtLength/2)*8, bh/2-4+by);

				}	break;
				default:

					break;
				}
			}
		}
}

#endif /* GUI_H_ */
