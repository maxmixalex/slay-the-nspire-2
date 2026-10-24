#pragma once
#include "core.hpp"
namespace spire {
// Card face coordinates, separate from transparent cache padding. The
// original300x422 frame is centered on its holder; a padded tile must not
// move that pivot or enlarge its mouse hitbox.
struct HandPose {int cx,cy,scale,sine,cosine;};
constexpr int HandTileW=80,HandTileH=112,HandFaceX=8,HandFaceY=6,HandFaceW=69,HandFaceH=97;
constexpr int HandPivotX=(HandFaceX*2+HandFaceW)*512,HandPivotY=(HandFaceY*2+HandFaceH)*512;
constexpr int HandFocusScale=(300*4*1024+21*HandFaceW/2)/(21*HandFaceW);
inline int hand_project(int source){return (source*4+10)/21;}
inline HandPose hand_pose(int count,int index,bool focused,int focus=-1){
    if(count<1)count=1;if(count>MaxHand)count=MaxHand;
    if(index<0)index=0;if(index>=count)index=count-1;
    struct Point {int16_t x,y,sine,cosine;};
    static constexpr Point points[]={
#include "combat_hand_positions.inc"
    };
    static_assert(sizeof points/sizeof points[0]==MaxHand*(MaxHand+1)/2,"Every hand size needs its authored fan");
    const auto p=points[count*(count-1)/2+index];int x=p.x;
    // RefreshLayout pushes each of the three nearest neighbours away from
    // the hovered card. The focused holder keeps its authored horizontal slot.
    if(focus>=0&&focus<count&&focus!=index){int distance=focus>index?focus-index:index-focus;
        if(distance<4)x+=(index<focus?-1:1)*(100-distance*25);}
    if(focused)return {hand_project(840+x),hand_project(1260-211+2),HandFocusScale,0,1024};
    const int percent=count>=8?80*(100-(count-7)*5):8000;
    const int scale=(HandFocusScale*percent+5000)/10000;
    return {hand_project(840+x),hand_project(1260+p.y),scale,p.sine,p.cosine};
}
inline HandPose hand_selected_pose(){
    // NCardPlay.CenterCard: viewport bottom minus half of the .75-scale
    //422px hitbox, with the holder's ordinary position/scale interpolation.
    return {160,(1260*16-422*6+42)/84,(HandFocusScale*3+2)/4,0,1024};
}
struct HandRect {int x,y,w,h;};
inline HandRect hand_bounds(HandPose p){
    const int sn=p.sine<0?-p.sine:p.sine,cs=p.cosine<0?-p.cosine:p.cosine;
    const int half_w=((cs*HandFaceW+sn*HandFaceH)*p.scale+2097151)/2097152;
    const int half_h=((sn*HandFaceW+cs*HandFaceH)*p.scale+2097151)/2097152;
    return {p.cx-half_w,p.cy-half_h,half_w*2,half_h*2};
}
inline bool hand_contains(HandPose p,int x,int y){
    if(p.scale<=0)return false;
    const int64_t dx=x-p.cx,dy=y-p.cy;
    const int64_t u=2*(dx*p.cosine+dy*p.sine),v=2*(-dx*p.sine+dy*p.cosine);
    return u>=-int64_t(HandFaceW)*p.scale&&u<int64_t(HandFaceW)*p.scale&&
           v>=-int64_t(HandFaceH)*p.scale&&v<int64_t(HandFaceH)*p.scale;
}
// Original merchant_room.tscn uses one scene transform for background,
// merchant and the relaxed player. 1920x1080 is uniformly covered by the
// 320x218 room below the top bar, never fitted separately per skeleton.
struct MerchantLayout {
 static constexpr int BodyX=209,BodyY=116,BodyW=55,BodyH=67;
};
struct CreaturePose {
 int x,feet,width,scale,intent,hp,status;
 int body_left=0,body_top=0,body_width=0,body_height=0,center_x=0,center_y=0,intent_x=0,sprite_x=0,sprite_y=0;
};
inline CreaturePose legacy_creature_pose(CreaturePose p){
 p.body_left=p.x-p.width/2;p.body_height=84*p.scale/1024;p.body_top=p.feet-p.body_height;
 p.body_width=p.width;p.center_x=p.x;p.center_y=p.feet-31;p.intent_x=p.x;p.sprite_x=p.x;p.sprite_y=p.feet;return p;
}
inline int creature_health_left(CreaturePose p){return p.body_left+(p.body_width-p.width)/2;}
inline CreaturePose enemy_pose(int count,int index){
 if(count<1)count=1;if(count>MaxEnemies)count=MaxEnemies;
 if(index<0)index=0;if(index>=count)index=count-1;
 if(count==1)return legacy_creature_pose({238,122,60,1024,50,129,149});
 if(count==2)return legacy_creature_pose({178+index*88,122,60,1024,50,129,149});
 if(count<=4){int width=180/count;return legacy_creature_pose({137+width/2+index*width,122,width-4,720,58,129,148});}
 int columns=(count+1)/2,row=index/columns,col=index%columns,width=180/columns;
 int feet=row?145:86;return legacy_creature_pose({137+width/2+col*width,feet,width-5,448,row?113:50,feet+3,feet+18});
}
struct EncounterPositions {EncounterId encounter;uint8_t count;uint16_t zoom;int16_t offset_x,offset_y;int16_t points[6][2];};
inline const EncounterPositions* encounter_positions(EncounterId encounter){
 static constexpr EncounterPositions layouts[]={
#include "combat_encounter_positions.inc"
 };
 unsigned low=0,high=sizeof layouts/sizeof layouts[0];
 while(low<high){const unsigned middle=(low+high)/2;if(unsigned(layouts[middle].encounter)<unsigned(encounter))low=middle+1;else high=middle;}
 return low<sizeof layouts/sizeof layouts[0]&&layouts[low].encounter==encounter?&layouts[low]:nullptr;
}
struct EncounterCamera {int zoom=1000,x=0,y=0;};
inline EncounterCamera encounter_camera(EncounterId encounter){
 if(const auto*p=encounter_positions(encounter))return {p->zoom,p->offset_x,p->offset_y};
 // All remaining camera overrides belong to encounters without a scene.
 switch(encounter){
 case EncounterId::AeonglassBoss:return {900,0,60};
 case EncounterId::BygoneEffigyElite:return {880,0,50};
 case EncounterId::CeremonialBeastBoss:case EncounterId::MechaKnightElite:
 case EncounterId::SlimesNormal:case EncounterId::VantomBoss:return {900,0,50};
 case EncounterId::KnowledgeDemonBoss:return {850,0,70};
 case EncounterId::LagavulinMatriarchBoss:case EncounterId::TerrorEelElite:
 case EncounterId::TheInsatiableBoss:case EncounterId::WaterfallGiantBoss:return {900,0,0};
 default:return {};
 }
}
struct CreatureBounds {uint16_t width,height;};
inline CreatureBounds creature_bounds(EnemyId kind){
 static constexpr CreatureBounds bounds[]={
#include "combat_creature_bounds.inc"
 };
 static_assert(sizeof bounds/sizeof bounds[0]==EnemyKinds,"Every model needs a bounds entry");
 return unsigned(kind)<EnemyKinds?bounds[unsigned(kind)]:CreatureBounds{0,0};
}
struct CreatureAnchors {int16_t left,top,right,bottom,center_x,center_y,intent_x,intent_y;};
inline CreatureAnchors creature_anchors(EnemyId kind){
 static constexpr CreatureAnchors anchors[]={
#include "combat_creature_anchors.inc"
 };
 static_assert(sizeof anchors/sizeof anchors[0]==EnemyKinds,"Every model needs authored anchors");
 return unsigned(kind)<EnemyKinds?anchors[unsigned(kind)]:CreatureAnchors{};
}
inline CreatureAnchors player_anchors(Character character,bool pet=false){
 static constexpr CreatureAnchors anchors[]={
  {-121,-278,121,0,4,-168,30,-310},{-122,-244,122,0,0,-146,0,-302},
  {-125,-260,125,0,4,-179,0,-328},{-115,-335,115,0,0,-178,24,-414},
  {-105,-270,105,0,3,-191,0,-326},{-116,-204,116,0,-15,-109,0,-218}
 };
 return anchors[pet?5:unsigned(character)<5?unsigned(character):0];
}
inline int creature_sprite_units(EnemyId kind){
 static constexpr uint16_t units[]={
#include "combat_sprite_units.inc"
 };
 static_assert(sizeof units/sizeof units[0]==EnemyKinds,"Every model needs an independently registered sprite");
 return unsigned(kind)<EnemyKinds?units[unsigned(kind)]:1024;
}
struct PlayerSpriteRegistration {uint16_t units;int16_t x,y;};
inline PlayerSpriteRegistration player_sprite_registration(Character character,bool pet=false){
 // Raw player bakes predate the scene sampler. Apply the original Spine
 // child's scale and position separately from the creature's Bounds.
 static constexpr PlayerSpriteRegistration registrations[]={
  {4483,5,-19},{3239,16,-21},{3618,4,-27},{4541,2,-20},{5120,-2,-6},{4254,-19,-23}
 };
 return registrations[pet?5:unsigned(character)<5?unsigned(character):0];
}
// Camera-scaled source coordinates in thousandths of a pixel. Scaling the
// formation before dividing preserves the exact 960px half-width and centers
// at fractional zooms. The 4:3 projection divides only after applying the
// original container transform and its optional aspect-fit ratio.
// Retaining removed models keeps survivors stationary after a death.
struct EnemyFormationPoint {int x=0,y=200000;};
inline EnemyFormationPoint unnamed_enemy_point(const State&s,int index,int zoom){
 if(s.enemy_count<1||s.enemy_count>MaxEnemies||index<0||index>=s.enemy_count||zoom<=0)return {};
 int widths[MaxEnemies]{},sum=0;
 for(int i=0;i<s.enemy_count;++i){const auto b=creature_bounds(s.enemy_world.models[i].kind);widths[i]=int(b.width?b.width:360)*zoom;sum+=widths[i];}
 const int area=960000;int gap=70*zoom,span=sum+(s.enemy_count-1)*gap;
 int left=(area-span)/2;if(left<150*zoom)left=150*zoom;int stagger=0;
 if(left+span>area&&s.enemy_count>1){
  gap=(area-150*zoom-sum)/(s.enemy_count-1);if(gap<5*zoom)gap=5*zoom;
  span=sum+(s.enemy_count-1)*gap;left=(area-span)/2;
  if(gap<30*zoom)stagger=60*zoom-(gap-5*zoom)*20/25;
 }
 int x=left+widths[index]/2;for(int i=0;i<index;++i)x+=widths[i]+gap;
 return {x,200*zoom-(index%2?stagger:0)};
}
inline EnemyFormationPoint enemy_formation_point(const State&s,int index,int z){
 const auto*layout=encounter_positions(s.enemy_world.encounter);const auto&model=s.enemy_world.models[index];
 unsigned slot=model.slot;
 // This encounter has HasScene and literal merc/sneaky/fat SlotNames but
 // no Slots override. Its simulation slots intentionally retain spawn order.
 if(s.enemy_world.encounter==EncounterId::GremlinMercNormal)
  slot=model.kind==EnemyId::GremlinMerc?0:model.kind==EnemyId::SneakyGremlin?1:2;
 if(layout&&slot<layout->count)return {(layout->points[slot][0]-960)*z,(layout->points[slot][1]-540)*z};
 return unnamed_enemy_point(s,index,z);
}
inline EnemyFormationPoint player_formation_point(const State&s,int z,bool pet=false){
 const bool centered=s.enemy_world.encounter==EncounterId::KaiserCrabBoss;
 const int ordinary=centered?0:-480000;
 if(!pet)return {ordinary-(s.character==Character::Necrobinder?150*z:0),200*z};
 // PositionLocalPlayerOsty places the initial companion from targetXPos,
 // the player's half Bounds width and GetOstyOffsetFromPlayer. Subsequent
 // HP-driven growth/reposition tweens remain a separate presentation flow.
 const auto bounds=player_anchors(s.character);int hp=s.expansion.osty_max_hp;
 if(hp<0)hp=0;if(hp>150)hp=150;
 return {ordinary+(bounds.right-bounds.left+150)*z+100*z*hp/150,125*z};
}
// NGame.FourByThree is 1680x1260. combat_room.tscn keeps the fixed
// (960,540) SceneContainer pivot, but both creature parents are anchored at
// (840,630). NCombatRoom.AdjustCreatureScaleForAspectRatio runs at Ready
// and viewport changes, not after a summon or a death. A retained value is
// enough to reproduce that contract without retaining another State.
struct CombatSceneLayout {int32_t right=0;};
constexpr int CombatSourceWidth=1680000;
inline CombatSceneLayout combat_layout_for(const State&s){
 if(!s.enemy_world.active)return {};
 const auto camera=encounter_camera(s.enemy_world.encounter);const int z=camera.zoom;
 const int base=960000-120*z+camera.x*1000;
 int right=0;
 const auto include=[&](EnemyFormationPoint p,int width){const int edge=base+p.x+width*z/2;if(edge>right)right=edge;};
 const auto player=player_anchors(s.character);include(player_formation_point(s,z),player.right-player.left);
 if(s.character==Character::Necrobinder||s.expansion.osty_max_hp>0)include(player_formation_point(s,z,true),232);
 for(int i=0;i<s.enemy_count;++i)include(enemy_formation_point(s,i,z),creature_bounds(s.enemy_world.models[i].kind).width);
 right+=15000;return {right>CombatSourceWidth?right:CombatSourceWidth};
}
inline int combat_project_x(EncounterCamera camera,CombatSceneLayout layout,int point,bool enemy){
 const int64_t right=layout.right>CombatSourceWidth?layout.right:CombatSourceWidth;
 const int64_t base=960000-120*camera.zoom+camera.x*1000;
 // EnemyContainer's extra left shift is in SceneContainer local units;
 // its camera scale must be applied after the aspect-fit correction.
 const int64_t shifted=enemy?(right-CombatSourceWidth)*camera.zoom:0;
 const int64_t value=base*right*1000+int64_t(point)*CombatSourceWidth*1000-shifted*CombatSourceWidth;
 return int(value*4/(right*21000000));
}
inline int combat_project_y(EncounterCamera camera,CombatSceneLayout layout,int point){
 const int64_t right=layout.right>CombatSourceWidth?layout.right:CombatSourceWidth;
 const int64_t base=540000+90*camera.zoom+camera.y*1000;
 return int((base*right+int64_t(point)*CombatSourceWidth)*4/(right*21000));
}
inline int combat_project_extent(EncounterCamera camera,CombatSceneLayout layout,int source,int numerator=1,int denominator=1){
 const int64_t right=layout.right>CombatSourceWidth?layout.right:CombatSourceWidth;
 return int(int64_t(source)*camera.zoom*CombatSourceWidth*4*numerator/(right*21000*denominator));
}
inline CreaturePose project_creature_pose(EncounterCamera camera,CombatSceneLayout layout,EnemyFormationPoint point,
                                        CreatureAnchors anchors,bool enemy,int sprite_units,int visual_scale=1024,int bounds_scale=1024,
                                        int sprite_offset_x=0,int sprite_offset_y=0){
 const auto xx=[&](int offset){return combat_project_x(camera,layout,point.x+offset*camera.zoom,enemy);};
 const auto yy=[&](int offset){return combat_project_y(camera,layout,point.y+offset*camera.zoom);};
 const auto sx=[&](int offset,int scale){return combat_project_x(camera,layout,point.x+int(int64_t(offset)*camera.zoom*scale/1024),enemy);};
 const auto sy=[&](int offset,int scale){return combat_project_y(camera,layout,point.y+int(int64_t(offset)*camera.zoom*scale/1024));};
 CreaturePose p{};p.x=xx(0);p.feet=yy(0);p.body_left=sx(anchors.left,bounds_scale);p.body_top=sy(anchors.top,bounds_scale);
 p.body_width=sx(anchors.right,bounds_scale)-p.body_left;p.body_height=sy(anchors.bottom,bounds_scale)-p.body_top;
 p.center_x=sx(anchors.center_x,visual_scale);p.center_y=sy(anchors.center_y,visual_scale);
 p.sprite_x=sx(sprite_offset_x,visual_scale);p.sprite_y=sy(sprite_offset_y,visual_scale);
 p.width=p.body_width<22?22:p.body_width;
 // Bounds are authored interaction rectangles, not a raster normalizer.
 // Recover the exact common clip bake scale before projecting to4:3.
 p.scale=combat_project_extent(camera,layout,sprite_units,visual_scale,1024);
 if(p.scale<1)p.scale=1;
 p.intent_x=xx(anchors.intent_x);p.intent=sy(anchors.intent_y,bounds_scale)-8;if(p.intent<25)p.intent=25;
 // creature.tscn's7px health offset and StateDisplay's additional18px
 // power offset are source pixels. Preserve their order while retaining
 // the compact renderer's6px readable bar and an unobstructed status row.
 int health_offset=combat_project_extent(camera,layout,7);if(health_offset<1)health_offset=1;
 p.hp=p.feet+health_offset;p.status=p.feet+combat_project_extent(camera,layout,25);
 if(p.status<p.hp+9)p.status=p.hp+9;return p;
}
inline CreaturePose player_pose(const State&s,CombatSceneLayout layout,bool pet=false){
 if(!s.enemy_world.active){auto p=legacy_creature_pose({pet?112:60,pet?123:124,pet?34:62,1024,50,pet?131:129,149});return p;}
 const auto camera=encounter_camera(s.enemy_world.encounter);
 const auto registration=player_sprite_registration(s.character,pet);
 const int hp=s.expansion.osty_hp>0?s.expansion.osty_max_hp:0,scale=pet?1024+1024*(hp>150?150:hp<0?0:hp)/150:1024;
 return project_creature_pose(camera,layout,player_formation_point(s,camera.zoom,pet),player_anchors(s.character,pet),false,
  registration.units,scale,scale,registration.x,registration.y);
}
inline CreaturePose enemy_pose(const State&s,int index,CombatSceneLayout layout){
 if(index<0||index>=s.enemy_count||!s.enemy_world.active)return enemy_pose(s.enemy_count,index);
 const auto camera=encounter_camera(s.enemy_world.encounter);const auto kind=s.enemy_world.models[index].kind;
 if(kind==EnemyId::LegacyNibbit)return enemy_pose(s.enemy_count,index);
 auto p=project_creature_pose(camera,layout,enemy_formation_point(s,index,camera.zoom),creature_anchors(kind),true,creature_sprite_units(kind));
 // NHealthBar adds24 source pixels around monster Bounds. The segmented
 // boss alone subtracts35 first; player health bars have no extra width.
 const bool segment=kind==EnemyId::DecimillipedeSegmentBack||kind==EnemyId::DecimillipedeSegmentFront||kind==EnemyId::DecimillipedeSegmentMiddle;
 p.width=p.body_width+combat_project_extent(camera,layout,segment?-11:24);
 if(p.width<22)p.width=22;return p;
}
inline CreaturePose enemy_pose(const State&s,int index){return enemy_pose(s,index,combat_layout_for(s));}
inline int combat_player_x(const State&s){return player_pose(s,combat_layout_for(s)).x;}

}
