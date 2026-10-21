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
static void map_background(Canvas&c,const WorldMapView&view,const MapColors&colors){
    c.rect(0,22,320,185,colors.paper);const char*parts[]={"_TOP","_MIDDLE","_BOTTOM"};
    // The original 4:3 top offset is relative to the screen center. Adjacent
    // 1080-high layers scroll together at the map's uniform calculator scale.
    for(int i=0;i<3;++i){int y=view.origin-458+i*238;map_picture(c,map_named_picture(c.a.content,"BG_",colors.region,parts[i]),160,y+119,320,238,MapShader::Original,0,0,255,0,22,207);}
}
static int map_direction(int dx,int dy){
    int best=0,score=-2147483647;for(int angle=0;angle<360;angle+=5){int v=dx*map_sin(angle+90)+dy*map_sin(angle);if(v>score){score=v;best=angle;}}return best;
}
static void map_paths(Canvas&c,const reference::ReferenceMap&m,const WorldMapLayout&layout,const WorldMapView&view,const MapColors&colors){
    const auto&s=c.a.game;const auto*visited=s.world.visited[s.world.act_index];const int dot=c.a.content.world_image("map","DOT");
    for(unsigned i=0;i<m.count;++i){const auto&p=m.points[i];if(!p.active)continue;
        for(unsigned j=0;j<p.children.used;++j){const unsigned next=p.children.slots[j];if(next==reference::NoPoint||next>=m.count)continue;
            const auto&a=layout.points[i];const auto&b=layout.points[next];const int y0=a.y+view.origin,y1=b.y+view.origin;
            if(mx(y0,y1)<WorldMapTop-6||mn(y0,y1)>WorldMapBottom+6)continue;
            const int dx=b.x-a.x,dy=b.y-a.y;const unsigned distance=map_sqrt(unsigned(dx*dx+dy*dy));
            const unsigned count=distance*100/484+1;const bool traveled=visited[i]&&visited[next];
            reference::Random noise;noise.reset(s.world.run_seed+uint64_t(s.world.act_index)*reference::MaxPoints*reference::MaxPoints+i*reference::MaxPoints+next);
            const int angle=map_direction(dx,dy)+90;
            for(unsigned at=1;at<count;++at){
                int x=a.x+dx*int(at)/int(count)+noise.range(-1,2),y=y0+dy*int(at)/int(count)+noise.range(-1,2);
                const int size=traveled?5:4,rotation=angle+noise.range(-6,7)+(noise.below(2)?180:0);
                map_picture(c,dot,x,y,size,size,MapShader::Tint,traveled?colors.ink:colors.faint,0,255,rotation);
            }
        }
    }
}
static EncounterId map_boss(const State&s,const reference::ReferenceMap&m,unsigned point){
    const auto&q=s.world.rooms[s.world.act_index];return q.bosses[point==m.second_boss&&q.boss_count>1?1:0];
}
static const char* map_point_title(const App&a,const reference::ReferenceMap&m,unsigned point){
    if(point>=m.count)return "选择相连的节点";const auto type=m.points[point].type;
    if(type==reference::PointType::Start){const char*name=a.content.world_title("events",original_event_key(OriginalEventId(a.game.world.rooms[a.game.world.act_index].ancient)));if(*name)return name;}
    if(type==reference::PointType::Boss){const auto*d=encounter_definition(map_boss(a.game,m,point));if(d){const char*name=a.content.world_title("encounters",d->key);if(*name)return name;}}
    return world_point_name(world_map_visible_type(a.game,point));
}
static void map_normal_node(Canvas&c,const reference::ReferenceMap&m,unsigned id,int x,int y,int angle,bool allowed,bool focused,const MapColors&colors){
    const auto&s=c.a.game;const auto&type=m.points[id].type;const bool visited=s.world.visited[s.world.act_index][id];
    const auto visible=world_map_visible_type(s,id);int image=world_point_image(type);
    if(visited&&type==reference::PointType::Unknown&&visible!=type)image=map_named_picture(c.a.content,"UNKNOWN_",map_normal_key(visible));
    const bool highlighted=c.a.map_legend&&type==map_legend_type(c.a.map_legend_item);
    const int pulse=allowed&&!focused&&!c.a.map_legend?24+map_sin(int((c.a.animation*33u%1571u)*360u/1571u))*5/1024:20;
    const int size=focused||highlighted?29:pulse;const uint16_t outline=focused?rgb(255,255,255):colors.paper;
    map_picture(c,map_named_picture(c.a.content,"OUTLINE_",map_normal_key(visible)),x,y,size,size,MapShader::Tint,outline,0,focused?191:255,angle);
    map_picture(c,image,x,y,size,size,MapShader::Normal,0,colors.paper,visited||allowed||highlighted?255:128,angle);
    if(visited){reference::Random circle;circle.reset(s.world.run_seed+uint64_t(m.points[id].row)*132u);
        const int rotation=world_float_truncate(world_random_float(circle,0,360));
        const int circle_size=world_map_round(world_float_round(reference::math::Real::integer(44)*world_map_random_float(circle,world_float(0x3f59999aU),world_float(0x3f666666U))));
        map_picture(c,c.a.content.world_image("map","CIRCLE"),x,y,circle_size,circle_size,MapShader::Tint,rgb(36,31,26),0,242,rotation);
    }
}
static void map_special_node(Canvas&c,const reference::ReferenceMap&m,unsigned id,int x,int y,bool allowed,bool focused,const MapColors&colors){
    const auto&s=c.a.game;const bool traveled=s.world.visited[s.world.act_index][id];
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
static void map_legend_screen(Canvas&c,const MapColors&colors){
    auto&a=c.a;const int x=186,y=35;
    map_picture(c,a.content.world_image("map","LEGEND"),x+66,y+88,132,176,MapShader::Original,0,0,255,0,22,235);
    c.centered("图例",x+9,y+6,106,12,colors.ink);c.text("×",x+110,y+4,12,colors.ink);c.action(x+103,y,27,25,UiAction::ToggleMapLegend);
    for(unsigned i=0;i<6;++i){const int row=y+29+int(i)*23;const auto type=map_legend_type(i);const bool selected=a.map_legend_item==i;
        if(selected)c.box(x+10,row-2,111,23,colors.ink);
        map_picture(c,world_point_image(type),x+27,row+8,19,19,MapShader::Normal,0,colors.paper,255,0,22,235);
        c.text(world_point_name(type),x+43,row+2,12,colors.ink,75);c.action(x+9,row-3,114,25,UiAction(int(UiAction::MapLegendUnknown)+int(i)));
    }
}
static void world_map_screen(Canvas&c,bool inspect){
    auto&a=c.a;const auto&s=a.game;const auto*m=world_current_map(s);WorldMapLayout layout;
    if(!m||!world_map_layout(s,layout)){c.centered("地图尚未生成",0,105,320,12,Red);return;}
    const WorldMapView view=world_map_view(s,layout,a.map_scroll);a.map_scroll=view.scroll;
    const unsigned region=s.world.rooms[s.world.act_index].region;const MapColors colors=map_colors(region);map_background(c,view,colors);
    map_paths(c,*m,layout,view,colors);
    uint8_t available[reference::Columns];const unsigned available_count=world_available_points(s,available,reference::Columns);
    if(!inspect&&!a.map_legend&&available_count){bool valid=false;for(unsigned i=0;i<available_count;++i)if(m->points[available[i]].col==a.cursor)valid=true;if(!valid)a.cursor=m->points[available[0]].col;}
    unsigned selected=reference::NoPoint,quest=reference::NoPoint;
    if(event_card_spoils_act(s)==s.world.act_index)for(unsigned col=0;col<reference::Columns&&quest==reference::NoPoint;++col)for(unsigned row=0;row<m->config.rows;++row){unsigned id=m->grid[col][row];if(id<m->count&&m->points[id].type==reference::PointType::Treasure){quest=id;break;}}
    for(unsigned i=0;i<m->count;++i){const auto&p=m->points[i];if(!p.active)continue;
        const int x=layout.points[i].x,y=layout.points[i].y+view.origin;
        const bool allowed=world_reachable(s,i),focused=allowed&&p.col==a.cursor&&!inspect&&!a.map_legend;if(focused)selected=i;
        const int half=world_map_half_size(*m,i);
        const int reticle_height=p.type==reference::PointType::Boss?306:p.type==reference::PointType::Start?208:84;
        const int visible_half=a.map_reticle_opacity(i)?mx(half,map_reticle_project(reticle_height/2+12,a.map_reticle_scale(i),p.type==reference::PointType::Boss&&m->config.second_boss)):half;
        if(y+visible_half<WorldMapTop||y-visible_half>WorldMapBottom)continue;
        if(p.type==reference::PointType::Start||p.type==reference::PointType::Boss)map_special_node(c,*m,i,x,y,allowed,focused,colors);
        else map_normal_node(c,*m,i,x,y,layout.points[i].angle,allowed,focused,colors);
        if(i==quest)map_picture(c,a.content.world_image("map","QUEST"),x+15,y-13,15,15);
        // The scene's SelectionReticle follows the complete icon/quest tree.
        map_reticle(c,i,x,y,p.type==reference::PointType::Boss?374:p.type==reference::PointType::Start?240:84,
                    reticle_height,p.type==reference::PointType::Boss&&m->config.second_boss);
        if(allowed&&!inspect&&!a.map_legend){const int size=p.type==reference::PointType::Boss?mn(82,2*half):p.type==reference::PointType::Start?48:30;
            const int top=mx(WorldMapTop,y-size/2),bottom=mn(WorldMapBottom,y+size/2);if(bottom-top>=15)c.hit(x-size/2,top,size,bottom-top,HitKind::WorldNode,int(i));}
    }
    if(s.world.current_point<m->count){const unsigned id=s.world.current_point;const auto&p=m->points[id];
        // A 16-pixel character marker is the readability adaptation of the
        // original 40-unit marker; it sits above, never replaces a node.
        const int offset=p.type==reference::PointType::Boss?(m->config.second_boss?37:46):p.type==reference::PointType::Start?36:23;
        map_picture(c,character_map_image(a.content,s.character),layout.points[id].x,layout.points[id].y+view.origin-offset,16,16);
    }
    c.rect(0,22,320,22,colors.paper);const char*regions[]={"蔓生之地","沉没码头","虫巢","荣光之城"};
    char title[80]{};TextBuffer heading{title,sizeof title,0};heading.text("第 ");heading.number(s.world.act_index+1);heading.text(" 幕 · ");heading.text(region<4?regions[region]:"");
    c.centered(title,4,25,258,12,colors.ink);c.text("图例 I",268,26,10,colors.ink);c.action(262,22,58,22,UiAction::ToggleMapLegend);
    c.text("↑",304,47,12,colors.ink);c.hit(297,44,23,24,HitKind::Scroll,-1,!a.map_legend&&view.origin<view.maximum);
    c.text("↓",304,182,12,colors.ink);c.hit(297,178,23,27,HitKind::Scroll,1,!a.map_legend&&view.origin>view.minimum);
    c.rect(0,207,320,33,Panel);
    if(a.map_legend){c.centered("↑↓ 选择图例 · I / Esc 返回路线",0,215,320,10,Gold);map_legend_screen(c,colors);}
    else if(inspect)c.centered("↑↓ 浏览路线 · I 图例 · M / Esc 返回",0,215,320,10,Gold);
    else{c.centered(map_point_title(a,*m,selected),0,207,320,12,Gold);c.centered("←→ 选路 · Enter 进入 · ↑↓ 浏览",0,225,320,10,Muted);}
}
