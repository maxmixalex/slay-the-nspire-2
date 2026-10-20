// Seven original columns and original graph links; scrolling is presentation only.
static const char* world_point_name(reference::PointType type){
 switch(type){
 case reference::PointType::Unknown:return "未知";case reference::PointType::Shop:return "商人";
 case reference::PointType::Treasure:return "宝藏";case reference::PointType::Rest:return "休息处";
 case reference::PointType::Monster:return "战斗";case reference::PointType::Elite:return "精英";
 case reference::PointType::Boss:return "首领";case reference::PointType::Start:return "先古之民";
 default:return "";
 }
}
static int world_point_image(reference::PointType type){
 switch(type){
 case reference::PointType::Unknown:return A_MapEvent;case reference::PointType::Shop:return A_MapShop;
 case reference::PointType::Treasure:return A_MapTreasure;case reference::PointType::Rest:return A_MapCamp;
 case reference::PointType::Elite:case reference::PointType::Boss:return A_MapFinal;
 case reference::PointType::Start:return A_MapMarker;default:return A_MapCombat;
 }
}
static void world_map_screen(Canvas&c,bool inspect){
 auto&a=c.a;auto&s=a.game;const auto*m=world_current_map(s);c.picture(A_MapBackground,0,0);
 if(!m){c.centered("地图尚未生成",0,105,320,12,Red);return;}
 const char* regions[]={"蔓生之地","沉没码头","虫巢","荣光之城"};
 unsigned region=s.world.rooms[s.world.act_index].region;
 char title[80]{};TextBuffer heading{title,sizeof title,0};heading.text("第 ");heading.number(s.world.act_index+1);
 heading.text(" 幕 · ");heading.text(region<4?regions[region]:world_region_key(s));
 c.centered(title,0,25,320,12,Ink);
 int last=0;for(unsigned i=0;i<m->count;i++)if(m->points[i].active)last=mx(last,m->points[i].row);
 const int rows=5,step=32,natural=mx(0,mn(last-rows+1,s.world.map_row));
 const int base=mx(0,mn(mx(0,last-rows+1),natural+a.map_scroll));a.map_scroll=base-natural;
 auto px=[](const reference::MapPoint&p){return 29+int(p.col)*43;};
 auto py=[&](const reference::MapPoint&p){return 188-(int(p.row)-base)*step;};
 const auto*visited=s.world.visited[s.world.act_index];
 for(unsigned i=0;i<m->count;i++){
  const auto&p=m->points[i];if(!p.active)continue;
  for(unsigned j=0;j<p.children.used;j++){
   unsigned next=p.children.slots[j];if(next==reference::NoPoint||next>=m->count)continue;
   const auto&q=m->points[next];if(q.row<base||p.row>=base+rows)continue;
   int y1=py(p)-8,y2=py(q)+8;if(y1<44||y2>202)continue;
   int x1=px(p),x2=px(q);const int dy=y2-y1;
   if(y1>202&&dy){x1+=((x2-x1)*(202-y1))/dy;y1=202;}
   if(y2<44&&y2!=y1){x2=x1+(x2-x1)*(44-y1)/(y2-y1);y2=44;}
   c.line(x1,y1,x2,y2,visited[i]&&visited[next]?Ink:rgb(113,88,59),true);
  }
 }
 const reference::MapPoint* selected=nullptr;
 for(unsigned i=0;i<m->count;i++){
  const auto&p=m->points[i];if(!p.active||p.row<base||p.row>=base+rows)continue;
  const int x=px(p),y=py(p);bool allowed=world_reachable(s,i),focus=allowed&&p.col==a.cursor&&!inspect;
  if(focus){c.picture(A_MapHalo,x-17,y-17,34,34);selected=&p;}
  c.picture(world_point_image(p.type),x-10,y-10,20,20,visited[i]?110:allowed?255:175);
  if(visited[i]){c.line(x-12,y-10,x+12,y+10,Ink);c.line(x-12,y+10,x+12,y-10,Ink);}
  if(i==s.world.current_point)c.picture(character_map_image(a.content,s.character),x-10,y-10,20,20);
  if(allowed&&!inspect)c.hit(x-15,y-15,30,30,HitKind::WorldNode,int(i));
 }
 for(int row=base;row<base+rows&&row<=last;row++)c.num(row,2,181-(row-base)*step,8,rgb(105,76,46));
 c.text("↑",304,45,12,Ink);c.hit(297,42,23,24,HitKind::Scroll,-1,base<last-rows+1);
 c.text("↓",304,183,12,Ink);c.hit(297,177,23,27,HitKind::Scroll,1,base>0);
 c.rect(0,207,320,33,Panel);
 if(inspect)c.centered("↑↓ 浏览路线 · M / Esc 返回",0,215,320,10,Gold);
 else{
  c.centered(selected?world_point_name(selected->type):"选择相连的节点",0,207,320,12,Gold);
  c.centered("←→ 选路 · Enter 进入 · ↑↓ 浏览",0,225,320,10,Muted);
 }
}
