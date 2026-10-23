void App::sync_turn_banner(){
    if(!combat_phase(game.phase))return;
    const int encounter=game.enemy_world.active?int(game.enemy_world.encounter):-1;
    if(turn_banner_seed!=game.seed||turn_banner_floor!=game.floor||turn_banner_combat!=game.world.combat_count||turn_banner_encounter!=encounter){
        turn_banner.reset();turn_banner_seed=game.seed;turn_banner_floor=game.floor;
        turn_banner_combat=game.world.combat_count;turn_banner_encounter=encounter;turn_banner_number=0;
    }
    if(game.turn>0&&turn_banner_number!=game.turn){
        const bool extra=turn_banner_number>0&&!turn_banner.in_enemy;
        const uint32_t when=int32_t(enemy_visual_clock-animation)>0?enemy_visual_clock:animation;
        if(game.turn>1)turn_banner.begin_player(game.turn,when,extra);turn_banner_number=game.turn;
    }
}
static void turn_banner_text(Canvas&c,const char*text,int center_x,int y,unsigned scale,int size,uint16_t color){
    if(!text||!*text)return;
    int x=center_x-int(unsigned(c.width(text,size))*scale/4096)/2;
    // Scale the existing bitmap glyphs, including RLE glyphs. This keeps the
    // original 2 -> 1 enemy label zoom without another font atlas or surface.
    while(*text){const auto g=c.a.content.glyph(utf8(text),size);
        auto pixel=[&](unsigned position,unsigned alpha){
            if(!alpha||!g.w)return;const unsigned sx=position%g.w,sy=position/g.w;
            const int left=x+int(sx*scale/4096),right=x+int((sx+1)*scale/4096);
            const int top=y+int(sy*scale/4096),bottom=y+int((sy+1)*scale/4096);
            for(int yy=mx(0,top);yy<mn(c.clip_h,bottom);++yy)for(int xx=mx(0,left);xx<mn(c.clip_w,right);++xx)c.paint(yy*c.pitch+xx,color,alpha);
        };
        if(g.pixels&&g.encoded_size){unsigned position=0;for(unsigned i=0;i<g.encoded_size;i+=2){const unsigned count=g.pixels[i];for(unsigned j=0;j<count;++j)pixel(position+j,g.pixels[i+1]);position+=count;}}
        else if(g.pixels)for(unsigned i=0;i<unsigned(g.w)*g.h;++i)pixel(i,g.pixels[i]);
        x+=int(g.advance*scale/4096);
    }
}
static void turn_banner_frame(Canvas&c,TurnBannerPresentation::Frame frame){
    auto&a=c.a;if(frame.kind==TurnBannerPresentation::None||!frame.alpha)return;
    Canvas layer=c;layer.opacity=uint8_t(frame.alpha);
    const char*key=frame.kind==TurnBannerPresentation::Enemy?"ENEMY_TURN":frame.extra?"PLAYER_TURN_EXTRA":"PLAYER_TURN";
    const char*label=a.content.world_string("gameplay_ui",key);
    // Scene anchors (.5,.433), (.5,.48), (.5,.454) and opposite 50px
    // title/counter tweens. Text stays centered on its scale pivot.
    const int y=frame.kind==TurnBannerPresentation::Enemy?104-int(8*frame.scale/4096):107-frame.offset;
    const uint16_t gold=rgb(239,200,81);
    const uint16_t color=frame.kind==TurnBannerPresentation::Enemy?blend(gold,rgb(255,0,0),frame.red):gold;
    turn_banner_text(layer,label,161,y+1,frame.scale,16,Ink);
    turn_banner_text(layer,label,160,y,frame.scale,16,color);
    if(frame.kind==TurnBannerPresentation::Player){
        char number[96]{};
        const auto resolve=[](const void*context,const char*key)->LocalizedValue{return wt_equal(key,"turnNumber")?LocalizedValue{true,*static_cast<const int*>(context),1,nullptr}:LocalizedValue{};};
        LocalizedTemplate(number,sizeof number,resolve,&frame.number).format(a.content.world_string("gameplay_ui","TURN_COUNT"));
        layer.centered(number,1,105+frame.offset,320,10,Ink);layer.centered(number,0,104+frame.offset,320,10,rgb(135,206,235));
    }
}
static void turn_banner_screen(Canvas&c){
    auto&a=c.a;if(!combat_phase(a.game.phase)||a.paused||a.help||a.panel||a.settings_open||a.frontend_confirm||a.map_window.visible||
       a.detail_card>=0||a.detail_item>=0||a.status_actor>=0||choice_phase(a.game.phase))return;
    turn_banner_frame(c,a.turn_banner.sample_enemy(a.animation));
    turn_banner_frame(c,a.turn_banner.sample_player(a.animation));
}
