#include "main.h"



SDL_Window * window;
SDL_Renderer * renderer;

uint32_t g_canvasSizeX;
uint32_t g_canvasSizeY;


uint32_t g_time_ms;
uint32_t g_dtime_ms;




uint32_t rendTexturePixels[rendererSizeX*rendererSizeY];
uint32_t hudTexturePixels[hudSizeX*hudSizeY];
uint32_t frameBuffer[rendererSizeX*rendererSizeY];

sdlTexture rendTexture = {NULL,NULL,rendTexturePixels,rendererSizeX,rendererSizeY,rendererSizeX*sizeof(uint32_t)};
sdlTexture hudTexture = {NULL,NULL,hudTexturePixels,hudSizeX,hudSizeY,hudSizeX*sizeof(uint32_t)};

input_t input;

float g_windowScaleX = 0.5f;
float g_windowScaleY = 0.5f;

#if ENABLE_MULTITHREADING
atomic_int threadDone = 0;
#else
int threadDone = 0;
#endif

void generate_shadowmap(){

 //flush shadowmap
	memset(map.shadow, 0, sizeof((map.shadow)));


//		Calculate shadows by iterating over map diagonally like example below.
//		------- Save the highest tileheight in diagonal and decrease by 1 each step.
//		|6|3|1| If current tile is higher, save that one as new highest point.
//		|8|5|2| If not then that tile is in shadow.
//		|9|7|4|
//		------- ONLY WORKS ON SQUARE MAPS!!!

	int diagonalLines = (map.w + map.h) - 1;        //number of diagonal lines in map
	int midPoint = (diagonalLines / 2) + 1; //number of the diagonal that crosses midpoint of map
	int itemsInDiagonal = 0;                //stores number of tiles in a diagonal


	for(int diagonal = 1;diagonal <= diagonalLines;diagonal++){
		float terrainPeakHeight = 1;
		int x,y;
		if (diagonal <= midPoint) {
			itemsInDiagonal++;
			for (int item = 0; item < itemsInDiagonal; item++) {
				y = (diagonal - item) - 1;
				x = map.w-item-1;
				terrainPeakHeight -= 0.5f;
				if(terrainPeakHeight < map.height[x+y*map.w]){
					map.shadow[x+y*map.w] = -30;
					terrainPeakHeight = map.height[x+y*map.w];
				}
			}
		} else {
			itemsInDiagonal--;
			for (int item = 0; item < itemsInDiagonal; item++) {
				y = (map.h - 1) - item;
				x = diagonalLines - diagonal - item;
				terrainPeakHeight -= 0.5f;
				if(terrainPeakHeight < map.height[x+y*map.w]){
					map.shadow[x+y*map.w] = -30;
					terrainPeakHeight = map.height[x+y*map.w];
				}
			}
		}
	}


//ambient occlusion
//Calculate ambient occlusion using a box filter, optimized with technique found here: http://blog.ivank.net/fastest-gaussian-blur.html#results

	int r = 4;

	for(int i=0; i<map.h; i++) {
		int ti = i*map.w, li = ti, ri = ti+r;
		float fv = map.height[ti], lv = map.height[ti+map.w-1], val = (r+1)*fv;
		for(int j=0; j<r; j++) val += map.height[ti+j];
		for(int j=0  ; j<=r ; j++) {
			val += map.height[ri++] - fv       ;
			map.shadow[ti] += ROUNDF(val-(r+r+1)*map.height[ti-1]);
			ti++;
		}
		for(int j=r+1; j<map.w-r; j++) { val += map.height[ri++] - map.height[li++];
		map.shadow[ti] += ROUNDF(val-(r+r+1)*map.height[ti-1]);
		ti++;
		}
		for(int j=map.w-r; j<map.w  ; j++) {
			val += lv        - map.height[li++];
			map.shadow[ti] += ROUNDF(val-(r+r+1)*map.height[ti-1]);
			ti++;
		}
	}
	for(int i=0; i<map.w; i++) {
		int ti = i, li = ti, ri = ti+r*map.w;
		float fv = map.height[ti], lv = map.height[ti+map.w*(map.h-1)], val = (r+1)*fv;
		for(int j=0; j<r; j++) val += map.height[ti+j*map.w];
		for(int j=0  ; j<=r ; j++) { val += map.height[ri] - fv     ;
		map.shadow[ti] += ROUNDF(val-(r+r+1)*map.height[ti]);
		ri+=map.w; ti+=map.w;
		}
		for(int j=r+1; j<map.h-r; j++) {
			val += map.height[ri] - map.height[li];
			map.shadow[ti] += ROUNDF(val-(r+r+1)*map.height[ti]);
			li+=map.w; ri+=map.w; ti+=map.w;
		}
		for(int j=map.h-r; j<map.h  ; j++) {
			val += lv      - map.height[li];
			map.shadow[ti] += ROUNDF(val-(r+r+1)*map.height[ti]);
			li+=map.w; ti+=map.w;
		}
	}

	//Add light sources
	//Lava
//	for(int i=0;i<map.w*map.h;i++){
//		map.shadow[i] -= (short)(lavaHeight[i]);
//	}

	//smooth shadows
	boxBlur_4(map.shadow,map.shadow,map.w*map.h,map.w,map.h,1);
	//moother shadows
	boxBlur_4(map.shadow,map.shadowSoft,map.w*map.h,map.w,map.h,4);


	//reset flag
	map.flags.updateShadowMap = 0;
}

//update map data and stuff
void updateData(){
	/* 
	for(int y=0;y<Map.h;y++){
		for(int x=0;x<Map.w;x++){
			Uint32 posID = x + y*Map.w;
			Map.waterHeight[posID]     = PIPE::W[4+x*5 + y*PIPE::width*5];
			Map.mistHeight[posID]      = PIPE::M[4+x*5 + y*PIPE::width*5];
			Map.lavaHeight[posID]      = PIPE::L[4+x*5 + y*PIPE::width*5];
		}
	}
	
	//if(terrainHasChanged){
		for(int y=0;y<Map.h;y++){
			for(int x=0;x<Map.w;x++){
				Uint32 posID = x + y*Map.w;
				Map.groundHeight[posID]    = (Map.height[posID] + Map.sediment[posID]);//get height
				PIPE::T[x + y*PIPE::width] = Map.groundHeight[posID];
				Map.totalHeight[posID] = Map.groundHeight[posID] + Map.waterHeight[posID] + Map.mistHeight[posID] + Map.lavaHeight[posID];
			}
		}
		Map.generate_flowfield();
		Map.generate_colormap();
		terrainHasChanged = 0;
	//}
	*/
}

argb_t getTileColorLava(int xwti, int ywti, float shade){
	/* 
	int r,g,b;
	float lavaHeight = Map.lavaHeight[xwti + ywti*PIPE::width];
	float slopeX = PIPE::L[4+(xwti-1)*5+(ywti)*PIPE::width*5] - PIPE::L[4+(xwti+1)*5+(ywti)*PIPE::width*5];
	float slopeY = PIPE::L[4+(xwti)*5+(ywti-1)*PIPE::width*5] - PIPE::L[4+(xwti)*5+(ywti+1)*PIPE::width*5];
	r = 50 + lavaHeight*100 +(- Map.foamLevel[xwti + ywti*PIPE::width] - slopeX - slopeY)*(1-shade);
	g = 50 + lavaHeight*30  +(- Map.foamLevel[xwti + ywti*PIPE::width] - slopeX - slopeY)*(1-shade);
	b = 50 + lavaHeight*10  +(- Map.foamLevel[xwti + ywti*PIPE::width] - slopeX - slopeY)*(1-shade);
	if(slopeX + slopeY > 0.1 && slopeX + slopeY < 1){
		r += 20*(1-shade);
		g += 20*(1-shade);
		b += 20*(1-shade);
	}
	
	rgb returnRGB;
	returnRGB.r = std::min(std::max((int)r,0),255);
	returnRGB.g = std::min(std::max((int)g,0),255);
	returnRGB.b = std::min(std::max((int)b,0),255);
	
	return returnRGB;
	*/
}

