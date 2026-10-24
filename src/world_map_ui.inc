#include "world_map_draw.inc"
static const char* world_point_name(reference::PointType type){
    switch(type){
    case reference::PointType::Unknown:return "未知";case reference::PointType::Shop:return "商人";
    case reference::PointType::Treasure:return "宝藏";case reference::PointType::Rest:return "休息处";
    case reference::PointType::Monster:return "战斗";case reference::PointType::Elite:return "精英";
    case reference::PointType::Boss:return "首领";case reference::PointType::Start:return "先古之民";
    default:return "";
    }
}
static const char* map_normal_key(reference::PointType type){
    switch(type){case reference::PointType::Shop:return "SHOP";case reference::PointType::Treasure:return "CHEST";
    case reference::PointType::Rest:return "REST";case reference::PointType::Monster:return "MONSTER";case reference::PointType::Elite:return "ELITE";default:return "UNKNOWN";}
}
static int world_point_image(reference::PointType type){
    switch(type){case reference::PointType::Unknown:return A_MapEvent;case reference::PointType::Shop:return A_MapShop;
    case reference::PointType::Treasure:return A_MapTreasure;case reference::PointType::Rest:return A_MapCamp;
    case reference::PointType::Monster:return A_MapCombat;case reference::PointType::Elite:return A_MapFinal;default:return -1;}
}
static int map_named_picture(const Content&content,const char*prefix,const char*key,const char*suffix=""){
    char name[128]{};TextBuffer text{name,sizeof name,0};text.text(prefix);text.text(key);text.text(suffix);return content.world_image("map",name);
}
static reference::PointType map_legend_type(unsigned item){
    const reference::PointType types[]={reference::PointType::Unknown,reference::PointType::Shop,reference::PointType::Treasure,reference::PointType::Rest,reference::PointType::Monster,reference::PointType::Elite};
    return types[item<6?item:0];
}
struct MapColors {uint16_t ink,faint,paper;const char*region;};
static MapColors map_colors(unsigned region){
    switch(region){
    case 1:return {rgb(24,15,36),rgb(83,74,98),rgb(159,149,165),"UNDERDOCKS"};
    case 2:return {rgb(39,34,28),rgb(110,119,80),rgb(155,149,98),"HIVE"};
    case 3:return {rgb(29,30,47),rgb(96,113,124),rgb(129,154,151),"GLORY"};
    default:return {rgb(40,35,29),rgb(135,114,86),rgb(167,138,103),"OVERGROWTH"};
    }
}
static void map_background(Canvas&c,const WorldMapView&view,const MapColors&colors,bool fill_paper=true){
    if(fill_paper)c.rect(0,WorldMapTop,320,WorldMapBottom-WorldMapTop,colors.paper);const char*parts[]={"_TOP","_MIDDLE","_BOTTOM"};
    // The original 4:3 top offset is relative to the screen center. Adjacent
    // 1080-high layers scroll together at the map's uniform calculator scale.
    for(int i=0;i<3;++i){int y=view.origin-458+i*238;map_picture(c,map_named_picture(c.a.content,"BG_",colors.region,parts[i]),160,y+119,320,238,MapShader::Original,0,0,255,0,WorldMapTop,WorldMapBottom);}
}
static int map_direction(int dx,int dy){
    int best=0,score=-2147483647;for(int angle=0;angle<360;angle+=5){int v=dx*map_sin(angle+90)+dy*map_sin(angle);if(v>score){score=v;best=angle;}}return best;
}
static void map_paths(Canvas&c,const reference::ReferenceMap&m,const WorldMapLayout&layout,const WorldMapView&view,const MapColors&colors){
    const auto&s=c.a.game;const int dot=c.a.content.world_image("map","DOT");
    for(unsigned i=0;i<m.count;++i){const auto&p=m.points[i];if(!p.active)continue;
        for(unsigned j=0;j<p.children.used;++j){const unsigned next=p.children.slots[j];if(next==reference::NoPoint||next>=m.count)continue;
            const auto&a=layout.points[i];const auto&b=layout.points[next];const int y0=a.y+view.origin,y1=b.y+view.origin;
            if(mx(y0,y1)<WorldMapTop-6||mn(y0,y1)>WorldMapBottom+6)continue;
            const int dx=b.x-a.x,dy=b.y-a.y;const unsigned distance=map_sqrt(unsigned(dx*dx+dy*dy));
            const unsigned count=distance*100/484+1;
            reference::Random noise;noise.reset(s.world.run_seed+uint64_t(s.world.act_index)*reference::MaxPoints*reference::MaxPoints+i*reference::MaxPoints+next);
            const int angle=map_direction(dx,dy)+90;
            for(unsigned at=1;at<count;++at){
                int x=a.x+dx*int(at)/int(count)+noise.range(-1,2),y=y0+dy*int(at)/int(count)+noise.range(-1,2);
                const auto sample=c.a.map_visuals.path_dot(s,i,next,at-1,count-1,c.a.map_clock_ms);
                const int size=map_reticle_project(16,sample.scale,false),rotation=angle+noise.range(-6,7)+(noise.below(2)?180:0);
                // Newly traversed dots use the original fixed ink. A rebuilt
                // saved map uses its act's traveled color instead.
                const uint16_t ink=sample.fresh?rgb(36,31,26):sample.traveled?colors.ink:colors.faint;
                map_picture(c,dot,x,y,size,size,MapShader::Tint,ink,0,255,rotation);
            }
        }
    }
}
static EncounterId map_boss(const State&s,const reference::ReferenceMap&m,unsigned point){
    const auto&q=s.world.rooms[s.world.act_index];return q.bosses[point==m.second_boss&&q.boss_count>1?1:0];
}
static void map_normal_node(Canvas&c,const reference::ReferenceMap&m,unsigned id,int x,int y,int angle,bool allowed,bool focused,const MapColors&colors){
    const auto&s=c.a.game;const auto&type=m.points[id].type;const bool visited=c.a.map_visuals.traveled(s,id);
    const auto visible=world_map_visible_type(s,id);int image=world_point_image(type);
    if(visited&&type==reference::PointType::Unknown&&visible!=type)image=map_named_picture(c.a.content,"UNKNOWN_",map_normal_key(visible));
    const bool highlighted=c.a.map_legend&&type==map_legend_type(c.a.map_legend_item);
    const int pulse=allowed&&!focused&&!c.a.map_legend?24+map_sin(int((c.a.animation*33u%1571u)*360u/1571u))*5/1024:20;
    const int size=focused||highlighted?29:pulse;const uint16_t outline=focused?rgb(255,255,255):colors.paper;
    map_picture(c,map_named_picture(c.a.content,"OUTLINE_",map_normal_key(visible)),x,y,size,size,MapShader::Tint,outline,0,focused?191:255,angle);
    map_picture(c,image,x,y,size,size,MapShader::Normal,0,colors.paper,visited||allowed||highlighted?255:128,angle);
    const auto sample=c.a.map_visuals.circle(id,c.a.map_clock_ms);
    if(sample.alpha){reference::Random circle;circle.reset(s.world.run_seed+uint64_t(m.points[id].row)*132u);
        const int rotation=world_float_truncate(world_random_float(circle,0,360));
        const auto final_size=world_float_round(reference::math::Real::integer(44)*world_map_random_float(circle,world_float(0x3f59999aU),world_float(0x3f666666U)));
        const int circle_size=world_map_round(world_float_round(final_size*reference::math::Real::integer(sample.scale)/reference::math::Real::integer(4096)));
        const char*frames[]={"CIRCLE_0","CIRCLE_1","CIRCLE_2","CIRCLE_3","CIRCLE"};
        map_picture(c,c.a.content.world_image("map",frames[sample.frame]),x,y,circle_size,circle_size,MapShader::Tint,rgb(36,31,26),0,sample.alpha,rotation);
    }
}
static void map_special_node(Canvas&c,const reference::ReferenceMap&m,unsigned id,int x,int y,bool allowed,bool focused,const MapColors&colors){
    const auto&s=c.a.game;const bool traveled=c.a.map_visuals.traveled(s,id);
    if(m.points[id].type==reference::PointType::Start){
        const char*key=original_event_key(OriginalEventId(s.world.rooms[s.world.act_index].ancient));const int size=focused?51:46;
        map_picture(c,map_named_picture(c.a.content,"ANCIENT_",key,"_OUTLINE"),x,y,size,size,MapShader::Tint,focused?rgb(255,255,255):colors.paper,0,focused?191:255);
        map_picture(c,map_named_picture(c.a.content,"ANCIENT_",key),x,y,size,size,MapShader::Tint,traveled||allowed?rgb(36,31,26):rgb(125,106,85),0,traveled||allowed?255:216);
        return;
    }
    const EncounterId boss=map_boss(s,m,id);const auto*d=encounter_definition(boss);if(!d)return;
    const unsigned duration=world_map_clip_duration(boss);const int size=m.config.second_boss?75:100;const uint16_t ink=traveled||allowed?colors.ink:colors.faint;
    if(duration){const auto clip=c.a.content.clip(c.a.content.world_clip("map",d->key));
        if(clip.count){const unsigned frame=world_map_clip_frame(c.a.animation*33u,clip.count,duration);
            map_picture(c,int(clip.first)+int(frame),x,y,focused?size*105/100:size,focused?size*105/100:size,MapShader::Boss,ink,colors.paper);}
    }else{int static_size=boss==EncounterId::VantomBoss?77:82;if(m.config.second_boss)static_size=static_size*3/4;if(focused)static_size=static_size*105/100;
        map_picture(c,map_named_picture(c.a.content,"BOSS_",d->key,"_OUTLINE"),x,y,static_size,static_size,MapShader::Tint,colors.paper);
        map_picture(c,map_named_picture(c.a.content,"BOSS_",d->key),x,y,static_size,static_size,MapShader::Tint,ink);
    }
}
static void map_legend_screen(Canvas&c,const MapColors&colors,int offset_x=0){
    auto&a=c.a;const int x=186+offset_x,y=35;
    map_picture(c,a.content.world_image("map","LEGEND"),x+66,y+88,132,176,MapShader::Original,0,0,255,0,22,235);
    c.centered("图例",x+9,y+6,106,12,colors.ink);c.text("×",x+110,y+4,12,colors.ink);c.action(x+103,y,27,25,UiAction::ToggleMapLegend);
    for(unsigned i=0;i<6;++i){const int row=y+29+int(i)*23;const auto type=map_legend_type(i);const bool selected=a.map_legend_item==i;
        if(selected)c.box(x+10,row-2,111,23,colors.ink);
        map_picture(c,world_point_image(type),x+27,row+8,19,19,MapShader::Normal,0,colors.paper,255,0,22,235);
        c.text(world_point_name(type),x+43,row+2,12,colors.ink,75);c.action(x+9,row-3,114,25,UiAction(int(UiAction::MapLegendUnknown)+int(i)));
    }
}
static uint8_t map_window_product(unsigned byte,unsigned q12){
    return uint8_t((byte*q12+2048)/4096);
}
static void world_map_screen(Canvas&parent,bool inspect,const MapWindowSample*window=nullptr){
    Canvas c=parent;
    if(window){
        c.opacity=map_window_product(c.opacity,window->map.alpha);
        c.brightness=map_window_product(c.brightness,window->map.rgb);
    }
    auto&a=c.a;const auto&s=a.game;const auto*m=world_current_map(s);WorldMapLayout layout;
    if(!m||!world_map_layout(s,layout)){c.centered("地图尚未生成",0,105,320,12,Red);return;}
    a.map_visuals.update(s,a.map_clock_ms);
    WorldMapView view=a.map_view(layout);a.map_scroll=view.scroll;
    const unsigned region=s.world.rooms[s.world.act_index].region;const MapColors colors=map_colors(region);
    // A HUD window uses the authored BG alpha over the retained live room;
    // the standalone Phase::Map path retains its existing paper fallback.
    map_background(c,view,colors,window==nullptr||s.phase==Phase::Map);
    map_paths(c,*m,layout,view,colors);
    uint8_t available[reference::Columns];const unsigned available_count=world_available_points(s,available,reference::Columns);
    if(!inspect&&!a.map_legend&&available_count){bool valid=false;for(unsigned i=0;i<available_count;++i)if(m->points[available[i]].col==a.cursor)valid=true;if(!valid)a.cursor=m->points[available[0]].col;}
    unsigned quest=reference::NoPoint;
    if(event_card_spoils_act(s)==s.world.act_index)for(unsigned col=0;col<reference::Columns&&quest==reference::NoPoint;++col)for(unsigned row=0;row<m->config.rows;++row){unsigned id=m->grid[col][row];if(id<m->count&&m->points[id].type==reference::PointType::Treasure){quest=id;break;}}
    // Points inherit TheMap's modulation, then their own delayed alpha.
    // Paths and Marker are siblings of Points and do not share that delay.
    Canvas points=c;if(window)points.opacity=map_window_product(points.opacity,window->points_alpha);
    for(unsigned i=0;i<m->count;++i){const auto&p=m->points[i];if(!p.active)continue;
        const int x=layout.points[i].x,y=layout.points[i].y+view.origin;
        const bool traveling=a.map_visuals.traveling(),target=a.map_visuals.target()==i;
        const bool allowed=!traveling&&!a.treasure_empty_waiting()&&world_reachable(s,i),focused=target||(allowed&&p.col==a.cursor&&!inspect&&!a.map_legend);
        const int half=world_map_half_size(*m,i);
        const int reticle_height=p.type==reference::PointType::Boss?306:p.type==reference::PointType::Start?208:84;
        int visible_half=a.map_reticle_opacity(i)?mx(half,map_reticle_project(reticle_height/2+12,a.map_reticle_scale(i),p.type==reference::PointType::Boss&&m->config.second_boss)):half;
        const auto circle=a.map_visuals.circle(i,a.map_clock_ms);
        if(circle.alpha)visible_half=mx(visible_half,int(((40*circle.scale+4095)/4096*181+255)/256));
        if(y+visible_half<WorldMapTop||y-visible_half>WorldMapBottom)continue;
        // OnSelected changes the destination to Traveled. Its circle and
        // full icon color identify it while the hover/reticle releases.
        if(p.type==reference::PointType::Start||p.type==reference::PointType::Boss)map_special_node(points,*m,i,x,y,allowed,focused&&!traveling,colors);
        else map_normal_node(points,*m,i,x,y,layout.points[i].angle,allowed,focused&&!traveling,colors);
        if(i==quest)map_picture(points,a.content.world_image("map","QUEST"),x+15,y-13,15,15);
        // The scene's SelectionReticle follows the complete icon/quest tree.
        map_reticle(points,i,x,y,p.type==reference::PointType::Boss?374:p.type==reference::PointType::Start?240:84,
                    reticle_height,p.type==reference::PointType::Boss&&m->config.second_boss);
        if(allowed&&!inspect&&!a.map_legend){const int size=p.type==reference::PointType::Boss?mn(82,2*half):p.type==reference::PointType::Start?48:30;
            const int top=mx(WorldMapTop,y-size/2),bottom=mn(WorldMapBottom,y+size/2);if(bottom-top>=15)points.hit(x-size/2,top,size,bottom-top,HitKind::WorldNode,int(i));}
    }
    const auto marker=a.map_visuals.marker(a.map_clock_ms);
    if(marker.point<m->count){const unsigned id=marker.point;const auto&p=m->points[id];
        // A 16-pixel character marker is the readability adaptation of the
        // original 40-unit marker; it sits above, never replaces a node.
        const int offset=p.type==reference::PointType::Boss?(m->config.second_boss?37:46):p.type==reference::PointType::Start?36:23;
        const int dy=map_source_offset(marker.source_y),width=int((16*marker.scale_x+2048)/4096),height=int((16*marker.scale_y+2048)/4096);
        map_picture(c,character_map_image(a.content,s.character),layout.points[id].x,layout.points[id].y+view.origin-offset+dy,width,height);
    }
    // NMapScreen's paper and graph run behind the HUD without a separate
    // header/footer. The act name already has the original NActBanner entry.
    // Fold only the legend for 320px; leave its toggle on the paper itself.
    if(!a.map_legend){
        c.text("图例",278,27,10,colors.ink);c.action(268,23,52,24,UiAction::ToggleMapLegend);
        if(a.map_can_scroll()&&view.origin<view.maximum){
            c.text("↑",304,53,12,colors.ink);c.hit(297,47,23,28,HitKind::Scroll,-1);
        }
        if(a.map_can_scroll()&&view.origin>view.minimum){
            c.text("↓",304,218,12,colors.ink);c.hit(297,211,23,29,HitKind::Scroll,1);
        }
    }else{
        // Legend is a sibling of TheMap, not a child of its fade/slide.
        Canvas legend=parent;int offset=0;
        if(window){legend.opacity=map_window_product(legend.opacity,window->legend.alpha);
            legend.brightness=map_window_product(legend.brightness,window->legend.rgb);
            offset=map_source_offset(window->legend_x_q12);}
        map_legend_screen(legend,colors,offset);
    }
}

