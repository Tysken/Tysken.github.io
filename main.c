#include "main.h"

SDL_Window * window;
SDL_Renderer * renderer;



uint32_t g_time_ms;
uint32_t g_dtime_ms;




uint32_t rendTexturePixels[rendererSizeX*rendererSizeY];
uint32_t hudTexturePixels[windowSizeX*windowSizeY];

sdlTexture rendTexture = {NULL,NULL,rendTexturePixels,rendererSizeX,rendererSizeY,rendererSizeX*sizeof(uint32_t)};
sdlTexture hudTexture = {NULL,NULL,hudTexturePixels,windowSizeX,windowSizeY,windowSizeX*sizeof(uint32_t)};







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

argb_t getTileColorWater(int x, int y, int ys, vec2f_t upVec, float shade){
	
		shade = min(shade, 1.f);
		int r,g,b;
		int posID = x + y * map.w;
		float wtrHeight = map.water[4+x*5+y*map.w*5];
		float camZoom = cam.zoom;
		//calculate slope of water for highlighting and caustics
		float slopX = -map.water[4+(x+1)*5 + (y)*map.w*5] + map.water[4+(x-1)*5 + (y)*map.w*5] - map.stone[(x+1) + (y)*map.w] + map.stone[(x-1) + (y)*map.w];
		float slopY = map.water[4+(x)*5 + (y-1)*map.w*5] - map.water[4+(x)*5 + (y+1)*map.w*5] + map.stone[(x) + (y-1)*map.w] - map.stone[(x) + (y+1)*map.w];
		if(1){//fancy water
			int test2 = 0;
			int underwaterFoam = 0; //use this shit
			for(int i=0;i<wtrHeight+1;i++){
				int seaFloorPosX = (x+(int)(upVec.x*i));
				int seaFloorPosY = (y+(int)(upVec.y*i));
//				underwaterFoam += max((int)(map.suSed[seaFloorPosX+seaFloorPosY*map.w]*200) - i*5,0) ;
				if(map.stone[seaFloorPosX+seaFloorPosY*map.w] >= wtrHeight*2 - (float)i*1.25){
					test2 = (x+(int)(upVec.x*i))+(y+(int)(upVec.y*i))*map.w;
					int newX = (x+(int)(upVec.x*i));
					int newY = (y+(int)(upVec.y*i));
					float caustic =  (map.water[4+(newX+1)*5+newY*map.w*5] - map.water[4+(newX-1)*5+newY*map.w*5] + map.stone[test2+1] - map.stone[test2-1] +
					map.water[4+(newX)*5+(newY+1)*map.w*5] - map.water[4+(newX)*5+(newY-1)*map.w*5] + map.stone[test2+1*map.w] - map.stone[test2-1*map.w])*10;	
					if(seaFloorPosX <= 2 || seaFloorPosX >= map.w-2 || seaFloorPosY <= 2 || seaFloorPosY >= map.h-2) caustic = 0;
					float foamShadow = 0;//map.foamLevel[test2] * 0.2;
					underwaterFoam = max(min(underwaterFoam,100),0);
					r =	map.argb[test2].r + (caustic - foamShadow + underwaterFoam*0.5f)*(1-min(shade,1.0f));
					g =	map.argb[test2].g + (caustic - foamShadow + underwaterFoam*0.5f)*(1-min(shade,1.0f));
					b =	map.argb[test2].b + (caustic - foamShadow + underwaterFoam)*(1-min(shade,1.0f));
					
					//lerp between hard and soft shadows depending on the depth
					int shadow;
					if(map.water[4+(newX)*5+(newY)*map.w*5] < 10){ //interpolate between hard and soft shadows
						shadow = map.shadow[test2] + (map.shadowSoft[test2] - map.shadow[test2])*(min(map.water[4+(newX)*5+(newY)*map.w*5]/10.f,1.f));
					}else{//interpolate between soft and no shadows
						shadow = map.shadowSoft[test2] + (0 - map.shadowSoft[test2])*(min(map.water[4+(newX)*5+(newY)*map.w*5]/100.f,1.f));
					}
					r -= shadow;
					g -= shadow;
					b -= shadow;
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
				caustic =  (map.water[4+(seaFloorPosX+1)*5+(seaFloorPosY)*map.w*5] - map.water[4+(seaFloorPosX-1)*5+(seaFloorPosY)*map.w*5] + map.stone[seaFloorPos+1] - map.stone[seaFloorPos-1] +
										map.water[4+(seaFloorPosX)*5+(seaFloorPosY+1)*map.w*5] - map.water[4+(seaFloorPosX)*5+(seaFloorPosY-1)*map.w*5] + map.stone[seaFloorPos+1*map.w] - map.stone[seaFloorPos-1*map.w])*10;	
			}
			r =	map.argb[posID].r + (caustic - foamShadow)*(1-shade);
			g =	map.argb[posID].g + (caustic - foamShadow)*(1-shade);
			b =	map.argb[posID].b + (caustic - foamShadow)*(1-shade);
		}					
		if(1){//water surface
			
			//highligt according to slope
			r += -20+(slopX+slopY)*1;
			g += -10*1+(int)(wtrHeight*camZoom*4*1+((slopX+slopY)*1));
			b += -4*1+(int)(wtrHeight*camZoom*20*1+((slopX+slopY)*1));
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

		}
	//draw foam
//	if(map.foamLevel[x + y*map.w] > 0){
//		r += map.foamLevel[x + y*map.w]*(1-min(shade,1.0f));
//		g += map.foamLevel[x + y*map.w]*(1-min(shade,1.0f));
//		b += map.foamLevel[x + y*map.w]*(1-min(shade,1.0f));
//	}
	
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
										if(Map.waterHeight[mistFloorPosX+mistFloorPosY*Map.w] + Map.groundHeight[mistFloorPosX+mistFloorPosY*Map.w] + Map.lavaHeight[mistFloorPosX+mistFloorPosY*Map.w] > mistHeight - (float)i*0.60f){ //*0.60f gives pretty much no refraction
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

int counter = 0;
//renders one pixel column
void renderColumn(int x, vec2f_t mapCornerBot, vec2f_t upVec,float tileEdgeSlopeRight,float tileEdgeSlopeLeft,float camZoomDiv,float xs,float ys,float xwt,float ywt,float dDxw, float dDyw){
	//save some variables as local
	float camZoom = cam.zoom;
	
	
	//init some variables
	Uint32 argb; //store color of Pixel that will be drawn
	int border=0; //used when skiping pixels to decide when at a edge of a tile that should be a darker shade
	int dPixels = 1; //how many pixels are skipped to get to the next tile
	
	
	
	//init ybuffer with lowest mappoint at current x screen position
		int ybuffer; //ybuffer stores the last drawn lowest position so there is no overdraw
		//set ybuffer to follow the lower edge of the map
		if(x > mapCornerBot.x) ybuffer = mapCornerBot.y+tileEdgeSlopeRight*(x-mapCornerBot.x);
		else ybuffer = mapCornerBot.y+tileEdgeSlopeLeft*(x-mapCornerBot.x);
		//start drawing column
		for(int y=rendTexture.h+100*camZoomDiv;y>0;y-=dPixels){ //the 100 offset is so terrain starts drawing a little bit below the screen
		counter++;
							
			xs = x;
			ys = y;
			int ywti = (int)ywt;
			int xwti = (int)xwt;

			if(ywti > 0 && ywti < map.h && xwti > 0 && xwti < map.w){
				int posID = xwti + ywti*map.w;


//					Map.pos[posID].waterHeight = (PIPE::H[(xwti-1)*4 + (ywti-1)*map.wateridth*4]+
//																				PIPE::H[(xwti)*4 + (ywti-1)*map.wateridth*4]+
//																				PIPE::H[(xwti+1)*4 + (ywti-1)*map.wateridth*4]+
//																				PIPE::H[(xwti-1)*4 + (ywti)*map.wateridth*4]+
//																				PIPE::H[(xwti)*4 + (ywti)*map.wateridth*4]+
//																				PIPE::H[(xwti+1)*4 + (ywti)*map.wateridth*4]+
//																				PIPE::H[(xwti-1)*4 + (ywti+1)*map.wateridth*4]+
//																				PIPE::H[(xwti)*4 + (ywti+1)*map.wateridth*4]+
//																				PIPE::H[(xwti+1)*4 + (ywti+1)*map.wateridth*4])/9.f;



				float gndHeight = map.stone[posID];
				float wtrHeight = map.water[4+xwti*5+ywti*map.w*5];
				float mistHeight = 0;//Map.mistHeight[posID];
				float lavaHeight = 0;//Map.lavaHeight[posID];

				
				ys = ys-(gndHeight+wtrHeight+mistHeight+lavaHeight)*camZoomDiv; //offset y by terrain height
				
				if(ys < ybuffer){ //check if position is obscured by terrain infront of it and draw if not
					if(xs > 0 && xs < rendTexture.w){ //check if position is within texture space
						//get color at worldspace and draw at screenspace 
						int r,g,b;
							
						//draw mist if present
						if(mistHeight > 0){
							
						//	rgb mistRGB = getTileColorMist(xwti, ywti, ys, upVec);
						//	r = mistRGB.r;
						//	g = mistRGB.g;
						//	b = mistRGB.b;
						}else if(wtrHeight > 0){ //draw water if present
							argb_t waterARGB = getTileColorWater(xwti, ywti, ys, upVec,0.f);
							r = waterARGB.r;
							g = waterARGB.g;
							b = waterARGB.b;
						}else if(lavaHeight > 0){ //draw lava if present
						//	rgb lavaRGB = getTileColorLava(xwti, ywti, 0.f);
						//	r = lavaRGB.r;
						//	g = lavaRGB.g;
						//	b = lavaRGB.b;
						
						}else{ //only ground									
							r =	map.argb[posID].r;
							g =	map.argb[posID].g;
							b =	map.argb[posID].b;
							r -= map.shadow[posID];
							g -= map.shadow[posID];
							b -= map.shadow[posID];

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
										r -= (int)(1*camZoomDiv);  
										g -= (int)(1*camZoomDiv);
										b -= (int)(1*camZoomDiv);											
//								 }
						}
							
							
						//clamp color values
						argb = (min(max((int)r,0),255) << 16) | (min(max((int)g,0),255) << 8) | (min(max((int)b,0),255));
						
						
						//only draw visible pixels
						for(int Y=ys;Y<ybuffer;Y++){
							
							//ide för optimisering, Omorganisera så pixlar sparas y + x*h ist så kan man använda std::fill sen för 
							//vertikala pixlar ligger brevid varandra i minnet då
							if(Y > 0 && Y < rendTexture.h ){
								rendTexture.pixels[(int)xs + ((int)Y)*rendTexture.w] = argb;	//draw pixels
							}
						}
					}
					ybuffer = ys; //save current highest point in pixel column
				}else{
						//if pixel is hidden behind terrain, save that position to hide other objects
//						for(int Y=ys;Y<ys+10;Y++){
//							if(xs >= 0 && xs < rendTexture.w && Y >= 0 && Y < rendTexture.h) cam.visibility[(int)xs+(int)Y*rendererSizeX] = 0;
//						}
				}

			}
				dPixels = 0; // reset delta Y pixels
				//this piece of code calculates how many y pixels (dPixels) there is to the next tile
				//and the border thing makes it so it only jumps one pixel when there is a
				//new tile and the next drawing part is darker, thus making the edge of the tile darker
				if(!border){ //!border
					border = 1;
					float testX,testY;
					 if(cam.rot <= 45*3.141592654/180 || cam.rot > 315*3.141592654/180 ){
						testX =  ( (xwt -(int)xwt))/dDxw;
					  testY =  ( (ywt -(int)ywt))/dDyw;
					 }else if(cam.rot <= 135*3.141592654/180 ){
						testX =  ((1-(xwt -(int)xwt))/dDxw);
						testY =  (((ywt -(int)ywt))/dDyw);
					 }else if(cam.rot <= 225*3.141592654/180){
					  testX =  (1 - (xwt -(int)xwt))/dDxw;
						testY =  (1 - (ywt -(int)ywt))/dDyw;
					 }else if(cam.rot <= 315*3.141592654/180 ){
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
			}
	
}



void render(){
        for(int y=0;y<rendTexture.h;y++){
            for(int x=0;x<rendTexture.w;x++){
                Uint32 rgb = (x*4)^(y*4);
                rendTexture.pixels[x + y*rendTexture.w] = rgb;	
            }
        }

		
		float xw,yw,zw;
		float xs,ys;

		//furstum
		xs = 0;
		ys = 0;
		vec2f_t ftl = screen2world(xs,ys);
		xs = rendTexture.w;
		ys = rendTexture.h;
		vec2f_t fbr = screen2world(xs,ys);
		xs = 0;
		ys = rendTexture.h;
		vec2f_t fbl = screen2world(xs,ys);
		xs = 0;
		ys = rendTexture.h+100/cam.zoom;
		vec2f_t fblb = screen2world(xs,ys);
		
		float dxw  = (fbr.x - fbl.x)/rendTexture.w; //delta x worldspace
		float dyw  = (fbr.y - fbl.y)/rendTexture.w; //delta y worldspace
		float dDxw = (ftl.x - fbl.x)/rendTexture.h; //delta x worldspace depth
		float dDyw = (ftl.y - fbl.y)/rendTexture.h; //delta y worldspace depth
		
		//create normalized vector that point up on the screen but in world coorinates, for use with raytracing water refraction
		vec2f_t  upVec = {ftl.x - fbl.x, ftl.y - fbl.y};
		double tempDistLongName = sqrt(upVec.x*upVec.x+upVec.y*upVec.y);
		upVec.x /= tempDistLongName;
		upVec.y /= tempDistLongName;
	
	
		//some stuff that I use
		float camZoom = cam.zoom;
		float camZoomDiv = 1/camZoom;
		float sed,hei,sloX,sloY;
		int water;

		
		
		///////// merge these calculations later
		//calculate screen coordinates of world corners
		vec2f_t tlw =world2screen(1,1);
		vec2f_t trw =world2screen(map.w,1);
		vec2f_t blw =world2screen(1,map.h);
		vec2f_t brw =world2screen(map.w,map.h);
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

		xw = fblb.x; //set world coords to coresponding coords for bottom left of screen
		yw = fblb.y; //then iterate left to right, bottom to top of screen

		for(int x=0;x<rendTexture.w;x++){
			float xwt = xw; //make a copy of world coordinate for leftmost position at current depth
			float ywt = yw; 
		
			renderColumn(x,mapCornerBot,upVec,tileEdgeSlopeRight,tileEdgeSlopeLeft,camZoomDiv,xs,ys,xwt,ywt,dDxw,dDyw);
			
		
			xw += dxw; //update world coords corresponding to one pixel right
			yw += dyw;
		}

		//draw mouse
		for(int y=-1;y<1;y++){
			for(int x=-1;x<1;x++){
				uint32_t rgb = (255 << 16) | (255 << 8) | (255);
				if(input.mouse.x-10 > 0 && input.mouse.x + 10 < rendTexture.w && 
				   	input.mouse.y-10 > 0 && input.mouse.y + 10 < rendTexture.h ){
						rendTexture.pixels[(input.mouse.x+x)+(input.mouse.y+y)*rendTexture.w] = rgb;
				}
			}
		}

	
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
}

void renderHud(){
	for(int y=0;y<hudTexture.h;y++){
		for(int x=0;x<hudTexture.w;x++){
			if(x < MAPW && y < MAPH){
				//argb_t argb = {map.argb[x+y*MAPW].b, map.argb[x+y*MAPW].r, map.argb[x+y*MAPW].g, 255};

				//hudTexture.pixels[x + y*hudTexture.w] = *(uint32_t*)&argb;	
			}

		}
	}

	
}


const Uint8 *keyboardState; // get pointer to key states 


void init(){
	
	keyboardState = SDL_GetKeyboardState(NULL); // get pointer to key states 

	map.w = MAPW;
	map.h = MAPH;

	cam.x = 150;
	cam.y = -300;
	cam.rot = 0;
	cam.zoom = 0.05;
	for(int y=0;y<map.h;y++){
		for(int x=0;x<map.w;x++){


		}
	}

	cursor.amount = 0.2;
	cursor.radius = 2;

	loadFont("assets/VictoriaBold.png"); //load font lol

	loadMap("assets/map.png");

}


void updateInput(){
	//update pos and state of mouse
	input.mouse.state = SDL_GetMouseState(&input.mouse.x,&input.mouse.y);
	//update mouse world pos
	vec2f_t pos = screen2world(input.mouse.x,input.mouse.y);
	cursor.worldX  = pos.x;
	cursor.worldY  = pos.y;
	cursor.screenX = input.mouse.x;
	cursor.screenY = input.mouse.y;
	
	if(input.mouse.state == SDL_BUTTON_LEFT){

		float radius = cursor.radius;
		for(int j=-radius;j<=radius;j++){
			for(int k=-radius;k<=radius;k++){
//				map.water[4+5*(cursor.worldX+k)+(cursor.worldY+j)*map.w*5] +=  cursor.amount*radius*radius*exp(-(k*k+j*j)/(2.f*radius*radius))/(2*3.14159265359*radius*radius)*g_dtime_ms;
				if(cursor.worldX+k >= 0 && cursor.worldY+j >= 0 && cursor.worldX+k < map.w && cursor.worldY+j < map.h){	
					map.stone[cursor.worldX+k+(cursor.worldY+j)*map.w] += cursor.amount*radius*radius*exp(-(k*k+j*j)/(2.f*radius*radius))/(2*3.14159265359*radius*radius)*g_dtime_ms; 
				}
			}
		}

	}

	if(keyboardState[SDL_SCANCODE_A]){

			cam.x += cos(cam.rot-3.1415/4)*300 *g_dtime_ms / 1000;
			cam.y += sin(cam.rot-3.1415/4)*300 *g_dtime_ms / 1000;

	}
	if(keyboardState[SDL_SCANCODE_D]){

			cam.x -= cos(cam.rot-3.1415/4)*300 *g_dtime_ms / 1000;
			cam.y -= sin(cam.rot-3.1415/4)*300 *g_dtime_ms / 1000;

	}
	if(keyboardState[SDL_SCANCODE_W]){

			cam.x -= sin(cam.rot-3.1415/4)*450 *g_dtime_ms / 1000;
			cam.y += cos(cam.rot-3.1415/4)*450 *g_dtime_ms / 1000;

	}
	if(keyboardState[SDL_SCANCODE_S]){

			cam.x += sin(cam.rot-3.1415/4)*450 *g_dtime_ms / 1000;
			cam.y -= cos(cam.rot-3.1415/4)*450 *g_dtime_ms / 1000;

	}
	if(keyboardState[SDL_SCANCODE_R]){
			float rotation = cam.rot; //save camera rotation 
			cam.rot = 0; //set camera rotation to 0

			vec2f_t pos1 = screen2world(rendTexture.w/2,rendTexture.h/2);
			cam.zoom += 1 *cam.zoom *g_dtime_ms / 1000;
			vec2f_t pos2 = world2screen(pos1.x,pos1.y);
			vec2f_t deltapos = {rendTexture.w/2-pos2.x,rendTexture.h/2 -pos2.y};
			//transform coordinate offset to isometric
			double sq2d2 = sqrt(2)/2;
			double sq6d2 = sqrt(6)/2;
			double xw = sq2d2*deltapos.x+sq6d2*deltapos.y;
			double yw = sq6d2*deltapos.y-sq2d2*deltapos.x;
			cam.y += yw;
			cam.x += xw;			
			cam.rot = rotation; //restore camera rotation
	}
	if(keyboardState[SDL_SCANCODE_F]){
			if(cam.zoom > 0.03){
				float rotation = cam.rot; //save camera rotation 
				cam.rot = 0; //set camera rotation to 0

				vec2f_t pos1 = screen2world(rendTexture.w/2,rendTexture.h/2);
				cam.zoom -= 1 *cam.zoom *g_dtime_ms / 1000;
				vec2f_t pos2 = world2screen(pos1.x,pos1.y);
				vec2f_t deltapos = {rendTexture.w/2-pos2.x,rendTexture.h/2 -pos2.y};
				//transform coordinate offset to isometric
				double sq2d2 = sqrt(2)/2;
				double sq6d2 = sqrt(6)/2;
				double xw = sq2d2*deltapos.x+sq6d2*deltapos.y;
				double yw = sq6d2*deltapos.y-sq2d2*deltapos.x;
				cam.y += yw;
				cam.x += xw;			
				cam.rot = rotation; //restore camera rotation
				
			}
	}
	if(keyboardState[SDL_SCANCODE_Q]){
		float angle = 32*3.14/180.f;
		cam.rot = fmod((cam.rot - angle *2 *g_dtime_ms / 1000), 6.283185307);
		if(cam.rot < 0) cam.rot = 6.283185307;
			
	}
	if(keyboardState[SDL_SCANCODE_E]){
		float angle = 32*3.14/180.f;
		cam.rot = fmod((cam.rot + angle *2 *g_dtime_ms / 1000), 6.283185307);
				
	}
}

void process(){

	//add water
	int x = map.w - 10;
	int y = 10;
	map.water[4+x*5+y*map.w*5] += 10.f;

	water_update(map.water, 9.81f, 1.f, 1.f, 0.99f, 0.15f);
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

	init();


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
