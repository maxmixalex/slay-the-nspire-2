// Anchoring follows the owner's pinned combat_ui/player_hand/pile scenes.
// The integer fan, tile dimensions and preview are small-screen adaptations.
namespace {
struct HandTile {
    bool valid=false,playable=false;
    const Content* owner=nullptr;
    Card card{};
    int energy=0;
    char description[512]{};
    uint16_t pixels[HandTileW*HandTileH]{};
    uint8_t alpha[HandTileW*HandTileH]{};
};
static HandTile hand_tiles[MaxHand];
static bool same_text(const char*a,const char*b){while(*a&&*a==*b){++a;++b;}return *a==*b;}
}
static void invalidate_hand_tiles(){for(auto&t:hand_tiles)t.valid=false;}
static void card_title(Canvas&c,const char*name,int x,int y,int width,uint16_t ink){
    for(int dx=-1;dx<=1;dx++)for(int dy=-1;dy<=1;dy++)if(dx||dy)c.centered(name,x+dx,y+dy,width,10,Ink);
    c.centered(name,x,y,width,10,ink);
}
static const HandTile& hand_tile(Canvas&c,int index){
    auto&a=c.a;Card card=a.game.cards[a.game.hand.ids[index]];
    int energy=cost(card);bool playable=energy<=a.game.energy;
    char description[512]{};a.card_description(card,definitions[card.kind].targeted?a.target:-1,description,sizeof description);
    auto&t=hand_tiles[index];
    if(t.valid&&t.owner==&a.content&&t.card.kind==card.kind&&t.card.upgraded==card.upgraded&&t.energy==energy&&
       t.playable==playable&&same_text(t.description,description))return t;
    t.owner=&a.content;t.card=card;t.energy=energy;t.playable=playable;
    for(unsigned i=0;i<sizeof description;i++)t.description[i]=description[i];
    for(int i=0;i<HandTileW*HandTileH;i++){t.pixels[i]=0;t.alpha[i]=0;}
    Canvas tile{a,t.pixels,HandTileW,HandTileW,HandTileH,t.alpha};
    tile.picture(a.content.frame_image(card),3,5,74,104);
    tile.picture(a.content.card_image(card.kind),8,24,64,44);
    tile.ui(card_rarity(card.kind)==4?U_RareBanner:card_rarity(card.kind)==3?U_BigBanner:U_CommonBanner,1,5,78,17);
    card_title(tile,a.content.card_string(card.kind),10,6,63,card.upgraded?Green:White);
    tile.picture(a.content.energy_image(card),0,0,18,18);tile.energy_label(energy,5,0,12);
    tile.centered(definitions[card.kind].type==CardType::Attack?"攻击":definitions[card.kind].type==CardType::Skill?"技能":"能力",7,67,66,10,Muted);
    tile.paragraph(description,9,80,62,2,0,10,White);
    if(tile.paragraph(description,9,80,62,0,0,10)>2)tile.text("…",60,96,10,Gold,13);
    if(card.upgraded)tile.text("+",66,16,10,Green);
    if(!playable)for(int i=0;i<HandTileW*HandTileH;i++)if(t.alpha[i])t.pixels[i]=blend(t.pixels[i],Ink,85);
    t.valid=true;return t;
}
static void draw_hand_tile(Canvas&c,const HandTile&t,HandPose p,bool focused){
    int sn=p.sine<0?-p.sine:p.sine;
    int half_w=((p.cosine*HandTileW+sn*HandTileH)*p.scale)/(2*1024*1024)+2;
    int half_h=((sn*HandTileW+p.cosine*HandTileH)*p.scale)/(2*1024*1024)+2;
    int left=mx(0,p.cx-half_w),right=mn(320,p.cx+half_w);
    int top=mx(0,p.cy-half_h),bottom=mn(240,p.cy+half_h);
    // Inverse affine sampling: only integer additions in the inner loop.
    int du=p.cosine*1024/p.scale,dv=-p.sine*1024/p.scale;
    if(focused){c.box(p.cx-HandTileW/2+1,p.cy-HandTileH/2+3,78,107,t.playable?Gold:Muted);}
    for(int y=top;y<bottom;y++){
        int u=HandTileW*512+((left-p.cx)*p.cosine+(y-p.cy)*p.sine)*1024/p.scale;
        int v=HandTileH*512+(-(left-p.cx)*p.sine+(y-p.cy)*p.cosine)*1024/p.scale;
        for(int x=left;x<right;x++,u+=du,v+=dv){
            if(u<0||v<0||u>=HandTileW*1024||v>=HandTileH*1024)continue;
            int at=(v/1024)*HandTileW+u/1024;
            c.paint(y*c.pitch+x,t.pixels[at],t.alpha[at]);
        }
    }
}
static void combat_pile(Canvas&c,int image,int count,int x,int y,int tab){
    c.picture(image,x,y,23,23);
    char label[16]{};TextBuffer b{label,sizeof label,0};b.number(count);
    card_title(c,label,x-2,y+13,27,Gold);
    c.hit(x-3,y-3,29,28,HitKind::OpenPile,tab);
}
static void combat_screen(Canvas&c){
    auto&a=c.a;auto&s=a.game;c.ui(U_BattleBackground,0,22);
    c.hit(0,44,320,140,HitKind::Battlefield);
    c.actor(0,60,124);c.hp(s.hp,s.max_hp,29,129,62);
    if(s.block){c.picture(A_Block,8,128,14,14);c.num(s.block,10,139,10,Blue);}
    status_strip(c,0,24,149,96);
    for(int i=0;i<s.enemy_count;i++){
        const auto&e=s.enemies[i];int x=s.enemy_count==1?238:178+i*88;
        if(!e.hp){c.centered("已击败",x-32,105,64,10,Muted);continue;}
        c.hit(x-37,48,74,95,HitKind::Enemy,i);c.actor(i+1,x,122);
        c.picture(e.move==2?A_BuffIntent:A_AttackIntent,x-21,50,17,17);
        if(e.move==2)c.text("+2",x,48,12,Green);else c.num(enemy_damage(e),x,48,12,Gold);
        if(e.move==1){c.picture(A_Block,x+19,51,10,10);c.num(5,x+30,48,10,Blue);}
        if(a.target==i)c.text("▼",x-5,72,10,Gold);
        c.hp(e.hp,e.max_hp,x-30,129,60);status_strip(c,i+1,x-31,149,62);
        if(e.block){c.picture(A_Block,x-44,126,13,13);c.num(e.block,x-43,138,10,Blue);}
    }
    for(int i=0;i<3;i++){
        int age=(int)a.animation-a.hit_start[i];
        if(a.last_hit[i]&&age>=0&&age<24){int x=i==0?48:s.enemy_count==1?225:165+(i-1)*88;c.num(a.last_hit[i],x,91-age/2,16,i?Gold:Red);}
    }
    int selected=mn(mx(a.cursor,0),mx(0,s.hand.count-1));
    // Non-focused cards sit on the arc; only the focused preview is enlarged.
    for(int i=0;i<s.hand.count;i++)if(i!=selected)draw_hand_tile(c,hand_tile(c,i),hand_pose(s.hand.count,i,false),false);
    if(s.hand.count){
        draw_hand_tile(c,hand_tile(c,selected),hand_pose(s.hand.count,selected,true),true);
        auto p=hand_pose(s.hand.count,selected,true);
        c.hit(p.cx-HandTileW/2,p.cy-HandTileH/2,HandTileW,68,HitKind::Hand,s.hand.ids[selected],!a.busy&&cost(s.cards[s.hand.ids[selected]])<=s.energy);
        // A stable bottom selection rail prevents the raised preview from
        // stealing its neighbours when the pointer crosses from card to card.
        for(int i=0;i<s.hand.count;i++){
            auto r=hand_rail(s.hand.count,i);
            c.hit(r.x,r.y,r.w,r.h,HitKind::Hand,s.hand.ids[i],!a.busy&&cost(s.cards[s.hand.ids[i]])<=s.energy);
        }
    }else c.centered("没有手牌",80,197,160,12,Muted);
    c.picture(s.character==Character::Silent?a.content.ui_image(U_SilentOrb):A_Energy,6,174,42,42);
    c.num(s.energy,18,182,20,White);
    c.picture(A_EndTurn,254,174,64,29);c.centered("结束回合",254,180,64,10,Gold);
    if(a.busy)c.dim(254,174,64,29,80);
    c.hit(254,173,66,31,HitKind::Command,(int)Key::EndTurn,s.phase==Phase::Combat&&!a.busy);
    combat_pile(c,A_DrawPile,s.draw.count,8,215,1);
    combat_pile(c,A_DiscardPile,s.discard.count,291,215,2);
    if(s.exhaust.count)combat_pile(c,A_ExhaustPile,s.exhaust.count,293,106,3);
    c.text("I 详情",3,160,10,Muted,44);c.hit(0,157,49,17,HitKind::Command,(int)Key::Detail,s.hand.count>0);
    if(a.card_fly>0&&a.fly_card>=0){int age=12-a.card_fly;c.picture(a.content.card_image(a.fly_card),150+age*4,163-age*5,26,19,(unsigned)(a.card_fly*18));}
}
