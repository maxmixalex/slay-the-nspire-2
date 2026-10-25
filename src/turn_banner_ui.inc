void App::sync_turn_banner(){
    if(!combat_phase(game.phase)&&!(combat_feedback.active&&choice_phase(game.phase)))return;
    const int encounter=game.enemy_world.active?int(game.enemy_world.encounter):-1;
    if(turn_banner_seed!=game.seed||turn_banner_floor!=game.floor||turn_banner_combat!=game.world.combat_count||turn_banner_encounter!=encounter){
        turn_banner.reset();turn_banner_seed=game.seed;turn_banner_floor=game.floor;
        turn_banner_combat=game.world.combat_count;turn_banner_encounter=encounter;turn_banner_number=0;
    }
    if(game.turn>0&&turn_banner_number!=game.turn){
        const bool extra=turn_banner_number>0&&!turn_banner.in_enemy;
        const uint32_t when=int32_t(enemy_visual_clock-animation)>0?enemy_visual_clock:animation;
        if(game.turn==1&&!turn_banner_suppress_intro)turn_banner.begin_battle(when);
        else if(game.turn>1)turn_banner.begin_player(game.turn,when,extra);turn_banner_number=game.turn;
        // A restored encounter consumes this one-time suppression. Later
        // floors must still receive their own Battle Start sequence.
        turn_banner_suppress_intro=false;
    }
}
static void turn_banner_text(Canvas&c,const char*text,int center_x,int y,unsigned scale,int size,uint16_t color){
    if(!text||!*text)return;
    size=c.a.content.font_size(size,c.font_face,false,c.sparse_scene_sizes);
    int x=center_x-int(unsigned(c.width(text,size))*scale/4096)/2;
    // Scale the existing bitmap glyphs, including RLE glyphs. This keeps the
    // original 2 -> 1 enemy label zoom without another font atlas or surface.
    while(*text){const auto g=c.a.content.glyph(utf8(text),size,c.font_face);
        auto pixel=[&](unsigned position,unsigned alpha){
            if(!alpha||!g.w||position>=unsigned(g.w)*g.h)return;const unsigned sx=position%g.w,sy=position/g.w;
            const int left=x+int(sx*scale/4096),right=x+int((sx+1)*scale/4096);
            const int top=y+int(sy*scale/4096),bottom=y+int((sy+1)*scale/4096);
            for(int yy=mx(0,top);yy<mn(c.clip_h,bottom);++yy)for(int xx=mx(0,left);xx<mn(c.clip_w,right);++xx)c.paint(yy*c.pitch+xx,color,alpha);
        };
        if(g.pixels&&g.encoded_size){unsigned position=0;const unsigned pixels=unsigned(g.w)*g.h;for(unsigned i=0;i+1<g.encoded_size&&position<pixels;i+=2){const unsigned count=mn(unsigned(g.pixels[i]),pixels-position);for(unsigned j=0;j<count;++j)pixel(position+j,g.pixels[i+1]);position+=count;}}
        else if(g.pixels)for(unsigned i=0;i<unsigned(g.w)*g.h;++i)pixel(i,g.pixels[i]);
        x+=int(g.advance*scale/4096);
    }
}
static void turn_banner_outline(Canvas&c,const char*text,int center_x,int y,unsigned scale,int size,int border,bool round_left=false){
    // Union the scaled stroke on one bounded scanline. Repainting the label
    // at every offset would accumulate its50% outline into opaque black and
    // repeat glyph lookups/decoding up to253 times during the enemy zoom.
    if(!text||!*text||border<1||border>9)return;
    size=c.a.content.font_size(size,c.font_face,false,c.sparse_scene_sizes);
    uint32_t codepoints[64];int positions[64];unsigned count=0,height=0;
    int x=center_x-(int(unsigned(c.width(text,size))*scale/4096)+int(round_left))/2;
    while(*text&&count<64){const uint32_t cp=utf8(text);const auto g=c.a.content.glyph_metrics(cp,size,c.font_face);
        codepoints[count]=cp;positions[count++]=x;x+=int(g.advance*scale/4096);height=mx(int(height),int(unsigned(g.h)*scale/4096));}
    int reach[10]{};for(int dy=0;dy<=border;++dy){int dx=border;while(dx*dx+dy*dy>border*border)--dx;reach[dy]=dx;}
    uint8_t alpha[ScreenW];const int right=mn(ScreenW,c.clip_w);
    for(int row=mx(0,y-border);row<mn(c.clip_h,y+int(height)+border);++row){
        for(auto&pixel:alpha)pixel=0;
        // A streamed glyph view is used only until the next resource read;
        // no pixel pointer survives another glyph's possible cache miss.
        for(unsigned index=0;index<count;++index){const auto g=c.a.content.glyph(codepoints[index],size,c.font_face);if(!g.pixels||!g.w||!g.h)continue;
            const auto span=[&](unsigned column,unsigned source_row,unsigned length,unsigned coverage){
                if(!coverage)return;const int top=y+int(source_row*scale/4096),bottom=y+int((source_row+1)*scale/4096);
                if(top==bottom)return;const int distance=row<top?top-row:row>=bottom?row-bottom+1:0;if(distance>border)return;
                const int left=positions[index]+int(column*scale/4096)-reach[distance];
                const int end=positions[index]+int((column+length)*scale/4096)+reach[distance];
                for(int xx=mx(0,left);xx<mn(right,end);++xx)if(alpha[xx]<coverage)alpha[xx]=uint8_t(coverage);
            };
            if(g.encoded_size){unsigned position=0,column=0,source_row=0;const unsigned pixels=unsigned(g.w)*g.h;
                for(unsigned at=0;at+1<g.encoded_size&&position<pixels;at+=2){unsigned length=mn(unsigned(g.pixels[at]),pixels-position);position+=length;
                    while(length){const unsigned part=mn(length,unsigned(g.w)-column);span(column,source_row,part,g.pixels[at+1]);column+=part;length-=part;
                        if(column==g.w){column=0;++source_row;}}}
            }else for(unsigned yy=0;yy<g.h;++yy)for(unsigned xx=0;xx<g.w;++xx)span(xx,yy,1,g.pixels[yy*g.w+xx]);
        }
        for(int xx=0;xx<right;++xx)if(alpha[xx])c.paint(row*c.pitch+xx,0,alpha[xx]);
    }
}
static void turn_banner_frame(Canvas&c,TurnBannerPresentation::Frame frame){
    auto&a=c.a;if(frame.kind==TurnBannerPresentation::None||!frame.alpha)return;
    Canvas layer=c;layer.opacity=uint8_t(frame.alpha);layer.font_face=a.content.original_label_fonts()?4:0;layer.sparse_scene_sizes=true;
    const char*key=frame.kind==TurnBannerPresentation::Battle?"BATTLE_START":frame.kind==TurnBannerPresentation::Enemy?"ENEMY_TURN":frame.extra?"PLAYER_TURN_EXTRA":"PLAYER_TURN";
    const char*label=a.content.world_string("gameplay_ui",key);
    // Scene anchors (.5,.433), (.5,.48), (.5,.454) and opposite 50px
    // title/counter tweens. Text stays centered on its scale pivot.
    const int title_size=a.content.font_size(18,layer.font_face,false,true);
    // The readable aimed card reaches y=113. Lift the whole banner with the
    // combat scene so early-turn selection never covers its card title.
    const int y=(frame.kind==TurnBannerPresentation::Enemy||frame.kind==TurnBannerPresentation::Battle?104-int((title_size+2)*frame.scale/8192):115-(title_size+2)/2-frame.offset)-combat_controls::SceneLift;
    const uint16_t gold=rgb(239,200,81);
    const uint16_t color=frame.kind==TurnBannerPresentation::Enemy?blend(gold,rgb(255,0,0),frame.red):gold;
    Canvas outline=layer;outline.opacity=uint8_t(div255(outline.opacity*128));
    const int border=TurnBannerPresentation::outline_pixels(24,frame.scale);
    turn_banner_outline(outline,label,160,y,frame.scale,title_size,border);
    turn_banner_text(layer,label,160,y,frame.scale,title_size,color);
    if(frame.kind==TurnBannerPresentation::Player){
        char number[96]{};
        const auto resolve=[](const void*context,const char*key)->LocalizedValue{return wt_equal(key,"turnNumber")?LocalizedValue{true,*static_cast<const int*>(context),1,nullptr}:LocalizedValue{};};
        LocalizedTemplate(number,sizeof number,resolve,&frame.number).for_language(a.content.language()).format(a.content.world_string("gameplay_ui","TURN_COUNT"));
        layer.font_face=a.content.original_label_fonts()?5:0;Canvas counter_outline=layer;counter_outline.opacity=uint8_t(div255(counter_outline.opacity*128));
        const int counter_border=TurnBannerPresentation::outline_pixels(16);
        turn_banner_outline(counter_outline,number,160,104+frame.offset-combat_controls::SceneLift,4096,7,counter_border,true);
        layer.centered(number,0,104+frame.offset-combat_controls::SceneLift,320,7,rgb(135,206,235));
    }
}
static void turn_banner_screen(Canvas&c){
    auto&a=c.a;if((!combat_phase(a.game.phase)&&!a.combat_feedback.active)||a.paused||a.help||a.panel||a.settings_open||a.frontend_confirm||a.map_window.visible||
       a.detail_card>=0||a.detail_item>=0||a.status_actor>=0||(choice_phase(a.game.phase)&&a.combat_choice_ready()))return;
    const unsigned backstop=a.turn_banner.battle_backstop(a.animation);if(backstop)c.dim(0,88-combat_controls::SceneLift,320,40,backstop);
    turn_banner_frame(c,a.turn_banner.sample_battle(a.animation));
    turn_banner_frame(c,a.turn_banner.sample_enemy(a.animation));
    turn_banner_frame(c,a.turn_banner.sample_player(a.animation));
}