static LocalizedValue map_act_number_value(const void*context,const char*key){
    const char expected[]="actNumber";unsigned i=0;
    while(expected[i]&&key[i]==expected[i])++i;
    return !expected[i]&&!key[i]?LocalizedValue{true,int64_t(*static_cast<const uint8_t*>(context))+1,1,nullptr}:LocalizedValue{};
}
static void world_map_act_banner_screen(Canvas&parent){
    auto&a=parent.a;
    for(unsigned i=0;i<a.map_act_intro.banner_count;++i){
        const auto&banner=a.map_act_intro.banners[i];const auto sample=banner.sample();
        // NActBanner is a MapScreen child, a sibling of TheMap. Each original
        // instance composites independently, including NMapRoom's duplicate.
        Canvas band=parent;band.opacity=map_window_product(band.opacity,sample.background_alpha);
        band.rect(0,96,ScreenW,44,0);
        char number[80]{};const char*format=a.content.world_string("gameplay_ui","ACT_NUMBER");
        LocalizedTemplate(number,sizeof number,map_act_number_value,&banner.act).format(*format?format:"第 {actNumber} 幕");
        const char*key=map_colors(banner.region).region;
        const char*name=a.content.world_title("acts",key);if(!*name)name=key;
        const int number_y=94+map_source_offset(sample.number_y_q12);
        Canvas label=parent;label.opacity=map_window_product(label.opacity,sample.number_alpha);
        Canvas shadow=label;shadow.opacity=uint8_t((unsigned(shadow.opacity)*13+127)/255);
        shadow.centered(number,1,number_y+1,ScreenW,12,0);
        label.centered(number,0,number_y,ScreenW,12,rgb(135,206,235));
        Canvas title=parent;title.opacity=map_window_product(title.opacity,sample.name_alpha);
        Canvas title_shadow=title;title_shadow.opacity=uint8_t((unsigned(title_shadow.opacity)*13+127)/255);
        title_shadow.centered(name,1,110,ScreenW,20,0);
        title.centered(name,0,109,ScreenW,20,rgb(239,200,81));
    }
}
static void world_map_window_screen(Canvas&c){
    auto&a=c.a;auto sample=a.map_window.sample(a.map_clock_ms);
    if(!sample.visible)return;
    if(a.map_window.closing())sample.map_y_q12=a.map_window_y_q12;
    const int old_hits=a.hit_count;
    if(a.map_window.is_open)a.hit_count=0;
    // Original capstones cover the map but hide its black Backstop. TopBar
    // is drawn afterwards and remains independent of all these modulations.
    if(!a.panel||a.panel==3){
        const unsigned alpha=world_map_window::backstop_coverage(sample.backstop_alpha);
        if(alpha)for(int y=0;y<ScreenH;++y)for(int x=0;x<ScreenW;++x)c.paint(y*c.pitch+x,0,alpha);
    }
    world_map_screen(c,!a.map_routes_active(),&sample);
    world_map_act_banner_screen(c);
    if(a.map_window.is_open&&a.panel==3&&a.game.phase!=Phase::Map)c.close_button();
    // Closed immediately restores room input; only the visual tail remains.
    if(!a.map_window.is_open)a.hit_count=old_hits;
}