argb_t getTileColorWater(int x, int y, int ys, vec2f_t upVec, float shade, camera_t* camPtr){
	
		shade = min(shade, 1.f);
		int r,g,b;
		int posID = x + y * map.w;
//		float wtrHeight = map.water[4+x*5+y*map.w*5];
		float wtrHeight = map.water[x+y*map.w].depth;
		float camZoom = camPtr->zoom;
		//calculate slope of water for highlighting and caustics
//		float slopX = -map.water[4+(x+1)*5 + (y)*map.w*5] + map.water[4+(x-1)*5 + (y)*map.w*5] - map.stone[(x+1) + (y)*map.w] + map.stone[(x-1) + (y)*map.w];
		float slopX = -map.water[(x+1)+y*map.w].depth + map.water[(x-1)+y*map.w].depth - map.height[(x+1) + (y)*map.w] + map.height[(x-1) + (y)*map.w];
//		float slopY = map.water[4+(x)*5 + (y-1)*map.w*5] - map.water[4+(x)*5 + (y+1)*map.w*5] + map.stone[(x) + (y-1)*map.w] - map.stone[(x) + (y+1)*map.w];
		float slopY = map.water[x+(y-1)*map.w].depth - map.water[x+y*map.w].depth + map.height[(x) + (y-1)*map.w] - map.height[(x) + (y+1)*map.w];
		if(1){//fancy water
			int test2 = 0;
			int underwaterFoam = 0; //use this shit
			for(int i=0;i<wtrHeight+1;i++){
				int seaFloorPosX = (x+(int)(upVec.x*i));
				int seaFloorPosY = (y+(int)(upVec.y*i));
//				underwaterFoam += max((int)(map.suSed[seaFloorPosX+seaFloorPosY*map.w]*200) - i*5,0) ;
				if(map.height[seaFloorPosX+seaFloorPosY*map.w] >= wtrHeight*2 - (float)i*1.25){
					test2 = (x+(int)(upVec.x*i))+(y+(int)(upVec.y*i))*map.w;
					int newX = (x+(int)(upVec.x*i));
					int newY = (y+(int)(upVec.y*i));
					float caustic = 0;
					if(!(seaFloorPosX <= 2 || seaFloorPosX >= map.w-2 || seaFloorPosY <= 2 || seaFloorPosY >= map.h-2)){
//						caustic =  (map.water[4+(newX+1)*5+newY*map.w*5] - map.water[4+(newX-1)*5+newY*map.w*5] + map.stone[test2+1] - map.stone[test2-1] +
						caustic =  (map.water[(newX+1)+newY*map.w].depth - map.water[(newX-1)+newY*map.w].depth + map.height[test2+1] - map.height[test2-1] +
//							map.water[4+(newX)*5+(newY+1)*map.w*5] - map.water[4+(newX)*5+(newY-1)*map.w*5] + map.stone[test2+1*map.w] - map.stone[test2-1*map.w])*10;
							map.water[(newX)+(newY+1)*map.w].depth - map.water[(newX)+(newY-1)*map.w].depth + map.height[test2+1*map.w] - map.height[test2-1*map.w])*10;
					}
					float foamShadow = 0;//map.foamLevel[test2] * 0.2;
					underwaterFoam = max(min(underwaterFoam,100),0);
					r =	min(map.argb[posID].r + map.sand[posID]*202.f*0.5f,202.f);   //map.argb[test2].r ;//+ (-caustic - foamShadow + underwaterFoam*0.5f)*(1-min(shade,1.0f));
					g =	min(map.argb[posID].r + map.sand[posID]*188.f*0.5f,188.f);   //map.argb[test2].g ;//+ (-caustic - foamShadow + underwaterFoam*0.5f)*(1-min(shade,1.0f));
					b =	min(map.argb[posID].r + map.sand[posID]*145.f*0.5f,145.f);   //map.argb[test2].b ;//+ (-caustic - foamShadow + underwaterFoam)*(1-min(shade,1.0f));
					
					//lerp between hard and soft shadows depending on the depth
					int shadow;
//					if(map.water[4+(newX)*5+(newY)*map.w*5] < 10){ //interpolate between hard and soft shadows
					if(map.water[(newX)+(newY)*map.w].depth < 10){ //interpolate between hard and soft shadows
//						shadow = map.shadow[test2] + (map.shadowSoft[test2] - map.shadow[test2])*(min(map.water[4+(newX)*5+(newY)*map.w*5]/10.f,1.f));
						shadow = map.shadow[test2] + (map.shadowSoft[test2] - map.shadow[test2])*(min(map.water[(newX)+(newY)*map.w].depth/10.f,1.f));
					}else{//interpolate between soft and no shadows
//						shadow = map.shadowSoft[test2] + (0 - map.shadowSoft[test2])*(min(map.water[4+(newX)*5+(newY)*map.w*5]/100.f,1.f));
						shadow = map.shadowSoft[test2] + (0 - map.shadowSoft[test2])*(min(map.water[(newX)+(newY)*map.w].depth/100.f,1.f));
					}
					r -= 0;//shadow;
					g -= 0;//shadow;
					b -= 0;//shadow;
					break;
				}
				if(seaFloorPosX < 1 || seaFloorPosX >= map.w-1 || seaFloorPosY < 1 || seaFloorPosY >= map.h-1){
					r = (102+(int)ys)>>2;//67
					g = (192+(int)ys)>>2;//157
					b = (229+(int)ys)>>2;//197
					break;
				}
			}
			
		}else{//unfancy water
			int seaFloorPosX = (x+(int)(upVec.x*wtrHeight));
			int seaFloorPosY = (y+(int)(upVec.y*wtrHeight));
			float foamShadow = 0;
			float caustic = 0;
			if(seaFloorPosX > 2 && seaFloorPosX < map.w-2 && seaFloorPosY > 2 && seaFloorPosY < map.h-2){
				int seaFloorPos = seaFloorPosX+seaFloorPosY*map.w;
//				foamShadow = map.foamLevel[seaFloorPos] * 0.1;
//				caustic =  (map.water[4+(seaFloorPosX+1)*5+(seaFloorPosY)*map.w*5] - map.water[4+(seaFloorPosX-1)*5+(seaFloorPosY)*map.w*5] + map.stone[seaFloorPos+1] - map.stone[seaFloorPos-1] +
				caustic =  (map.water[(seaFloorPosX+1)+(seaFloorPosY)*map.w].depth - map.water[(seaFloorPosX-1)+(seaFloorPosY)*map.w].depth + map.height[seaFloorPos+1] - map.height[seaFloorPos-1] +
//										map.water[4+(seaFloorPosX)*5+(seaFloorPosY+1)*map.w*5] - map.water[4+(seaFloorPosX)*5+(seaFloorPosY-1)*map.w*5] + map.stone[seaFloorPos+1*map.w] - map.stone[seaFloorPos-1*map.w])*10;
										map.water[(seaFloorPosX)+(seaFloorPosY+1)*map.w].depth - map.water[(seaFloorPosX)+(seaFloorPosY-1)*map.w].depth + map.height[seaFloorPos+1*map.w] - map.height[seaFloorPos-1*map.w])*10;
			}
//			r =	map.argb[posID].r + (caustic - foamShadow)*(1-shade);
//			g =	map.argb[posID].g + (caustic - foamShadow)*(1-shade);
//			b =	map.argb[posID].b + (caustic - foamShadow)*(1-shade);
		}					
		if(1){//water surface
			
			//highligt according to slope
			r += -25 - wtrHeight*4      + (slopX*slopX+slopY*slopY)*1;
			g += -16 - wtrHeight*2    + (slopX*slopX+slopY*slopY)*2;
			b += -8  - wtrHeight*1   + (slopX*slopX+slopY*slopY)*2;
			//if slope is certain angle, make water whiter to look like glare from sun
			if(slopX + slopY > 0.1 && slopX + slopY < 1){
				if(slopX + slopY > 0.5 && slopX + slopY < 1){
					r += 30*(1-min(shade,1.0f));
					g += 30*(1-min(shade,1.0f));
					b += 30*(1-min(shade,1.0f));
				}else{
					r += 10*(1-min(shade,1.0f));
					g += 10*(1-min(shade,1.0f));
					b += 10*(1-min(shade,1.0f));
				}
			}
			//if foam is present, make water whiter
			//if water is turbulent, make water whiter
//			float velX = PIPE::velX[x+y*map.w];
//			float velY = PIPE::velY[x+y*map.w];
//			float curl = PIPE::velY[x+1+y*map.w] - PIPE::velY[x-1+y*map.w] - PIPE::velX[x+(y+1)*map.w] + PIPE::velY[x+(y-1)*map.w];
//			float velDiff = velX*2 - PIPE::velX[x-1+y*map.w] - PIPE::velX[x+1+y*map.w] + velY*2 - PIPE::velY[x+(y-1)*map.w] - PIPE::velY[x+(y+1)*map.w];
//			r += (curl*curl*0.01+velDiff*velDiff*0.01)*(1-min(shade,1.0f));
//			g += (curl*curl*0.01+velDiff*velDiff*0.01)*(1-min(shade,1.0f));
//			b += (curl*curl*0.01+velDiff*velDiff*0.01)*(1-min(shade,1.0f));

			//reflections
//			for(int i=0;i<100;i++){
//				int reflectionPosX = (x+(int)(upVec.x*i));
//				int reflectionPosY = (y+(int)(upVec.y*i));
//				if(map.stone[reflectionPosX+reflectionPosY*map.w] >= map.stone[x+y*map.w] + wtrHeight + (float)i*0.75f){
//
//					r = map.argb[reflectionPosX+reflectionPosY*map.w].r;
//					g = map.argb[reflectionPosX+reflectionPosY*map.w].g;
//					b = map.argb[reflectionPosX+reflectionPosY*map.w].b;
//					break;
//				}
//			}
		}
	//draw foam
	if(map.foamLevel[x + y*map.w] > 0){
		r += map.foamLevel[x + y*map.w]*(1-min(shade,1.0f))*1;
		g += map.foamLevel[x + y*map.w]*(1-min(shade,1.0f))*1;
		b += map.foamLevel[x + y*map.w]*(1-min(shade,1.0f))*1;
	}
	if(map.susSed[x + y*map.w] > 0){
		r +=  map.susSed[x+y*map.w]*202.f/4.f;
		g +=  map.susSed[x+y*map.w]*188.f/4.f;
		b +=  map.susSed[x+y*map.w]*145.f/2.f;
	}
	//apply shadow on water
	float shadow = map.shadow[x+y*map.w]*(1-min(shade,1.0f));
	r -= shadow;
	g -= shadow;
	b -= shadow;
								
	
	
	argb_t returnRGB;
	returnRGB.r = min(max((int)r,0),255);
	returnRGB.g = min(max((int)g,0),255);
	returnRGB.b = min(max((int)b,0),255);
	
	return returnRGB;
	
}

argb_t getTileColorMist(int x, int y, int ys, vec2f_t upVec){
	/* 
		int r,g,b;
		int posID = x + y * Map.w;
		float mistHeight = Map.mistHeight[posID];
		float camZoom = cam.zoom;
		
		float mistShade = mistHeight * 0.1f;
									int test2 = 0;
									for(int i=0;i<mistHeight+100;i++){
										int mistFloorPosX = (x+(int)(upVec.x*i));
										int mistFloorPosY = (y+(int)(upVec.y*i));
										if(Map.waterHeight[mistFloorPosX+mistFloorPosY*Map.w] + Map.groundHeight[mistFloorPosX+mistFloorPosY*Map.w] + Map.lavaHeight[mistFloorPosX+mistFloorPosY*Map.w] > mistHeight - (float)i*0.60f){ // *0.60f gives pretty much no refraction
											if(Map.waterHeight[mistFloorPosX+mistFloorPosY*Map.w] > 0.f){
												test2 = (x+int(upVec.x*i))+(y+int(upVec.y*i))*Map.w;
												
												rgb waterRGB = getTileColorWater(mistFloorPosX,mistFloorPosY,ys,upVec,mistShade);
												r = waterRGB.r;
												g = waterRGB.g;
												b = waterRGB.b;
												break;
											}else if(Map.lavaHeight[mistFloorPosX+mistFloorPosY*Map.w]){ 
												test2 = (x+int(upVec.x*i))+(y+int(upVec.y*i))*Map.w;
												
												rgb lavaRGB = getTileColorLava(mistFloorPosX,mistFloorPosY,mistShade);
												r = lavaRGB.r;
												g = lavaRGB.g;
												b = lavaRGB.b;
												
												rgb returnRGB;
												returnRGB.r = std::max(std::min(r+mistHeight*15,255.f),0.f);
												returnRGB.g = std::max(std::min(g+mistHeight*10,255.f),0.f);
												returnRGB.b = std::max(std::min(b+mistHeight*10,255.f),0.f);
													
												
												return returnRGB;
												
												break;
											}else{ //if(Map.groundHeight[mistFloorPosX+mistFloorPosY*Map.w] > mistHeight - i*1){
												test2 = (x+int(upVec.x*i))+(y+int(upVec.y*i))*Map.w;
												r =	Map.pos[test2].r;
												g =	Map.pos[test2].g;
												b =	Map.pos[test2].b;
													
												//lerp between hard and soft shadows depending on the depth
												int shadow;
												if(Map.mistHeight[test2] < 5){ //interpolate between hard and soft shadows
													shadow = Map.shadow[test2] + (Map.shadowSoft[test2] - Map.shadow[test2])*(std::min(Map.mistHeight[test2]/5.f,1.f));
												}else{//interpolate between soft and no shadows
													shadow = Map.shadowSoft[test2] + (0 - Map.shadowSoft[test2])*(std::min(Map.mistHeight[test2]/50.f,1.f));
												}
												r -= shadow;
												g -= shadow;
												b -= shadow;
												
												//draw foam];
												if(Map.foamLevelSoft[test2] > 0){
													float t = std::min(mistShade, 1.0f);
													r += Map.foamLevel[test2] * (1.f - t) + Map.foamLevelSoft[test2] * t;
													g += Map.foamLevel[test2] * (1.f - t) + Map.foamLevelSoft[test2] * t;
													b += Map.foamLevel[test2] * (1.f - t) + Map.foamLevelSoft[test2] * t;
													
													//r += Map.foamLevel[test2]*(std::max((1-mistShade),0.f));
													//g += Map.foamLevel[test2]*(std::max((1-mistShade),0.f));
													//b += Map.foamLevel[test2]*(std::max((1-mistShade),0.f));
												}
												
												break;
											}
										}else if(mistFloorPosX < 1 || mistFloorPosX >= Map.w-1 || mistFloorPosY < 1 || mistFloorPosY >= Map.h-1){
											r = (102+(int)ys)>>2;//67
											g = (192+(int)ys)>>2;//157
											b = (229+(int)ys)>>2;//197	
											break;
										}
									}
								
	//apply shadow on top of mist
//	float shadow = Map.shadowSoft[x+y*map.w];
//	r -= shadow;
//	g -= shadow;
//	b -= shadow;
//									
	//also make mist white, but not too white, because that is a property of mist or fog or whatever this is
	rgb returnRGB;
	returnRGB.r = std::max(std::min(r+mistHeight*20,250.f),0.f);
	returnRGB.g = std::max(std::min(g+mistHeight*20,250.f),0.f);
	returnRGB.b = std::max(std::min(b+mistHeight*20,250.f),0.f);
		
	
	return returnRGB;
	*/
}


//renders one pixel column
void renderColumn(int x, int yBot, int yTop, vec2f_t mapCornerBot, vec2f_t upVec,float tileEdgeSlopeRight,float tileEdgeSlopeLeft,float xwt,float ywt,float dDxw, float dDyw, camera_t* camPtr){
	//save some variables as local
	float camZoom    = camPtr->zoom;
	float camZoomDiv = 1.f / camPtr->zoom;
	
	//init some variables
	uint32_t argb; //store color of Pixel that will be drawn
	int border = 0; //used when skiping pixels to decide when at a edge of a tile that should be a darker shade
	int dPixels = 1; //how many pixels are skipped to get to the next tile
	int ys;
	
	//init ybuffer with lowest mappoint at current x screen position
	int ybuffer; //ybuffer stores the last drawn lowest position so there is no overdraw
	//set ybuffer to follow the lower edge of the map
	if(x > mapCornerBot.x) ybuffer = mapCornerBot.y+tileEdgeSlopeRight*(x-mapCornerBot.x);
	else ybuffer = mapCornerBot.y+tileEdgeSlopeLeft*(x-mapCornerBot.x);
	//start drawing column
	for(int y=yBot;y>yTop;y-=dPixels){ //the 100 offset is so terrain starts drawing a little bit below the screen

//		xs = x;
		ys = y;
		int ywti = (int)ywt;
		int xwti = (int)xwt;
//		ywti = max(ywti, 0);
//		ywti = min(ywti, map.h);
//		xwti = max(xwti, 0);
//		xwti = min(xwti, map.w);
		if(ywti > 0 && ywti < map.h && xwti > 0 && xwti < map.w){
			int posID = xwti + ywti*map.w;


			float gndHeight = map.stone[posID] + map.sand[posID]; //not using map.height ensures instant terrain update, map.height is okay for blurry/unclear renderings as underwater
			float wtrHeight = map.water[xwti+ywti*map.w].depth;
			float mistHeight = 0;//Map.mistHeight[posID];
			float lavaHeight = 0;//Map.lavaHeight[posID];


			ys = ys-(gndHeight+wtrHeight+mistHeight+lavaHeight)*camZoomDiv; //offset y by terrain height
			if(ys < ybuffer){
			//get color at worldspace and draw at screenspace
			int r,g,b;

			//draw mist if present
			if(mistHeight > 0){
			//	rgb mistRGB = getTileColorMist(xwti, ywti, ys, upVec);
			//	r = mistRGB.r;
			//	g = mistRGB.g;
			//	b = mistRGB.b;
			}else if(wtrHeight > 0){ //draw water if present
				argb_t waterARGB = getTileColorWater(xwti, ywti, ys, upVec, 0.f, camPtr);
				r = waterARGB.r;
				g = waterARGB.g;
				b = waterARGB.b;
			}else if(lavaHeight > 0){ //draw lava if present
			//	rgb lavaRGB = getTileColorLava(xwti, ywti, 0.f);
			//	r = lavaRGB.r;
			//	g = lavaRGB.g;
			//	b = lavaRGB.b;

			}else{ //only ground
//				xwt + ywt*map.w;  	202, 188, 145
				r =	min(map.argb[posID].r + map.sand[posID]*202.f*0.5f,202.f); //map.argb[posID].r;
				g =	min(map.argb[posID].r + map.sand[posID]*188.f*0.5f,188.f); //map.argb[posID].g;
				b =	min(map.argb[posID].r + map.sand[posID]*145.f*0.5f,145.f); //map.argb[posID].b;
				r -= map.shadow[posID];
				g -= map.shadow[posID];
				b -= map.shadow[posID];
				if(      map.foamLevel[xwti + ywti*map.w] > 0){
					r += map.foamLevel[xwti + ywti*map.w];
					g += map.foamLevel[xwti + ywti*map.w];
					b += map.foamLevel[xwti + ywti*map.w];
				}



			}

			//calculate and draw cursor
			if((xwti-cursor.worldX)*(xwti-cursor.worldX) + (ywti-cursor.worldY)*(ywti-cursor.worldY) <= cursor.radius*cursor.radius){
				r += 0;
				g += 30;
				b += 0;
			}

			//make borders of tiles darker, make it so they become darker the more zoomed in you are
			if(camZoom < 0.3 && !border){
//							float borderWidth = 1.6*camZoom;
//							if(xwt - (int)xwt < borderWidth ||
//								 ywt - (int)ywt < borderWidth ||
//								 (int)xwt+1 - xwt < borderWidth||
//								 (int)ywt+1 - ywt < borderWidth){
							r -= F2INT(1*camZoomDiv);
							g -= F2INT(1*camZoomDiv);
							b -= F2INT(1*camZoomDiv);
//								 }
			}


			//clamp color values
			argb = (min(max(r,0),255) << 16) | (min(max(g,0),255) << 8) | (min(max(b,0),255));

			ybuffer = min(ybuffer, rendTexture.h);
			ys = max(ys, 0);
			//only draw visible pixels
			for(int Y=ybuffer-1;Y>=ys;Y--){

				frameBuffer[x + (Y)*rendTexture.w] = argb;	//draw pixels

			}

			ybuffer = ys; //save current highest point in pixel column
			}
		dPixels = 0; // reset delta Y pixels
		//this piece of code calculates how many y pixels (dPixels) there is to the next tile
		//and the border thing makes it so it only jumps one pixel when there is a
		//new tile and the next drawing part is darker, thus making the edge of the tile darker
		if(!border){ //!border
			border = 1;
			float testX,testY;
			 if(camPtr->rot <= (45.f*3.141592654)/180.f || camPtr->rot > (315.f*3.141592654)/180.f ){
				testX =  ( (xwt -(int)xwt))/dDxw;
				testY =  ( (ywt -(int)ywt))/dDyw;
			 }else if(camPtr->rot <= (135.f*3.141592654)/180.f ){
				testX =  ((1-(xwt -(int)xwt))/dDxw);
				testY =  (((ywt -(int)ywt))/dDyw);
			 }else if(camPtr->rot <= (225.f*3.141592654)/180.f){
			  testX =  (1 - (xwt -(int)xwt))/dDxw;
				testY =  (1 - (ywt -(int)ywt))/dDyw;
			 }else if(camPtr->rot <= (315.f*3.141592654)/180.f ){
				testX =  (((xwt -(int)xwt))/dDxw);
				testY =  ((1-(ywt -(int)ywt))/dDyw);
			 }
			 dPixels = min(fabsf(testX),fabsf(testY));
			// if(dPixels < 1) dPixels = 1;
			xwt += dDxw * dPixels;
			ywt += dDyw * dPixels;
		}else{
			border = 0;
			xwt += dDxw;
			ywt += dDyw;
			dPixels = 1;
		}
}else{
	dPixels = 1;
	xwt += dDxw * dPixels;
	ywt += dDyw * dPixels;
}
	}
	
}



void render(){
	//Since this runs on a separate thread from input update I need to back up camera variables so they don't change during rendering
	camera_t cam;
	cam.rot = g_cam.rot;
	cam.zoom = g_cam.zoom;
	cam.x = g_cam.x;
	cam.y = g_cam.y;
	float camZoomDiv = 1.f / cam.zoom;


	for(int y=0;y<rendTexture.h;y++){
			Uint32 rgb = (((102+y)>>2) << 16) | (((192+y)>>2) << 8) | ((229+y)>>2);
		for(int x=0;x<rendTexture.w;x++){


			frameBuffer[x + y*rendTexture.w] = rgb;
		}
	}


	float xw,yw,zw;
	float xs,ys;

	//furstum
	xs = 0;
	ys = 0;
	vec2f_t ftl = screen2world(xs,ys,&cam);
	xs = rendTexture.w;
	ys = rendTexture.h;
	vec2f_t fbr = screen2world(xs,ys,&cam);
	xs = 0;
	ys = rendTexture.h;
	vec2f_t fbl = screen2world(xs,ys,&cam);
	xs = 0;
	ys = rendTexture.h+100/cam.zoom;
	vec2f_t fblb = screen2world(xs,ys,&cam);
	
	float dxw  = (fbr.x - fbl.x)/rendTexture.w; //delta x worldspace
	float dyw  = (fbr.y - fbl.y)/rendTexture.w; //delta y worldspace
	float dDxw = (ftl.x - fbl.x)/rendTexture.h; //delta x worldspace depth
	float dDyw = (ftl.y - fbl.y)/rendTexture.h; //delta y worldspace depth
	
	//create normalized vector that point up on the screen but in world coorinates, for use with raytracing water refraction
	vec2f_t  upVec = {ftl.x - fbl.x, ftl.y - fbl.y};
	double tempDistLongName = sqrt(upVec.x*upVec.x+upVec.y*upVec.y);
	upVec.x /= tempDistLongName;
	upVec.y /= tempDistLongName;



	float sed,hei,sloX,sloY;
	int water;



	///////// merge these calculations later
	//calculate screen coordinates of world corners
	vec2f_t tlw =world2screen(1, 1, &cam);
	vec2f_t trw =world2screen(map.w, 1, &cam);
	vec2f_t blw =world2screen(1, map.h, &cam);
	vec2f_t brw =world2screen(map.w, map.h, &cam);
	//check what relative postion map corners have
	vec2f_t mapCornerTop,mapCornerLeft,mapCornerBot,mapCornerRight;
	if(fabsf(cam.rot) < 45*3.141592654/180 || fabsf(cam.rot) >= 315*3.141592654/180 ){
		 mapCornerTop   = (vec2f_t){tlw.x,tlw.y};
		 mapCornerLeft  = (vec2f_t){blw.x,blw.y};
		 mapCornerBot   = (vec2f_t){brw.x,brw.y};
		 mapCornerRight = (vec2f_t){trw.x,trw.y};
	}else if(fabsf(cam.rot) < 135*3.141592654/180){
		 mapCornerRight = (vec2f_t){brw.x,brw.y};
		 mapCornerTop   = (vec2f_t){trw.x,trw.y};
		 mapCornerLeft  = (vec2f_t){tlw.x,tlw.y};
		 mapCornerBot   = (vec2f_t){blw.x,blw.y};
	}else if(fabsf(cam.rot) < 225*3.141592654/180){
		 mapCornerBot   = (vec2f_t){tlw.x,tlw.y};
		 mapCornerRight = (vec2f_t){blw.x,blw.y};
		 mapCornerTop   = (vec2f_t){brw.x,brw.y};
		 mapCornerLeft  = (vec2f_t){trw.x,trw.y};
	}else if(fabsf(cam.rot) < 315*3.141592654/180){
		 mapCornerLeft  = (vec2f_t){brw.x,brw.y};
		 mapCornerBot   = (vec2f_t){trw.x,trw.y};
		 mapCornerRight = (vec2f_t){tlw.x,tlw.y};
		 mapCornerTop   = (vec2f_t){blw.x,blw.y};
	}
	//calculate slope of tile edges on screen
	float tileEdgeSlopeRight = (float)(mapCornerRight.y-mapCornerBot.y)/(float)(mapCornerRight.x-mapCornerBot.x);
	float tileEdgeSlopeLeft = (float)(mapCornerBot.y-mapCornerLeft.y)/(float)(mapCornerBot.x-mapCornerLeft.x);
	/////////


	//these coordinates will be the bounds at which renderColumn() will render any terrain
	int leftMostXCoord  = max((int)mapCornerLeft.x, 0);
	int rightMostXCoord = min((int)mapCornerRight.x, rendTexture.w);
	int botMostYCoord = min((int)mapCornerBot.y, rendTexture.h+100/cam.zoom);
	int topMostYCoord = max((int)mapCornerTop.y-100/cam.zoom, 0);

	vec2f_t leftMostWorldCoord = screen2world(leftMostXCoord, botMostYCoord, &cam);

	xw = leftMostWorldCoord.x; //set world coords to coresponding coords for bottom left of screen
	yw = leftMostWorldCoord.y; //then iterate left to right, bottom to top of screen

	for(int x = leftMostXCoord; x < rightMostXCoord;x++){
		float xwt = xw; //make a copy of world coordinate for leftmost position at current depth
		float ywt = yw;

		renderColumn(x,botMostYCoord,topMostYCoord,mapCornerBot,upVec,tileEdgeSlopeRight,tileEdgeSlopeLeft,xwt,ywt,dDxw,dDyw,&cam);
		

		xw += dxw; //update world coords corresponding to one pixel right
		yw += dyw;
	}

		
	//draw mouse
	for(int y=-1;y<1;y++){
		for(int x=-1;x<1;x++){
			uint32_t rgb = (255 << 16) | (255 << 8) | (255);
			if(input.mouse.x-10 > 0 && input.mouse.x + 10 < rendTexture.w &&
				input.mouse.y-10 > 0 && input.mouse.y + 10 < rendTexture.h ){
					frameBuffer[(cursor.screenX+x)+(cursor.screenY+y)*rendTexture.w] = rgb;
			}
		}
	}

	

}

void renderHud(){
	for(int y=0;y<hudTexture.h;y++){
		for(int x=0;x<hudTexture.w;x++){

			//argb_t argb = {map.argb[x+y*MAPW].b, map.argb[x+y*MAPW].r, map.argb[x+y*MAPW].g, 255};

			hudTexture.pixels[x + y*hudTexture.w] = (100 << 8) | (150);


		}
	}

	gui_drawGUI(&hudTexture);
	
}


const Uint8 *keyboardState; // get pointer to key states 

void toolSelectWater(void){
	cursor.tool = TOOL_WATER;
}

void toolSelectSand(void){
	cursor.tool = TOOL_SAND;
}

void toolSelectStone(void){
	cursor.tool = TOOL_STONE;
}

float g_simSpeed = 1.f;
void init(){
	
	keyboardState = SDL_GetKeyboardState(NULL); // get pointer to key states 

	map.w = MAPW;
	map.h = MAPH;
	map.tileWidth = 1.f;

	//init camera position, the following will init camera to center overview a 256x256 map
	//camera: x:336.321991 y:-93.287224 rot:1.570000 zoom:0.609125camera: x:327.101379 y:-84.052345 rot:1.570000 zoom:0.609125
	g_cam.x = 221.321991;
	g_cam.y = 21.287224;
	g_cam.rot = 3.14f/2;
	g_cam.zoom = 0.609125;

	cursor.amount = 0.3;
	cursor.radius = 8;

	loadFont("assets/VictoriaBold.png"); //load font lol

//	loadHeightMap("assets/mountain_height.png");
//	loadColorMap("assets/mountain_color.png");

	map.flags.updateShadowMap = 1; //make sure shadows are updated after map load


	memset(&foam,0,sizeof(foam));
	foam.size = FOAMSIZE;

    for(int y=0;y<MAPH;y++){
        for(int x=0;x<MAPW;x++){
        	map.stone[x+y*map.w] = 20.f + 0*(float)x/4.f;
        	map.sand[x+y*map.w] = 0.f;

        	map.argb[x+y*map.w].r = 80;
        	map.argb[x+y*map.w].g = 80;
        	map.argb[x+y*map.w].b = 80;

//        	if(x%20<10&&y%20<10)map.foamLevel[x+y*map.w] = 50.f;
        }
    }

    //create GUI
    gui_createPressButton("Water", 20,  20, 160, 50, &toolSelectWater);
    gui_createPressButton("Sand",  20,  90, 160, 50, &toolSelectSand);
    gui_createPressButton("Stone", 20, 160, 160, 50, &toolSelectStone);

    float radiusMax = 32.f;
    float radiusMin = 1.f;
    float amountMax = 1.f;
    float amountMin = 0.01f;
    float simSpeedMin = 0.1f;
    float simSpeedMax = 2.0f;
    gui_createSlider("Radius", 20, 300, 160, 10, SLIDER_TYPE_FLOAT, &radiusMax, &radiusMin, &(cursor.radius));
    gui_createSlider("Amount", 20, 340, 160, 10, SLIDER_TYPE_FLOAT, &amountMax, &amountMin, &(cursor.amount));
    gui_createSlider("Sim speed", 20, 380, 160, 10, SLIDER_TYPE_FLOAT, &simSpeedMax, &simSpeedMin, &(g_simSpeed));
}


void updateInput(){
	//update pos and state of mouse
	uint32_t SDL_mouseState;
	SDL_mouseState = SDL_GetMouseState(&input.mouse.x,&input.mouse.y);

	static int leftTrigger = 0;
	if(SDL_mouseState == SDL_BUTTON_LEFT){
		if(leftTrigger == 0){
			input.mouse.left = KEY_PRESSED;
			leftTrigger = 1;
		}else{
			input.mouse.left = KEY_HELD;
		}

	}else if(leftTrigger == 1){
		input.mouse.left = KEY_RELEASED;
		leftTrigger = 0;
	}
	static int rightTrigger = 0;
	if(SDL_mouseState == 4){
		if(rightTrigger == 0){
			input.mouse.right = KEY_PRESSED;
			rightTrigger = 1;
		}else{
			input.mouse.right = KEY_HELD;
		}

	}else if(rightTrigger == 1){
		input.mouse.right = KEY_RELEASED;
		rightTrigger = 0;
	}

	//update mouse world pos
	float percentX = ((float)input.mouse.x / (float)windowSizeX);
	float pixelsX = percentX * (float)g_canvasSizeX;

	float ratioX =  (float)windowSizeX / (float)g_canvasSizeX;
	float ratioY = (float)g_canvasSizeY / (float)windowSizeY;
	float test = get_browser_height();
	cursor.screenX = (float)input.mouse.x * (float)g_windowScaleX;
	cursor.screenY = (float)input.mouse.y * (float)g_windowScaleY;
	vec2f_t pos = screen2world(cursor.screenX, cursor.screenY, &g_cam);
	cursor.worldX  = pos.x;
	cursor.worldY  = pos.y;

	gui_handleGUI(cursor.screenX - rendererSizeX , cursor.screenY, &input);

	switch(SDL_mouseState){
	case SDL_BUTTON_LEFT:
	{
		float radius = cursor.radius;
		for(int j=-radius*4;j<=radius*4;j++){
			for(int k=-radius*4;k<=radius*4;k++){
//					map.water[4+5*(cursor.worldX+k)+(cursor.worldY+j)*map.w*5] +=  cursor.amount*radius*radius*exp(-(k*k+j*j)/(2.f*radius*radius))/(2*3.14159265359*radius*radius)*g_dtime_ms;
					switch(cursor.tool){
					case TOOL_WATER:
						if(cursor.worldX+k >= 3 && cursor.worldY+j >= 3 && cursor.worldX+k < map.w-2 && cursor.worldY+j < map.h-2){
							map.water[(cursor.worldX+k)+(cursor.worldY+j)*map.w].depth +=  cursor.amount*radius*radius*exp(-(k*k+j*j)/(2.f*radius*radius))/(2*3.14159265359*radius*radius)*g_dtime_ms;
						}
						break;
					case TOOL_SAND:
						if(cursor.worldX+k >= 2 && cursor.worldY+j >= 2 && cursor.worldX+k < map.w-1 && cursor.worldY+j < map.h-1){
							map.sand[cursor.worldX+k+(cursor.worldY+j)*map.w] += cursor.amount*radius*radius*exp(-((float)(k*k+j*j))/(2.f*radius*radius))/(2*3.14159265359*radius*radius)*g_dtime_ms;
						}
						break;
					case TOOL_STONE:
						if(cursor.worldX+k >= 1 && cursor.worldY+j >= 1 && cursor.worldX+k < map.w-0 && cursor.worldY+j < map.h-0){
							map.stone[cursor.worldX+k+(cursor.worldY+j)*map.w] += cursor.amount*radius*radius*exp(-((float)(k*k+j*j))/(2.f*radius*radius))/(2*3.14159265359*radius*radius)*g_dtime_ms;
						}
						break;
					}
//					map.stone[cursor.worldX+k+(cursor.worldY+j)*map.w] += cursor.amount*radius*radius*exp(-(k*k+j*j)/(2.f*radius*radius))/(2*3.14159265359*radius*radius)*g_dtime_ms;
			}
		}
	}
		break;
	case 4: //RIGHT
	{
		float radius = cursor.radius;
		for(int j=-radius*2;j<=radius*2;j++){
			for(int k=-radius*2;k<=radius*2;k++){
				if(cursor.worldX+k >= 0 && cursor.worldY+j >= 0 && cursor.worldX+k < map.w && cursor.worldY+j < map.h){
					switch(cursor.tool){
					case TOOL_WATER:
						map.water[(cursor.worldX+k)+(cursor.worldY+j)*map.w].depth = max(map.water[(cursor.worldX+k)+(cursor.worldY+j)*map.w].depth -  cursor.amount*radius*radius*exp(-(k*k+j*j)/(2.f*radius*radius))/(2*3.14159265359*radius*radius)*g_dtime_ms, 0.f);
						break;
					case TOOL_SAND:
						map.sand[cursor.worldX+k+(cursor.worldY+j)*map.w]          = max(map.sand[cursor.worldX+k+(cursor.worldY+j)*map.w] - cursor.amount*radius*radius*exp(-((float)(k*k+j*j))/(2.f*radius*radius))/(2*3.14159265359*radius*radius)*g_dtime_ms, 0.f);
						break;
					case TOOL_STONE:
						map.stone[cursor.worldX+k+(cursor.worldY+j)*map.w]         = max(map.stone[cursor.worldX+k+(cursor.worldY+j)*map.w] - cursor.amount*radius*radius*exp(-((float)(k*k+j*j))/(2.f*radius*radius))/(2*3.14159265359*radius*radius)*g_dtime_ms, 5.f);
						break;
					}
				}
			}
		}
		map.flags.updateShadowMap = 1;
	}
		break;
	}



	if(keyboardState[SDL_SCANCODE_A]){

			g_cam.x += cos(g_cam.rot-3.1415/4)*300 *g_dtime_ms / 1000;
			g_cam.y += sin(g_cam.rot-3.1415/4)*300 *g_dtime_ms / 1000;

	}
	if(keyboardState[SDL_SCANCODE_D]){

			g_cam.x -= cos(g_cam.rot-3.1415/4)*300 *g_dtime_ms / 1000;
			g_cam.y -= sin(g_cam.rot-3.1415/4)*300 *g_dtime_ms / 1000;

	}
	if(keyboardState[SDL_SCANCODE_W]){

			g_cam.x -= sin(g_cam.rot-3.1415/4)*450 *g_dtime_ms / 1000;
			g_cam.y += cos(g_cam.rot-3.1415/4)*450 *g_dtime_ms / 1000;

			//Print camera pos
			printf("camera: x:%f y:%f rot:%f zoom:%f", g_cam.x, g_cam.y, g_cam.rot, g_cam.zoom);
	}
	if(keyboardState[SDL_SCANCODE_S]){

			g_cam.x += sin(g_cam.rot-3.1415/4)*450 *g_dtime_ms / 1000;
			g_cam.y -= cos(g_cam.rot-3.1415/4)*450 *g_dtime_ms / 1000;

	}
	if(keyboardState[SDL_SCANCODE_R]){
			float rotation = g_cam.rot; //save camera rotation
			g_cam.rot = 0; //set camera rotation to 0

			vec2f_t pos1 = screen2world(rendTexture.w/2, rendTexture.h/2, &g_cam);
			g_cam.zoom += 1 *g_cam.zoom *g_dtime_ms / 1000;
			vec2f_t pos2 = world2screen(pos1.x, pos1.y, &g_cam);
			vec2f_t deltapos = {rendTexture.w/2-pos2.x,rendTexture.h/2 -pos2.y};
			//transform coordinate offset to isometric
			double sq2d2 = sqrt(2)/2;
			double sq6d2 = sqrt(6)/2;
			double xw = sq2d2*deltapos.x+sq6d2*deltapos.y;
			double yw = sq6d2*deltapos.y-sq2d2*deltapos.x;
			g_cam.y += yw;
			g_cam.x += xw;
			g_cam.rot = rotation; //restore camera rotation
	}
	if(keyboardState[SDL_SCANCODE_F]){
			if(g_cam.zoom > 0.03){
				float rotation = g_cam.rot; //save camera rotation
				g_cam.rot = 0; //set camera rotation to 0

				vec2f_t pos1 = screen2world(rendTexture.w/2, rendTexture.h/2, &g_cam);
				g_cam.zoom -= 1 *g_cam.zoom *g_dtime_ms / 1000;
				vec2f_t pos2 = world2screen(pos1.x, pos1.y, &g_cam);
				vec2f_t deltapos = {rendTexture.w/2-pos2.x, rendTexture.h/2 -pos2.y};
				//transform coordinate offset to isometric
				double sq2d2 = sqrt(2)/2;
				double sq6d2 = sqrt(6)/2;
				double xw = sq2d2*deltapos.x+sq6d2*deltapos.y;
				double yw = sq6d2*deltapos.y-sq2d2*deltapos.x;
				g_cam.y += yw;
				g_cam.x += xw;
				g_cam.rot = rotation; //restore camera rotation
				
			}
	}
	if(keyboardState[SDL_SCANCODE_Q]){
		float angle = 32*3.14/180.f;
		g_cam.rot = fmod((g_cam.rot - angle *2 *g_dtime_ms / 1000), 6.283185307);
		if(g_cam.rot < 0) g_cam.rot = 6.283185307;
			
	}
	if(keyboardState[SDL_SCANCODE_E]){
		float angle = 32*3.14/180.f;
		g_cam.rot = fmod((g_cam.rot + angle *2 *g_dtime_ms / 1000), 6.283185307);
				
	}
}

void process(){


//	update_fluid_velocity();

	//foam
	//spawn foam where water is turbulent
	for(int y=1;y<map.h-1;y++){
		for(int x=1;x<map.w-1;x++){

			float velX = map.waterVel[x+y*map.w].x;
			float velY = map.waterVel[x+y*map.w].y;
			//curl is something, velDiff is speed difference with nearby tiles
			float curl = map.waterVel[(x+1)+y*map.w].y - map.waterVel[(x-1)+y*map.w].y - map.waterVel[x+(y+1)*map.w].x + map.waterVel[x+(y-1)*map.w].y;
			float velDiff = velX*2 - map.waterVel[(x-1)+(y)*map.w].x - map.waterVel[(x+1)+(y)*map.w].x + velY*2 - map.waterVel[x+(y-1)*map.w].y - map.waterVel[x+(y+1)*map.w].y;
			float deltaV = (map.water[(x-1)+(y)*MAPW].right+map.water[(x)+(y+1)*MAPW].down+map.water[(x+1)+(y)*MAPW].left+map.water[(x)+(y-1)*MAPW].up - (map.water[(x)+(y)*MAPW].right+map.water[(x)+(y)*MAPW].down+map.water[(x)+(y)*MAPW].left+map.water[(x)+(y)*MAPW].up));

			if(deltaV > 0.5f){//(fabsf(velDiff) > 10 || fabsf(curl) > 5)){
				foam.status[foam.counter%foam.size].amount = 2;
				foam.status[foam.counter%foam.size].on = 1;
				foam.pos[foam.counter%foam.size].x = x;
				foam.pos[foam.counter%foam.size].y = y;
//				foam.vel[foam.counter%foam.size].x = velX;
//				foam.vel[foam.counter%foam.size].y = velY;
				foam.counter ++;
//				map.foamLevel[x+y*map.h] += 2;
			}

		}
	}



	//clear foamlevels and update with new levels

//	memset(map.foamLevel, 0 , sizeof(*map.foamLevel)*map.w*map.h);
//
//
//	for(int i=0;i<foam.size;i++){
//		if(foam.status[i].on){
//			foam.pos[i].x += map.waterVel[ROUNDF(foam.pos[i].x)+ROUNDF(foam.pos[i].y)*map.w].x*0.1;
//			foam.pos[i].y -= map.waterVel[ROUNDF(foam.pos[i].x)+ROUNDF(foam.pos[i].y)*map.w].y*0.1;
//			int x = ROUNDF(foam.pos[i].x);
//			int y = ROUNDF(foam.pos[i].y);
//			if(map.foamLevel[x+y*map.w] > 128)  foam.status[i].on = 0;
//			else map.foamLevel[x+y*map.w] += 2;//foam.amount[i];
//
//		}
//	}


#if (ENABLE_MULTITHREADING == 0)
	water_update(map.water, 9.81f, 1.f, 1.f, 0.99f, /*min(0.001f*g_dtime_ms, 0.15f)*/0.15f*g_simSpeed);
#endif

	static int timer_100ms = 0;
	if(g_time_ms - timer_100ms > 100 || map.flags.updateShadowMap){
		timer_100ms = g_time_ms;

		for(int i=0;i<map.w*map.h;i++){
			map.height[i] = map.stone[i]+map.sand[i];
		}

//	float totStone = 0;
//	float totSus = 0;
//	for(int y=0;y<map.h-0;y++){
//		for(int x=0;x<map.w-0;x++){
//			totStone += map.stone[x+y*map.w];
//			totSus += map.susSed[x+y*map.w];
//		}
//	}
//	printf("stone: %f sed: %f tot: %f\n",totStone,totSus,totStone+totSus);

		if(map.flags.updateShadowMap) generate_shadowmap();
	}

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

#if (ENABLE_MULTITHREADING == 0)
	render();
#endif

	if(threadDone == 1 || ENABLE_MULTITHREADING == 0){

		memcpy(rendTexture.pixels,frameBuffer,sizeof(frameBuffer));
	//	for(int y=0;y<rendererSizeY;y++){
	//		for(int x=0;x<rendererSizeX;x++){
	//			rendTexture.pixels[x+y*rendererSizeX] = frameBuffer[x+y*rendererSizeX];
	//		}
	//	}
		//calculate and print fps
		static uint32_t frameCount = 0;
		frameCount++;
		static uint32_t timer_1000ms = 0;
		static int fps;
		if(g_time_ms - timer_1000ms > 1000){
			timer_1000ms = g_time_ms;
			fps = frameCount;
			frameCount = 0;
		}

		char fpsStr[20];
		sprintf(fpsStr,"FPS: %d",fps);
		print(&rendTexture,fpsStr,10,10);

	threadDone = 0;
	}
	SDL_UnlockTexture(rendTexture.Texture);
//    	rendTexture.unlock();

	SDL_LockTexture(hudTexture.Texture,NULL,&(hudTexture.mPixels), &(hudTexture.pitch));
	hudTexture.pixels = (uint32_t*)hudTexture.mPixels;
	renderHud();
	SDL_UnlockTexture(hudTexture.Texture);

	SDL_Rect hudRect;
	hudRect.x = 600;
	hudRect.y = 0;
	hudRect.w = 200;
	hudRect.h = 600;

	SDL_Rect renderRect;
	renderRect.x = 0;
	renderRect.y = 0;
	renderRect.w = 600;
	renderRect.h = 600;


	SDL_RenderCopy(renderer,rendTexture.Texture,NULL,&renderRect); //copy screen texture to renderer
	SDL_RenderCopy(renderer,hudTexture.Texture,NULL,&hudRect); //copy hud texture to renderer

	// Render the changes above
	SDL_RenderPresent( renderer);

//       SDL_GL_SwapWindow(window);
    
}
void main_loop() { loop(); }



EM_BOOL resize_callback(int eventType, const EmscriptenUiEvent* uiEvent, void *userData)
{


	int browserWidth = get_browser_width();
	int browserHeight = get_browser_height();


	int newWidth,newHeight;
	if(browserWidth > browserHeight){
		g_windowScaleY = (float)windowSizeY / (float)browserHeight;
		newHeight = browserHeight;
		newWidth = windowSizeX / g_windowScaleY;
		g_windowScaleX = (float)windowSizeX / (float)newWidth;
	}else{
		g_windowScaleX = (float)windowSizeX / (float)newWidth;
		newWidth = browserWidth;
		newHeight = windowSizeY / g_windowScaleX;
		g_windowScaleY = (float)windowSizeY / (float)browserHeight;
	}

	SDL_SetWindowSize(window, newWidth, newHeight);


	printf("Window resized: browser %dx%d, viewport %dx%d\n", (int)browserWidth, (int)browserHeight, newWidth, newHeight);

printf("%f\n",g_windowScaleY);

	return 1;
}

static inline const char *emscripten_event_type_to_string(int eventType) {
  const char *events[] = { "(invalid)", "(none)", "keypress", "keydown", "keyup", "click", "mousedown", "mouseup", "dblclick", "mousemove", "wheel", "resize",
    "scroll", "blur", "focus", "focusin", "focusout", "deviceorientation", "devicemotion", "orientationchange", "fullscreenchange", "pointerlockchange",
    "visibilitychange", "touchstart", "touchend", "touchmove", "touchcancel", "gamepadconnected", "gamepaddisconnected", "beforeunload",
    "batterychargingchange", "batterylevelchange", "webglcontextlost", "webglcontextrestored", "(invalid)" };
  ++eventType;
  if (eventType < 0) eventType = 0;
  if (eventType >= sizeof(events)/sizeof(events[0])) eventType = sizeof(events)/sizeof(events[0])-1;
  return events[eventType];
}
EM_BOOL mouse_callback(int eventType, const EmscriptenMouseEvent *e, void *userData)
{
  printf("%s, screen: (%ld,%ld), client: (%ld,%ld),%s%s%s%s button: %hu, buttons: %hu, movement: (%ld,%ld), target: (%ld, %ld)\n",
    emscripten_event_type_to_string(eventType), e->screenX, e->screenY, e->clientX, e->clientY,
    e->ctrlKey ? " CTRL" : "", e->shiftKey ? " SHIFT" : "", e->altKey ? " ALT" : "", e->metaKey ? " META" : "",
    e->button, e->buttons, e->movementX, e->movementY, e->targetX, e->targetY);

  if (e->screenX != 0 && e->screenY != 0 && e->clientX != 0 && e->clientY != 0 && e->targetX != 0 && e->targetY != 0)
  {

    //if (eventType == EMSCRIPTEN_EVENT_MOUSEMOVE && (e->movementX != 0 || e->movementY != 0)) gotMouseMove = 1;
  }


  return 0;
}
#if ENABLE_MULTITHREADING
/* this function is run by the second thread */
void *render_thread()
{

while(1){
	if(threadDone == 0){
		render();
		threadDone = 1;
	}

}
/* the function must return something - NULL will do */
return NULL;

}

/* this function is run by the second thread */
void *update_water_thread()
{
	uint32_t timer = 1;
while(1){
	water_update(map.water, 9.81f, 1.f, 1.f, 0.99f, 1.0f/(float)g_dtime_ms);
}
/* the function must return something - NULL will do */
return NULL;

}
#endif
int main()
{
	init();

	int x = 0, y = 0;

	/* show the initial values of x and y */
	printf("x: %d, y: %d\n", x, y);

#if ENABLE_MULTITHREADING
	/* this variable is our reference to the second thread */
	pthread_t update_water_pthread;
	pthread_t render_pthread;
	/* create a second thread which executes inc_x(&x) */
	if(pthread_create(&update_water_pthread, NULL, update_water_thread, NULL)) printf("Error creating thread\n");
	if(pthread_create(&render_pthread, NULL, render_thread, NULL)) printf("Error creating thread\n");
#endif

	/* wait for the second thread to finish */
//	if(pthread_join(inc_x_thread, NULL)) {
//
//	printf("Error joining thread\n");
//
//	}


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
//	SDL_SetTextureBlendMode(hudTexture.Texture,SDL_BLENDMODE_BLEND);


	if (IS_FULLSCREEN){


		EmscriptenFullscreenStrategy strategy;
		memset(&strategy, 0, sizeof(strategy));
		strategy.scaleMode = EMSCRIPTEN_FULLSCREEN_SCALE_ASPECT;
		strategy.filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_DEFAULT;
//		strategy.canvasResizedCallback = on_canvassize_changed;
		//emscripten_enter_soft_fullscreen("canvas", &strategy);
		  EMSCRIPTEN_RESULT ret = emscripten_enter_soft_fullscreen("canvas", &strategy);
		  printf("ret %d\n",ret);
	}

	int browserWidth = get_browser_width();
	int browserHeight = get_browser_height();


	int newWidth,newHeight;
	if(browserWidth > browserHeight){
		g_windowScaleY = (float)windowSizeY / (float)browserHeight;
		newHeight = browserHeight;
		newWidth = windowSizeX / g_windowScaleY;
		g_windowScaleX = (float)windowSizeX / (float)newWidth;
	}else{
		g_windowScaleX = (float)windowSizeX / (float)newWidth;
		newWidth = browserWidth;
		newHeight = windowSizeY / g_windowScaleX;
		g_windowScaleY = (float)windowSizeY / (float)browserHeight;
	}

	SDL_SetWindowSize(window, newWidth, newHeight);


	printf("Window sized: browser %dx%d, viewport %dx%d\n", (int)browserWidth, (int)browserHeight, newWidth, newHeight);


	emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, mouse_callback);
	emscripten_set_resize_callback(NULL, NULL, 0, resize_callback);

    emscripten_set_main_loop(main_loop, 0, true);

    return 0;
}

