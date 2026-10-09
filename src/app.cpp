#include "app.hpp"
#include "rich_text.hpp"
namespace spire {
static int mn(int a,int b){return a<b?a:b;}static int mx(int a,int b){return a>b?a:b;}
static uint16_t rgb(int r,int g,int b){return (uint16_t)((r>>3)<<11|(g>>2)<<5|(b>>3));}
static const uint16_t Ink=rgb(16,20,22),Panel=rgb(27,29,31),Gold=rgb(223,186,112),White=rgb(239,229,206),Muted=rgb(153,156,150),Red=rgb(168,57,45),Blue=rgb(88,157,188),Green=rgb(118,180,112);
static unsigned div255(unsigned v){v+=128;return (v+(v>>8))>>8;}
static uint16_t blend(uint16_t a,uint16_t b,unsigned alpha){
    unsigned r=div255((a>>11)*(255-alpha)+(b>>11)*alpha);
    unsigned ag=(a>>5)&63,bg=(b>>5)&63;
    unsigned g=div255(ag*(255-alpha)+bg*alpha);
    unsigned bl=div255((a&31)*(255-alpha)+(b&31)*alpha);
    return (uint16_t)((r<<11)|(g<<5)|bl);
}
struct TextBuffer{
    char*p;size_t cap,n;
    void ch(char v){if(n+1<cap){p[n++]=v;p[n]=0;}}
    void text(const char*s){while(*s)ch(*s++);}
    void number(int v){if(v<0){ch('-');v=-v;}char b[12];int n=0;do{b[n++]=(char)('0'+v%10);v/=10;}while(v&&n<12);while(n)ch(b[--n]);}
};
static uint32_t utf8(const char*&p){
    uint32_t x=(uint8_t)*p++;if(x<128)return x;int n=x<224?1:(x<240?2:3);x&=(1u<<(6-n))-1;
    while(n--){uint8_t b=(uint8_t)*p;if(!b)return '?';++p;if((b&192)!=128)return '?';x=(x<<6)|(b&63);}return x;
}
struct Canvas{
    App&a;
    void hit(int x,int y,int w,int h,HitKind k,int value=0,bool enabled=true){a.add_hit(x,y,w,h,k,value,enabled);}
    void close_button(int x=292,int y=26){box(x,y,21,20,Gold);centered("×",x,y,21,12,Gold);hit(x,y,21,20,HitKind::Close);}

    void rect(int x,int y,int w,int h,uint16_t color){for(int yy=mx(0,y);yy<mn(240,y+h);yy++)for(int xx=mx(0,x);xx<mn(320,x+w);xx++)a.pixels[yy*320+xx]=color;}
    void box(int x,int y,int w,int h,uint16_t c){rect(x,y,w,1,c);rect(x,y+h-1,w,1,c);rect(x,y,1,h,c);rect(x+w-1,y,1,h,c);}
    void dim(int x,int y,int w,int h,unsigned alpha){for(int yy=mx(0,y);yy<mn(240,y+h);yy++)for(int xx=mx(0,x);xx<mn(320,x+w);xx++){auto&p=a.pixels[yy*320+xx];p=blend(p,Ink,alpha);}}
    void line(int x,int y,int xx,int yy,uint16_t color,bool dots=false){
        int dx=xx>x?xx-x:x-xx,dy=yy>y?y-yy:yy-y,sx=x<xx?1:-1,sy=y<yy?1:-1,err=dx+dy,step=0;
        for(;;){if(!dots||(step/3)%2==0)rect(x,y,1,1,color);if(x==xx&&y==yy)break;int e=err*2;if(e>=dy){err+=dy;x+=sx;}if(e<=dx){err+=dx;y+=sy;}step++;}
    }
    void picture(int id,int x,int y,int width=0,int height=0,unsigned opacity=255){
        auto p=a.content.image(id);if(!p.pixels||!opacity)return;if(!width)width=p.w;if(!height)height=p.h;
        if(width<=0||height<=0)return;int left=mx(0,-x),right=mn(width,320-x),top=mx(0,-y),bottom=mn(height,240-y);if(left>=right||top>=bottom)return;
        if(p.format==4){
            // Animation frames are prebaked at native size. Skip transparent runs entirely.
            if(width!=p.w||height!=p.h)return;
            const uint8_t* run=p.pixels;
            for(int yy=0;yy<p.h;yy++){
                unsigned spans=run[0]|(unsigned)run[1]<<8;run+=2;
                for(unsigned s=0;s<spans;s++){
                    unsigned sx=run[0]|(unsigned)run[1]<<8,n=run[2]|(unsigned)run[3]<<8,mode=run[4];run+=5;
                    unsigned stride=mode?3:2;
                    if(yy>=top&&yy<bottom){int l=mx(left,(int)sx),h=mn(right,(int)(sx+n));
                        if(l>=h){run+=n*stride;continue;}
                        auto*dst=a.pixels+(y+yy)*320+x+l;const uint8_t*src=run+(l-(int)sx)*stride;
                        for(int xx=l;xx<h;xx++,dst++,src+=stride){unsigned alpha=mode?src[2]:255;if(opacity!=255)alpha=div255(alpha*opacity);uint16_t v=src[0]|(uint16_t)src[1]<<8;*dst=alpha==255?v:blend(*dst,v,alpha);}}
                    run+=n*stride;
                }
            }
            return;
        }
        if(width==p.w&&height==p.h&&!p.alpha&&!p.palette&&opacity==255){
            for(int yy=top;yy<bottom;yy++){auto*dst=a.pixels+(y+yy)*320+x+left;const uint8_t*src=p.pixels+(yy*p.w+left)*2;for(int xx=left;xx<right;xx++,dst++,src+=2)*dst=src[0]|(uint16_t)src[1]<<8;}
            return;
        }
        uint16_t source[320];for(int xx=left;xx<right;xx++)source[xx-left]=(uint16_t)(xx*p.w/width);
        for(int yy=top;yy<bottom;yy++){
            int row=(yy*p.h/height)*p.w;uint16_t*dst=a.pixels+(y+yy)*320+x+left;
            for(int xx=left;xx<right;xx++,dst++){
                int i=row+source[xx-left];unsigned alpha=p.alpha?p.alpha[i]:255;if(opacity!=255)alpha=div255(alpha*opacity);if(!alpha)continue;
                const uint8_t*pixel=p.palette?p.palette+p.pixels[i]*2:p.pixels+i*2;uint16_t color=pixel[0]|(uint16_t)pixel[1]<<8;
                *dst=alpha==255?color:blend(*dst,color,alpha);
            }
        }
    }
    void clip(int id,int x,int y,uint32_t milliseconds,bool anchored=true,bool loop=true){
        auto c=a.content.clip(id);if(!c.count)return;unsigned index=milliseconds/c.frame_ms;
        index=loop?index%c.count:(unsigned)mn((int)index,c.count-1);
        picture((int)c.first+(int)index,x-(anchored?c.pivot_x:0),y-(anchored?c.pivot_y:0));
    }
    void actor(int actor,int x,int y){
        int id=actor?AnimEnemyIdle:AnimHeroIdle;uint32_t elapsed=a.animation*33;
        const auto&m=a.motion[actor];if(m.clip>=0&&(int32_t)(a.animation-m.start)>=0){auto c=a.content.clip(m.clip);uint32_t t=(a.animation-m.start)*33;if(t<(uint32_t)c.count*c.frame_ms){id=m.clip;elapsed=t;}}
        clip(id,x,y,elapsed+((id==AnimEnemyIdle)?actor*347:0));
    }
    int width(const char*p,int size){int w=0;while(*p&&*p!='\n')w+=a.content.glyph(utf8(p),size).advance;return w;}
    void glyph(Glyph g,int x,int y,uint16_t color){
        if(g.pixels)for(int yy=mx(0,-y);yy<mn(g.h,240-y);yy++)for(int xx=mx(0,-x);xx<mn(g.w,320-x);xx++){
            unsigned alpha=g.pixels[yy*g.w+xx];auto&v=a.pixels[(y+yy)*320+x+xx];if(alpha)v=blend(v,color,alpha);
        }
    }
    int text(const char*p,int x,int y,int size=12,uint16_t color=White,int maxwidth=320,int maxlines=1){
        int start=x,line=0;while(*p){uint32_t cp=utf8(p);auto g=a.content.glyph(cp,size);
            if(cp=='\n'||(x>start&&x+g.advance>start+maxwidth)){x=start;y+=size+3;if(++line>=maxlines)break;if(cp=='\n')continue;}
            glyph(g,x,y,color);x+=g.advance;
        }return x;
    }
    int paragraph(const char*p,int x,int y,int w,int lines,int skip=0,int size=12,uint16_t color=White){
        const uint16_t colors[]={color,Gold,rgb(244,122,102),Blue,Green,rgb(190,145,217),rgb(236,164,92),rgb(104,210,205),White,Muted};
        RichReader reader(p);RichChar ch{};int at=x,row=0;
        static const int wave[16]={0,1,1,2,2,2,1,1,0,-1,-1,-2,-2,-2,-1,-1};
        while(reader.next(ch)){auto g=a.content.glyph(ch.codepoint,size);int advance=g.advance+(ch.style.bold?1:0);
            if(ch.codepoint=='\n'||(at>x&&at+advance>x+w)){at=x;row++;if(ch.codepoint=='\n')continue;}
            if(row>=skip&&row<skip+lines){int dy=ch.style.wave?wave[(a.animation/2+ch.index*2)%16]:0;
                int dx=ch.style.jitter?(int)((a.animation/2+ch.index*7)%3)-1:0;
                auto ink=colors[ch.style.color];int yy=y+(row-skip)*(size+3)+dy;
                glyph(g,at+dx,yy,ink);if(ch.style.bold)glyph(g,at+dx+1,yy,ink);}
            at+=advance;
        }return row+1;
    }
    void centered(const char*p,int x,int y,int w,int size=12,uint16_t color=White){text(p,x+mx(0,(w-width(p,size))/2),y,size,color,w);}
    void num(int v,int x,int y,int size=12,uint16_t color=White){char b[16]={};TextBuffer t{b,sizeof b,0};t.number(v);text(b,x,y,size,color);}
    void hp(int value,int total,int x,int y,int w){rect(x,y,w,5,Ink);rect(x,y,w*value/mx(1,total),5,Red);char b[24]={};TextBuffer t{b,sizeof b,0};t.number(value);t.ch('/');t.number(total);centered(b,x,y+3,w,10);}
    void button(const char*p,int x,int y,int w,bool selected,int h=25){
        picture(A_WideButton,x,y,w,h,selected?255:195);
        if(selected){rect(x+8,y+h-2,w-16,1,Gold);text("›",x+5,y+3,12,Gold);}centered(p,x+10,y+(h-18)/2,w-20,12,selected?Gold:White);
    }
};
bool App::init(const uint8_t*p,size_t n,Callbacks cb){callbacks=cb;return content.open(p,n)&&content.card_count()==CardKinds;}
void App::message(const char*s){notice=s;notice_ticks=60;}
void App::animate(int actor,int id,int delay){if(actor<0||actor>2)return;motion[actor]={id,animation+(uint32_t)mx(0,delay)};}
void App::reset_presentation(){pointer_cancel();detail_card=-1;cursor=target=panel=scroll=map_scroll=busy=card_fly=0;for(int i=0;i<3;i++){motion[i].clip=-1;last_hit[i]=hit_start[i]=0;}for(int i=0;i<game.enemy_count;i++)if(game.enemies[i].hp){target=i;break;}}
void App::advance(uint32_t ms){if(ms>250)ms=250;tick_remainder+=ms;while(tick_remainder>=33){tick_remainder-=33;tick();}}
void App::draw_frame(){render();}
void App::tick(){animation++;if(notice_ticks>0)--notice_ticks;if(feedback>0)--feedback;if(busy>0)--busy;if(card_fly>0)--card_fly;}
void App::card_description(Card c,int t,char*out,size_t cap)const{
    if(!cap)return;out[0]=0;if(c.kind>=CardKinds)return;TextBuffer b{out,cap,0};const char*p=content.card_string(c.kind,c.upgraded?2:1);const auto&d=definitions[c.kind];
    while(*p){
        if(*p=='\x1e'){++p;if(game.phase!=Phase::Combat&&game.phase!=Phase::ExhaustChoice){while(*p&&*p!='\x1f')++p;}continue;}
        if(*p=='\x1f'){++p;continue;}
        if(p[0]=='{'&&p[1]&&p[2]=='}'){
            int value=0;switch(p[1]){case 'h':value=d.hp_loss;break;case 'e':value=d.energy_gain+(c.upgraded?d.energy_up:0);break;case 'd':value=(game.phase==Phase::Combat||game.phase==Phase::ExhaustChoice)?damage(game,c,t):d.damage+(c.upgraded?d.damage_up:0);break;case 'b':value=(game.phase==Phase::Combat||game.phase==Phase::ExhaustChoice)?card_block(game,c):d.block+(c.upgraded?d.block_up:0);break;case 'n':value=d.draw+(c.upgraded?d.draw_up:0);break;case 'p':value=d.power+(c.upgraded?d.power_up:0);break;default:b.ch(*p++);continue;}
            b.number(value);p+=3;
        }else b.ch(*p++);}
}
static int choice_count(const App&a){
    switch(a.game.phase){case Phase::Map:return 3;case Phase::Shop:return 8;case Phase::Event:return a.game.event_page>=2?1:2;
    case Phase::Reward:return 4;case Phase::Camp:return 2;case Phase::Upgrade:case Phase::Remove:case Phase::Transform:return a.game.deck_count;
    case Phase::Treasure:return 1;default:return a.game.hand.count;}
}
static void next_target(App&a){for(int i=1;i<=a.game.enemy_count;i++){int t=(a.target+i)%a.game.enemy_count;if(a.game.enemies[t].hp){a.target=t;break;}}}
void App::input(Key k){
    if(!pointer.dispatching){pointer_cancel();pointer.visible=false;}
    if(reference_input(k))return;
    if(detail_card>=0){
        if(k==Key::Back||k==Key::Confirm||k==Key::Detail)detail_card=-1;
        else if(k==Key::Up)detail_scroll=mx(0,detail_scroll-1);
        else if(k==Key::Down)++detail_scroll;
        return;
    }
    if(k==Key::Detail&&!title&&!paused&&!help&&!panel&&game.hand.count){
        open_card_detail(game.cards[game.hand.ids[mn(cursor,game.hand.count-1)]]);return;
    }

    if(k==Key::Quit){quit=true;return;}
    if(help){if(k==Key::Back||k==Key::Confirm)help=false;return;}
    if(title){
        if(k==Key::Up||k==Key::Left)menu=(menu+5)%6;if(k==Key::Down||k==Key::Right)menu=(menu+1)%6;
        if(k==Key::Confirm){
            if(menu<2){new_run(game,0x20261003u+run_number++*7919u,menu==1);title=false;reset_presentation();if(game.practice)message("组合练习：固定起手，练习消耗与能力");}
            else if(menu==2){if(callbacks.load&&callbacks.load(game)){title=false;reset_presentation();message("已恢复完整远征状态");}else message("没有有效的存档");}
            else if(menu==3){map_preview.open=true;if(!map_preview.generated)map_preview.generate();}
            else if(menu==4)help=true;else quit=true;
        }return;
    }
    if(paused){
        if(k==Key::Back){paused=false;return;}if(k==Key::Up||k==Key::Left)menu=(menu+5)%6;if(k==Key::Down||k==Key::Right)menu=(menu+1)%6;
        if(k==Key::Confirm){switch(menu){case 0:paused=false;break;
            case 1:has_save=callbacks.save&&callbacks.save(game);message(has_save?"已保存远征":"保存失败");paused=false;break;
            case 2:if(callbacks.load&&callbacks.load(game)){reset_presentation();message("已载入");}else message("没有有效的存档");paused=false;break;
            case 3:help=true;break;case 4:paused=false;title=true;menu=0;break;case 5:quit=true;break;}}return;
    }
    if(k==Key::Back){
        if(panel){panel=0;scroll=map_scroll=0;}
        else if(game.phase==Phase::Remove){game.phase=Phase::Shop;cursor=6;}
        else if(game.phase==Phase::Upgrade){game.phase=Phase::Camp;cursor=1;}
        else{paused=true;menu=0;}return;
    }
    if(k==Key::Save){has_save=callbacks.save&&callbacks.save(game);message(has_save?"已保存远征":"保存失败");return;}
    if(k==Key::Load){if(callbacks.load&&callbacks.load(game)){reset_presentation();message("已载入");}else message("没有有效的存档");return;}
    if(k==Key::Log){panel=panel==2?0:2;scroll=0;return;}
    if(k==Key::Inspect){panel=panel==1?0:1;scroll=inspect_cursor=0;return;}
    if(k==Key::Map){if(game.practice){message("组合练习没有远征路线");return;}panel=panel==3?0:3;map_scroll=0;return;}
    if(k==Key::Relics){panel=panel==5?0:5;scroll=0;return;}
    if(k==Key::Potions){panel=panel==4?0:4;scroll=0;return;}
    if(panel){
        if(panel==5){if(k==Key::Up||k==Key::Left)scroll=mx(0,scroll-1);if(k==Key::Down||k==Key::Right)scroll=mn(mx(0,game.relic_count-1),scroll+1);return;}
        if(panel==4){
            if(k==Key::Left||k==Key::Right)scroll=1-scroll;if(k==Key::Up||k==Key::Down)next_target(*this);
            if(k==Key::Confirm){if(use_potion(game,scroll,target)){panel=0;busy=8;next_target(*this);if(game.phase==Phase::Reward)cursor=0;message("药水已使用");}else message("此时无法使用，或药水槽为空");}return;
        }
        if(panel==3){if(k==Key::Up)map_scroll=mn(MapRows,map_scroll+1);if(k==Key::Down)map_scroll=mx(-MapRows,map_scroll-1);return;}
        if(panel==1){
            if(k==Key::Up)inspect_cursor=mx(0,inspect_cursor-1);
            if(k==Key::Down)++inspect_cursor;
            scroll=(inspect_cursor/10)*10;
            if(k==Key::Confirm||k==Key::Detail){
                render(); // Keyboard batches may arrive before the next timed frame.
                const HitRegion* pick=nullptr;int n=0;
                for(int i=0;i<hit_count;i++)if(hits[i].kind==HitKind::PileCard){if(n++==inspect_cursor%10){pick=&hits[i];break;}}
                if(pick)open_card_detail({(uint8_t)(pick->value&255),(uint8_t)(pick->value>>8)});
                return;
            }
        }else{if(k==Key::Up)scroll=mx(0,scroll-1);if(k==Key::Down)scroll++;}
        if(panel==1&&(k==Key::Left||k==Key::Right)){pile_tab=(pile_tab+(k==Key::Left?4:1))%5;scroll=inspect_cursor=0;}return;
    }
    if(game.phase==Phase::Complete||game.phase==Phase::Dead||game.phase==Phase::Error){if(k==Key::Confirm){title=true;menu=0;}return;}
    int count=choice_count(*this);if(k==Key::Left)cursor=count?(cursor+count-1)%count:0;if(k==Key::Right)cursor=count?(cursor+1)%count:0;
    if(k==Key::Up||k==Key::Down){
        if(game.phase==Phase::Event){scroll=mx(0,scroll+(k==Key::Up?-1:1));}
        else if(game.phase==Phase::Map){map_scroll=mx(-MapRows,mn(MapRows,map_scroll+(k==Key::Up?1:-1)));}
        else if(game.phase==Phase::Upgrade||game.phase==Phase::Remove||game.phase==Phase::Transform)cursor=(cursor+(k==Key::Up?count-1:1))%mx(1,count);
        else if(game.phase==Phase::Shop)cursor=(cursor+4)%8;
        else if(game.phase==Phase::Camp)cursor=1-cursor;
        else if(game.phase==Phase::Combat||game.phase==Phase::ExhaustChoice)next_target(*this);
    }
    if(busy&&(k==Key::Confirm||k==Key::EndTurn))return;
    Phase before=game.phase;bool acted=false;int hp_before[3]={game.hp,game.enemies[0].hp,game.enemies[1].hp};
    if(k==Key::EndTurn&&game.phase==Phase::Combat){
        uint8_t moves[2]={game.enemies[0].move,game.enemies[1].move};acted=end_turn(game);
        if(acted){for(int i=0;i<game.enemy_count;i++)if(hp_before[i+1])animate(i+1,moves[i]==2?AnimEnemyBuff:AnimEnemyAttack,i*12);animate(0,AnimHeroHurt,7);busy=18+10*(game.enemy_count-1);feedback=10;}
    }
    if(k==Key::Confirm){
        switch(game.phase){
        case Phase::Combat:{
            Card card{};if(cursor<game.hand.count)card=game.cards[game.hand.ids[cursor]];
            acted=play(game,cursor,target);
            if(!acted)message("能量不足，或没有可打出的牌");else{animate(0,definitions[card.kind].type==CardType::Attack?AnimHeroAttack:AnimHeroCast);fly_card=card.kind;card_fly=12;busy=12;feedback=8;}break;}
        case Phase::ExhaustChoice:acted=choose_exhaust(game,cursor);break;
        case Phase::Map:if(map_scroll){map_scroll=0;message("已返回可选楼层，再次确认进入");}else{acted=choose_node(game,cursor);if(!acted)message("这条路线不相连，请选择亮起的节点");}break;
        case Phase::Shop:acted=cursor==7?leave_shop(game):buy(game,cursor);if(!acted)message("金币不足、已售出，或物品槽已满");else if(game.phase==Phase::Shop)message("购买成功");break;
        case Phase::Remove:acted=remove_card(game,cursor);if(acted)message("已移除所选卡牌");break;
        case Phase::Event:acted=event_choice(game,cursor);scroll=0;break;
        case Phase::Transform:acted=transform_card(game,cursor);if(!acted)message("请选择另一张尚未变化的牌");break;
        case Phase::Treasure:acted=take_treasure(game);if(acted)message("宝藏已加入本局");break;
        case Phase::Reward:acted=choose_reward(game,cursor==3?-1:cursor);break;
        case Phase::Camp:acted=camp(game,cursor==1);if(!acted)message("没有可以升级的牌");break;
        case Phase::Upgrade:acted=upgrade_card(game,cursor);if(!acted)message("这张牌已经升级");break;
        default:break;
        }
    }
    if(acted){
        for(int i=0;i<3;i++){int hp=i?game.enemies[i-1].hp:game.hp;int hit=hp_before[i]-hp;if(hit>0){last_hit[i]=hit;hit_start[i]=(int)animation+6;if(i)animate(i,AnimEnemyHurt,6);}}
        if(game.phase!=before){cursor=0;scroll=map_scroll=0;if(game.phase==Phase::Map){for(int i=0;i<3;i++)if(reachable(game,game.next_row*3+i)){cursor=i;break;}}}
        cursor=mn(cursor,mx(0,choice_count(*this)-1));if(target>=game.enemy_count||game.enemies[target].hp==0){target=0;for(int i=0;i<game.enemy_count;i++)if(game.enemies[i].hp){target=i;break;}}
    }
}
static void small_card(Canvas&c,Card card,int x,int y,bool selected,bool playable,int number){
    int type=(int)definitions[card.kind].type;int frame=type==0?A_CardAttack:type==1?A_CardSkill:A_CardPower;
    if(selected){c.rect(x-2,y-2,51,70,rgb(91,74,47));c.box(x-2,y-2,51,70,Gold);}
    c.picture(frame,x,y);c.picture(c.a.content.card_image(card.kind),x+3,y+15);c.picture(A_CardBanner,x,y+1,47,11);
    // Card art and text are independent; the description remains legible below the stage.
    c.centered(c.a.content.card_string(card.kind),x+3,y+48,41,10,card.upgraded?Green:White);
    c.picture(A_Energy,x-1,y-1,15,15);int energy=cost(card);if(energy<0)c.text("X",x+4,y-1,10,White);else c.num(energy,x+4,y-1,10,White);
    if(card.upgraded)c.text("+",x+35,y+1,10,Green);if(number)c.num(number,x+32,y+33,10,Muted);
    if(!playable)c.dim(x,y,47,66,125);
}
static void log_line(App&a,Log l,char*b,size_t n){
    b[0]=0;TextBuffer t{b,n,0};switch(l.kind){
    case LogKind::Turn:t.text("回合 ");t.number(l.a);break;
    case LogKind::Play:t.text("打出 ");t.text(a.content.card_string(l.a));if(l.b)t.ch('+');break;
    case LogKind::Damage:t.text("对敌人 ");t.number(l.b+1);t.text(" 造成 ");t.number(l.a);t.text(" 点伤害（含格挡）");break;
    case LogKind::Block:t.text("获得格挡 ");t.number(l.a);break;
    case LogKind::Exhaust:t.text("消耗 ");t.text(a.content.card_string(l.a));break;
    case LogKind::Draw:t.text("抽牌 ");t.number(l.a);break;
    case LogKind::Shuffle:t.text("弃牌堆洗回抽牌堆");break;
    case LogKind::Enemy:t.text("敌人 ");t.number(l.a+1);t.text(l.b==0?"：撞击":l.b==1?"：切击 + 格挡":"：嘶鸣 + 力量");break;
    case LogKind::Strength:t.text("获得力量 ");t.number(l.a);break;
    case LogKind::Pain:t.text("无惧疼痛 ");t.number(l.a);break;
    case LogKind::Embrace:t.text("黑暗之拥 ");t.number(l.a);break;
    case LogKind::Heal:t.text("恢复生命 ");t.number(l.a);break;
    case LogKind::Reward:case LogKind::Buy:t.text(l.kind==LogKind::Buy?"购买 ":"获得 ");t.text(a.content.card_string(l.a));break;
    case LogKind::Upgrade:t.text("升级 ");t.text(a.content.card_string(l.a));break;
    case LogKind::Gold:t.text("获得金币 ");t.number(l.a);break;
    case LogKind::Remove:t.text("移除 ");t.text(a.content.card_string(l.a));break;
    case LogKind::Transform:t.text("变化 ");t.text(a.content.card_string(l.a));t.text(" → ");t.text(a.content.card_string(l.b));break;
    case LogKind::Event:t.text("事件选择已结算");break;
    case LogKind::Relic:t.text("获得遗物：");if(auto*d=relic_definition((unsigned)l.a))t.text(d->name);break;
    case LogKind::Potion:t.text("使用药水");break;
    case LogKind::LoseHp:t.text("失去生命 ");t.number(l.a);break;
    case LogKind::Energy:t.text("获得能量 ");t.number(l.a);break;
    case LogKind::NoDraw:t.text("本回合不能抽牌");break;
    case LogKind::Weak:t.text("给予虚弱 ");t.number(l.a);break;
    default:t.text("容量上限：本局已安全停止");break;}
}
static void hud(Canvas&c){
    auto&s=c.a.game;c.rect(0,0,320,20,Panel);c.picture(A_Heart,4,3);char b[32]={};TextBuffer t{b,sizeof b,0};t.number(s.hp);t.ch('/');t.number(s.max_hp);c.text(b,21,2,12);
    c.picture(A_Coin,83,3);c.num(s.gold,99,2,12,Gold);c.text("层",140,3,10,Muted);c.num(s.floor,152,2,12);
    int x=175;for(int i=0;i<mn(4,s.relic_count);i++){const auto&d=relic_definitions[s.relic_order[i]];c.picture(d.asset,x,2,14,14);x+=15;}
    c.text("R",235,2,10,Gold);c.hit(175,0,68,20,HitKind::Command,(int)Key::Relics);
    c.text("P",245,3,10,Muted);c.hit(244,0,49,20,HitKind::Command,(int)Key::Potions);
    for(int i=0;i<2;i++)if(s.potions[i]!=Potion::Empty)c.picture(A_FirePotion+(int)s.potions[i]-1,255+i*19,1,15,18);else c.box(255+i*19,5,12,10,rgb(67,70,63));
    c.text("...",299,0,10,Gold);c.hit(295,0,25,20,HitKind::Command,(int)Key::Back);

}
static int room_asset(Room r){switch(r){case Room::Combat:return A_MapCombat;case Room::Event:return A_MapEvent;case Room::Shop:return A_MapShop;case Room::Camp:return A_MapCamp;case Room::Treasure:return A_MapTreasure;default:return A_MapFinal;}}
static const char* room_name(Room r){switch(r){case Room::Combat:return "战斗";case Room::Event:return "未知事件";case Room::Shop:return "商店";case Room::Camp:return "休息处";case Room::Treasure:return "宝藏";default:return "最后的战斗";}}
static void map_screen(Canvas&c,bool inspect=false){
    auto&a=c.a;auto&s=a.game;c.picture(A_MapBackground,0,0);c.centered("蔓生之地 · 远征路线",0,24,320,12,Ink);
    int natural=mx(0,mn(MapRows-4,(int)s.next_row-1));int base=mx(0,mn(MapRows-4,natural+a.map_scroll));
    a.map_scroll=base-natural;const int xs[3]={66,159,252};uint16_t path=rgb(99,78,55),faint=rgb(119,93,63);
    for(int row=base;row<base+3;row++)for(int col=0;col<3;col++){
        const auto&node=s.map[row*3+col];int y=183-(row-base)*44;
        for(int j=0;j<3;j++)if(node.links&(1<<j))c.line(xs[col],y-10,xs[j],y-34,node.visited&&s.map[(row+1)*3+j].visited?Ink:faint,true);
    }
    for(int row=base;row<base+4;row++){
        int y=183-(row-base)*44;c.num(row+1,15,y-8,10,path);
        for(int col=0;col<3;col++){
            int id=row*3+col;const auto&node=s.map[id];bool allowed=reachable(s,id),selected=allowed&&col==a.cursor&&!inspect;
            if(selected)c.picture(A_MapHalo,xs[col]-21,y-21,43,43,(unsigned)(210+(a.animation%30<15?a.animation%15:14-a.animation%15)*3));
            if(node.visited)c.picture(A_MapMarker,xs[col]-12,y-11,23,23);
            else c.picture(room_asset(node.room),xs[col]-12,y-12,24,24,allowed?255:row<(int)s.next_row?70:155);
            if(selected)c.text("▼",xs[col]-5,y-31,10,Ink);
            if(allowed&&!inspect)c.hit(xs[col]-19,y-19,38,38,HitKind::Choice,col);
        }
    }
    c.rect(0,207,320,33,Panel);
    c.text("↑",299,45,12,Ink);c.hit(294,41,26,25,HitKind::Scroll,-1);
    c.text("↓",299,180,12,Ink);c.hit(294,179,26,25,HitKind::Scroll,1);
    if(inspect)c.centered("路线预览 · ↑↓ 滚动 · M / Esc 返回",0,214,320,12,Gold);
    else{char b[64]={};TextBuffer t{b,sizeof b,0};t.text("第 ");t.number(s.next_row+1);t.text(" 层  ");t.text(room_name(s.map[s.next_row*3+a.cursor].room));c.centered(b,0,207,320,12,Gold);c.centered("←→ 选路   Enter 进入   ↑↓ 查看远方",0,225,320,10,Muted);}
}
static int event_text(const State&s){
    if(s.event_id==EventId::Baths){if(s.event_page==0)return 45;if(s.event_page==2)return 47;if(s.event_page==3)return 48;return s.event_count<=1?46:48+mn(9,s.event_count-1);}
    if(s.event_id==EventId::Vegetation)return s.event_page==0?59:s.event_page==4?61:60;
    return s.event_page==0?63:s.event_page==2?64:65;
}
static void event_screen(Canvas&c){
    auto&a=c.a;auto&s=a.game;int e=(int)s.event_id;
    c.picture(A_EventBaths+e,0,20);if(e)c.picture(A_EventVegetationForeground+e-1,0,20);
    c.dim(0,20,320,27,110);c.centered(a.content.string(e==0?44:e==1?58:62),0,23,320,16,Gold);
    c.dim(4,73,312,99,210);int total=c.paragraph(a.content.string(event_text(s)),12,78,293,0);
    a.scroll=mn(a.scroll,mx(0,total-6));c.paragraph(a.content.string(event_text(s)),12,78,293,6,a.scroll);
    c.hit(298,74,21,23,HitKind::Scroll,-1);c.hit(298,146,21,23,HitKind::Scroll,1);
    if(total>6){c.rect(313,79,2,84,rgb(77,74,63));c.rect(313,79+76*a.scroll/mx(1,total-6),2,8,Gold);}
    if(s.event_page>=2){c.button(e==1&&s.event_page==4?"战！":"继续前进",35,201,250,true,29);c.hit(35,201,250,29,HitKind::Choice,0);return;}
    for(int i=0;i<2;i++){
        int y=177+i*31;bool selected=a.cursor==i;char b[150]={};TextBuffer t{b,sizeof b,0};const char*title="";
        if(e==0){title=i?(s.event_page?"离开浴场":"敬而远之"):(s.event_page?"沉溺":"投身其中");
            if(i)t.text(s.event_page?"保留已获得的最大生命":"[green]回复 10 点生命[/green]");else{t.text("[green]最大生命 +2[/green]  [red]伤害 ");t.number(3+s.event_count);t.text("[/red]");}}
        else if(e==1){title=i?"休息":"坚持跋涉";if(i){t.text("[green]回复 ");t.number((s.max_hp*3)/10);t.text(" 生命[/green]，进入战斗");}else{t.text("[gold]金币 +");t.number(s.event_value);t.text("[/gold]  [red]生命 -8[/red]");}}
        else{title=i?"落单变形灵":"大群变形灵";t.text(i?"[green]最大生命 +5[/green]":"[red]失去全部金币[/red]，变化 2 张牌");}
        c.hit(5,y,310,29,HitKind::Choice,i);c.rect(5,y,310,29,selected?rgb(65,52,37):Panel);c.box(5,y,310,29,selected?Gold:rgb(77,71,61));
        if(selected){c.rect(5,y,3,29,Gold);c.text("›",11,y+4,16,Gold);}
        c.text(title,25,y,12,selected?Gold:White,274);c.paragraph(b,25,y+14,280,1,0,10);
    }

}
static const char* relic_name(int r){auto*d=relic_definition(r);return d?d->name:"售罄";}
static const char* relic_desc(int r){auto*d=relic_definition(r);return d?d->description:"没有剩余的已实现遗物。";}
static const char* potion_name(int p){return p==1?"火焰药水":p==2?"格挡药水":p==3?"血液药水":"空药水槽";}
static const char* potion_desc(int p){return p==1?"对所选敌人造成 20 点无增幅伤害。":p==2?"在战斗中获得 12 点格挡。":p==3?"回复最大生命的 20%。":"可从商店购买药水，最多携带两瓶。";}
static void shop_screen(Canvas&c){
    auto&a=c.a;auto&s=a.game;c.picture(A_ShopBackground,0,20);c.dim(0,20,320,220,35);
    c.clip(AnimMerchantBottom,155,52,a.animation*33,false);c.clip(AnimMerchantTop,243,21,a.animation*33,false);
    c.text("商人",11,25,20,Gold);c.text("←→ 选商品 · Enter 购买",10,51,10,White);
    c.dim(5,66,227,35,210);char b[256]={};const char*desc="";
    if(a.cursor<4){a.card_description({s.shop_cards[a.cursor],0},-1,b,sizeof b);desc=b;}
    else if(a.cursor==4)desc=relic_desc(s.shop_relic);
    else if(a.cursor==5)desc=potion_desc(s.shop_potion);
    else if(a.cursor==6)desc="永久移除一张牌。每次商店仅限一次。";
    else desc="离开商店，继续选择下一层的路线。";
    c.paragraph(desc,11,68,214,2,0,10);
    for(int i=0;i<4;i++){
        int x=21+77*i;bool sold=(s.shop_sold&(1<<i))!=0;small_card(c,{s.shop_cards[i],0},x,106,a.cursor==i,!sold,0);c.hit(x-3,103,53,85,HitKind::Choice,i,!sold);
        if(sold)c.centered("售罄",x-7,175,61,10,Muted);else{c.picture(A_Coin,x+2,174,11,11);c.num(s.shop_prices[i],x+17,173,12,s.gold>=s.shop_prices[i]?Gold:Muted);}
    }
    c.dim(0,194,320,46,155);
    for(int i=4;i<8;i++){
        int x=(i-4)*80;c.hit(x+2,195,76,43,HitKind::Choice,i,i==7||!(s.shop_sold&(1<<i)));if(a.cursor==i)c.box(x+2,195,76,43,Gold);
        if(i<6){int im=i==4?(relic_definition(s.shop_relic)?relic_definition(s.shop_relic)->asset:A_Chest):A_FirePotion+s.shop_potion-1;
            c.picture(im,x+6,199,22,26);c.text(i==4?relic_name(s.shop_relic):potion_name(s.shop_potion),x+29,198,10,White,48);
            if(s.shop_sold&(1<<i))c.text("售罄",x+29,216,10,Muted);else c.num(s.shop_prices[i],x+31,214,12,Gold);
        }else{c.centered(i==6?"移除卡牌":"离开",x,199,80,12,White);if(i==6){if(s.shop_sold&64)c.centered("已使用",x,216,80,10,Muted);else c.num(remove_price(s),x+27,214,12,Gold);}else c.centered("返回地图",x,216,80,10,Muted);}
    }
}
static void deck_select(Canvas&c){
    auto&a=c.a;auto&s=a.game;c.dim(0,20,320,220,235);const char*name=s.phase==Phase::Upgrade?"选择一张牌升级":s.phase==Phase::Remove?"永久移除一张牌":"选择两张不同的牌变化";
    c.centered(name,0,25,320,16,Gold);int first=mx(0,a.cursor-5);
    for(int i=first;i<mn(s.deck_count,first+6);i++){
        int y=57+(i-first)*22;c.hit(12,y,296,22,HitKind::Choice,i);if(i==a.cursor)c.rect(12,y,296,22,rgb(66,53,35));Card card=s.deck[i];
        c.num(i+1,15+6,y+1,12,Muted);c.text(a.content.card_string(card.kind),51,y+1,12,card.upgraded?Green:White);
        if(s.phase==Phase::Transform&&i==s.event_first)c.text("已变化",246,y+2,10,Muted);else if(card.upgraded)c.text("+",239,y,12,Green);
    }
    if(s.deck_count){Card card=s.deck[mn(a.cursor,s.deck_count-1)];if(s.phase==Phase::Upgrade)card.upgraded=1;char b[320]={};a.card_description(card,-1,b,sizeof b);c.paragraph(b,14,196,292,2);}
    if(s.phase==Phase::Transform){c.text("还需选择",15,224,10,Muted);c.num(s.event_remaining,61,222,12,Gold);c.text("张 · 变化结果来自已实现牌池",75,224,10,Muted);}
    else c.text("↑↓ 选择 · Enter 确认 · Esc 取消",17,224,10,Muted);
}
static void combat_screen(Canvas&c){
    auto&a=c.a;auto&s=a.game;c.picture(A_Battle,0,20);c.hit(0,20,320,120,HitKind::Battlefield);c.actor(0,62,125);
    if(s.block){c.picture(A_Block,7,120);c.num(s.block,23,119,12,Blue);}
    if(s.strength){c.text("力量",69,123,10,Gold);c.num(s.strength,92,122,10,Gold);}
    if(s.no_draw)c.text("不能抽牌",7,40,10,Gold);
    if(s.pain||s.embrace){char b[48]={};TextBuffer t{b,sizeof b,0};if(s.pain){t.text("疼痛 ");t.number(s.pain);}if(s.embrace){t.text("  拥 ");t.number(s.embrace);}c.text(b,7,24,10,Green);}
    for(int i=0;i<s.enemy_count;i++){
        const auto&e=s.enemies[i];int x=s.enemy_count==1?225:174+i*93;
        if(!e.hp){c.centered("已击败",x-34,104,68,10,Muted);continue;}
        c.hit(x-40,38,80,101,HitKind::Enemy,i);c.actor(i+1,x,119);c.picture(e.move==2?A_BuffIntent:A_AttackIntent,x-24,40);
        if(e.move==2)c.text("+2",x-3,39,12,Green);else c.num(enemy_damage(e),x-3,39,12,Gold);
        if(e.move==1){c.picture(A_Block,x+21,42,10,10);c.num(5,x+32,40,10,Blue);}
        if(e.weak){c.text("虚弱",x-31,90,10,Green);c.num(e.weak,x-9,90,10,Green);}
        if(a.target==i)c.text("▼",x-5,60,10,Gold);c.hp(e.hp,e.max_hp,x-32,122,64);
        if(e.vulnerable){c.text("易伤",x-31,76,10,Blue);c.num(e.vulnerable,x-9,76,10,Blue);}if(e.block){c.picture(A_Block,x+15,73,12,12);c.num(e.block,x+27,73,10,Blue);}
    }
    for(int i=0;i<3;i++){
        int age=(int)a.animation-a.hit_start[i];if(a.last_hit[i]&&age>=0&&age<24){int x=i==0?48:s.enemy_count==1?212:160+(i-1)*93;c.num(a.last_hit[i],x,83-age/2,16,i?Gold:Red);}
    }
    c.rect(0,140,320,100,Ink);c.rect(0,139,320,1,rgb(122,99,59));
    if(s.hand.count){Card selected=s.cards[s.hand.ids[mn(a.cursor,s.hand.count-1)]];char b[320]={};a.card_description(selected,definitions[selected.kind].targeted?a.target:-1,b,sizeof b);c.paragraph(b,7,142,308,2);}
    else c.text("没有手牌，按空格结束回合",7,142,12,Muted);
    if(s.phase==Phase::ExhaustChoice){c.rect(0,21,320,18,Red);c.centered("选择一张手牌消耗 · Enter 确认",0,21,320,12);}
    c.picture(A_Energy,5,173);c.num(s.energy,17,177,20,White);c.picture(A_DrawPile,5,206,13,13);c.num(s.draw.count,20,204,10,Muted);
    c.picture(A_EndTurn,1,219,51,19);c.centered("结束回合",1,221,51,10,Gold);c.hit(1,219,53,21,HitKind::Command,(int)Key::EndTurn,s.phase==Phase::Combat);c.hit(1,199,53,20,HitKind::OpenPile,1);
    const int step=s.hand.count>1?mn(51,207/(s.hand.count-1)):51;
    auto hand_card=[&](int j){
        Card card=s.cards[s.hand.ids[j]];int x=60+j*step,y=j==a.cursor?169:174;
        small_card(c,card,x,y,j==a.cursor,s.phase==Phase::ExhaustChoice||cost(card)<=s.energy,j+1);
        if(a.pointer.armed_card==s.hand.ids[j])c.box(x-2,y-2,51,69,Blue);
    };
    for(int j=0;j<s.hand.count;j++)if(j!=a.cursor)hand_card(j);
    if(s.hand.count)hand_card(mn(a.cursor,s.hand.count-1));
    // Stable x-strips prevent the raised card from stealing adjacent hover targets.
    for(int j=0;j<s.hand.count;j++)c.hit(60+j*step,168,j==s.hand.count-1?47:step,72,
        HitKind::Hand,s.hand.ids[j],!a.busy&&(s.phase==Phase::ExhaustChoice||cost(s.cards[s.hand.ids[j]])<=s.energy));
    if(a.card_fly>0&&a.fly_card>=0){int age=12-a.card_fly;c.picture(a.content.card_image(a.fly_card),90+age*5,153-age*5,26,19,(unsigned)(a.card_fly*18));}
}
static void reward_screen(Canvas&c){
    auto&a=c.a;auto&s=a.game;c.picture(A_Battle,0,20);c.actor(0,63,125);c.dim(0,20,320,120,100);
    c.centered("战斗胜利",0,28,320,20,Gold);c.picture(A_Coin,96,59);char b[100]={};TextBuffer t{b,sizeof b,0};t.ch('+');t.number(s.last_gold);t.text("   燃烧之血 +6");c.text(b,113,58,12,White);
    if(a.cursor<3){a.card_description({s.reward[a.cursor],0},-1,b,sizeof b);c.paragraph(b,12,91,296,2);}
    c.rect(0,137,320,103,Ink);for(int i=0;i<3;i++){small_card(c,{s.reward[i],0},58+i*79,140,a.cursor==i,true,i+1);c.hit(55+i*79,138,53,71,HitKind::Choice,i);}
    c.button("跳过奖励，返回路线",35,211,250,a.cursor==3,26);c.hit(35,211,250,26,HitKind::Choice,3);
}
static void camp_screen(Canvas&c){
    auto&a=c.a;c.picture(A_CampBackground,0,20);c.clip(AnimCampHero,187,72,a.animation*33,false);
    // The campfire texture is original. Its light pulse is a lightweight native effect.
    c.picture(A_CampFire,116,94,55,49,(unsigned)(220+(a.animation%16<8?a.animation%8:15-a.animation%16)*5));
    c.centered("片刻安宁",0,29,320,20,Gold);c.dim(0,157,320,83,170);
    char b[100]={};TextBuffer t{b,sizeof b,0};t.text("休息：回复 ");t.number((c.a.game.max_hp*3)/10);t.text(" 点生命");
    c.button(b,35,164,250,a.cursor==0,28);c.hit(35,164,250,28,HitKind::Choice,0);c.button("锻造：升级一张牌",35,200,250,a.cursor==1,28);c.hit(35,200,250,28,HitKind::Choice,1,has_upgrade(a.game));
}
static void panels(Canvas&c){
    auto&a=c.a;auto&s=a.game;a.hit_count=0;if(a.panel==3){map_screen(c,true);c.close_button();return;}
    c.rect(4,24,312,212,Panel);c.box(4,24,312,212,Gold);
    if(a.panel==1){
        const char*names[]={"牌组","抽牌堆","弃牌堆","消耗堆","能力"};
        for(int tab=0;tab<5;tab++){int x=10+tab*54;
            c.text(names[tab],x+3,28,10,tab==a.pile_tab?Gold:Muted,50);
            if(tab==a.pile_tab)c.rect(x,45,48,1,Gold);c.hit(x,25,52,23,HitKind::PileTab,tab);}
        int ids[MaxCards],n=0;
        if(a.pile_tab==0){n=s.deck_count;for(int i=0;i<n;i++)ids[i]=i;}
        else{const Pile&p=pile(s,a.pile_tab==1?Zone::Draw:a.pile_tab==2?Zone::Discard:a.pile_tab==3?Zone::Exhaust:Zone::Power);n=p.count;for(int i=0;i<n;i++)ids[i]=p.ids[i];
            // Inspecting the draw pile must never disclose its actual shuffle order.
            if(a.pile_tab==1)for(int i=1;i<n;i++){int id=ids[i],j=i;while(j&&s.cards[ids[j-1]].kind>s.cards[id].kind){ids[j]=ids[j-1];--j;}ids[j]=id;}}
        a.inspect_cursor=mn(a.inspect_cursor,mx(0,n-1));a.scroll=mn((a.scroll/10)*10,mx(0,((n-1)/10)*10));
        for(int i=a.scroll;i<mn(n,a.scroll+10);i++){
            Card card=a.pile_tab==0?s.deck[ids[i]]:s.cards[ids[i]];
            int x=24+(i-a.scroll)%5*54,y=52+(i-a.scroll)/5*74;
            bool hovered=a.pointer.visible&&a.pointer.x>=x-2&&a.pointer.x<x+50&&a.pointer.y>=y-2&&a.pointer.y<y+68;
            small_card(c,card,x,y,hovered||(!a.pointer.visible&&i==a.inspect_cursor),true,0);
            c.hit(x-2,y-2,51,70,HitKind::PileCard,card.kind+(card.upgraded<<8));
        }
        if(!n)c.centered("空",0,105,320,12,Muted);
        c.text("‹",14,201,16,Gold);c.hit(7,198,37,27,HitKind::Scroll,-2,a.scroll>0);
        c.text("›",296,201,16,Gold);c.hit(279,198,34,27,HitKind::Scroll,2,a.scroll+10<n);
        char page[64]={};TextBuffer b{page,sizeof page,0};b.number(n);b.text(" 张  ·  ");b.number(a.scroll/10+1);b.text(" / ");b.number(mx(1,(n+9)/10));
        c.centered(page,45,202,230,12,Gold);
        c.centered(a.pile_tab==1?"无序查看 · 点击卡牌看详情":"点击卡牌看详情 · ←→ 切换牌堆",0,221,320,10,Muted);
    }else if(a.panel==2){
        c.centered("本局记录（最近 24 条）",5,27,310,12,Gold);a.scroll=mn(a.scroll,mx(0,s.log_count-10));int first=mx(0,s.log_count-10-a.scroll);
        for(int i=first;i<mn(s.log_count,first+10);i++){char b[160]={};log_line(a,s.logs[i],b,sizeof b);c.text(b,13,49+(i-first)*16,10,White,295);}c.text("↑↓ 滚动 · Esc 返回",15,216,10,Muted);
    }else if(a.panel==5){
        c.centered("遗物",0,30,320,16,Gold);int first=mx(0,a.scroll-5);
        for(int i=first;i<mn(s.relic_count,first+6);i++){
            const auto&d=relic_definitions[s.relic_order[i]];int y=57+(i-first)*20;
            c.hit(10,y,300,20,HitKind::Relic,i);if(i==a.scroll)c.rect(10,y,300,20,rgb(66,53,35));c.picture(d.asset,15,y+1,18,18);
            c.text(d.name,41,y,12,i==a.scroll?Gold:White,225);
            if(d.id==HappyFlower||d.id==PenNib){c.num(relic_counter(s,d.id),276,y,12,Gold);}
        }
        if(s.relic_count){const auto&d=relic_definitions[s.relic_order[a.scroll]];c.paragraph(d.description,15,184,290,2);}
        c.centered("↑↓ 查看 · 计数随存档保留 · R / Esc 返回",0,219,320,10,Muted);
    }else if(a.panel==4){
        c.centered("药水",0,33,320,20,Gold);for(int i=0;i<2;i++){int x=61+i*126;c.hit(x-22,69,98,65,HitKind::Potion,i);if(a.scroll==i)c.box(x-22,69,98,65,Gold);int p=(int)s.potions[i];if(p)c.picture(A_FirePotion+p-1,x+10,77,26,32);c.centered(potion_name(p),x-20,114,94,12);}
        c.text(potion_desc((int)s.potions[a.scroll]),20,146,12,White,280,2);c.text("目标：",20,185,12,Muted);c.num(a.target+1,57,184,12,Gold);c.text("↑↓ 切换敌人",98,185,12,Muted);c.button("使用",115,204,91,true,26);c.hit(115,204,91,26,HitKind::Command,(int)Key::Confirm);
    }
    c.close_button();
    if(a.panel==2||a.panel==5){c.text("↑",299,53,12,Gold);c.hit(294,50,23,24,HitKind::Scroll,-1);c.text("↓",299,157,12,Gold);c.hit(294,153,23,24,HitKind::Scroll,1);}
}
static void detail_screen(Canvas&c){
    auto&a=c.a;Card card{(uint8_t)(a.detail_card&255),(uint8_t)(a.detail_card>>8)};
    a.hit_count=0;c.dim(0,0,320,240,170);c.rect(7,22,306,211,Panel);c.box(7,22,306,211,Gold);
    char title[128]={};TextBuffer name{title,sizeof title,0};name.text(a.content.card_string(card.kind));if(card.upgraded)name.text("+");
    c.centered(title,18,28,270,16,card.upgraded?Green:Gold);
    int type=(int)definitions[card.kind].type;
    c.picture(type==0?A_CardAttack:type==1?A_CardSkill:A_CardPower,18,61,94,132);
    c.picture(c.a.content.card_image(card.kind),24,90,82,60);c.picture(A_Energy,18,60,26,26);
    int energy=cost(card);if(energy<0)c.text("X",27,64,12);else c.num(energy,27,64,12);
    c.centered(title,21,161,87,12,card.upgraded?Green:White);
    char description[512]={};a.card_description(card,a.target,description,sizeof description);
    int lines=c.paragraph(description,123,66,177,0);a.detail_scroll=mn(a.detail_scroll,mx(0,lines-8));
    c.paragraph(description,123,66,177,8,a.detail_scroll);
    c.text(type==0?"攻击":type==1?"技能":"能力",124,48,10,Muted);
    c.button("返回",105,201,111,true,25);c.hit(105,201,111,25,HitKind::DetailClose);
    c.close_button(290,26);c.hit(288,24,25,24,HitKind::DetailClose);
    if(lines>8){c.hit(285,55,25,25,HitKind::Scroll,-1);c.hit(285,174,25,25,HitKind::Scroll,1);}
}
#include "reference_map_ui.inc"
static void pointer_overlay(Canvas&c){
    auto&a=c.a;auto&p=a.pointer;if(!p.visible)return;
    if(p.armed_card>=0&&p.armed_card<a.game.card_count){
        if(a.game.zones[p.armed_card]!=Zone::Hand||a.game.phase!=Phase::Combat)p.armed_card=-1;
        else{c.line(160,180,p.x,p.y,Blue,true);c.box(p.x-5,p.y-5,11,11,Gold);}
    }
    // Small high-contrast cursor, clipped by the same raster bounds as the UI.
    for(int y=0;y<12;y++)for(int x=0;x<=y/2;x++)c.rect(p.x+x,p.y+y,1,1,x==0||x==y/2||y==11?Ink:White);
    c.rect(p.x+4,p.y+10,2,5,Ink);c.rect(p.x+4,p.y+10,1,4,White);
}
void App::render(){
    Canvas c{*this};hit_count=0;c.rect(0,0,320,240,Ink);
    if(title){
        c.picture(A_Title,0,0);c.dim(0,0,320,240,25);c.picture(A_Logo,45,7);c.text("Ndless 原生移植 · 远征原型 0.7",18,78,10,Gold);
        c.actor(0,257,207);const char*items[]={"开始远征","组合练习","继续存档","参考地图","操作说明","退出"};for(int i=0;i<6;i++){c.button(items[i],18,92+i*23,161,menu==i,22);c.hit(18,92+i*23,161,22,HitKind::Menu,i);}
        c.text("v0.111.0 / 41cef1ea",199,224,10,Muted,120);
    }else{
        switch(game.phase){
        case Phase::Map:map_screen(c);break;
        case Phase::Shop:shop_screen(c);break;
        case Phase::Event:event_screen(c);break;
        case Phase::Camp:camp_screen(c);break;
        case Phase::Upgrade:case Phase::Remove:case Phase::Transform:c.picture(A_Title,0,0);deck_select(c);break;
        case Phase::Reward:reward_screen(c);break;
        case Phase::Combat:case Phase::ExhaustChoice:combat_screen(c);break;
        case Phase::Treasure:c.picture(A_ChestBackground,0,20);c.picture(A_Chest,128,91);c.centered("发现宝藏",0,37,320,20,Gold);c.button("打开宝箱",35,198,250,true,28);c.hit(35,198,250,28,HitKind::Choice,0);break;
        default:c.picture(A_Title,0,0);c.dim(0,20,320,220,120);c.centered(game.phase==Phase::Complete?"远征完成":game.phase==Phase::Dead?"倒在了高塔下":"达到安全容量上限",0,78,320,20,Gold);c.centered("Enter 返回标题",0,150,320,12);c.hit(0,130,320,65,HitKind::Command,(int)Key::Confirm);c.centered("原型路线结束，并非完整原版通关。",0,210,320,10,Muted);break;
        }
        if(panel)panels(c);hud(c);
        if(paused){hit_count=0;c.dim(0,0,320,240,160);c.rect(48,22,224,195,Panel);c.box(48,22,224,195,Gold);const char*items[]={"继续","保存","载入","操作说明","返回标题","退出程序"};for(int i=0;i<6;i++){c.button(items[i],65,32+i*29,190,menu==i,25);c.hit(65,32+i*29,190,25,HitKind::Menu,i);}}
    }
    if(help){
        hit_count=0;c.hit(6,7,308,226,HitKind::Close);c.rect(6,7,308,226,Panel);c.box(6,7,308,226,Gold);c.centered("操作说明",6,13,308,16,Gold);
        const char*lines[]={"方向键 / 4、6、8、2：导航","Enter / 5：出牌、选路或确认","空格：结束回合","Tab：牌组与牌堆    L：记录","M：地图  P：药水  R：遗物  I：详情","S：保存    O：载入","Esc：返回或打开菜单","事件中：↑↓ 阅读，←→ 选项","12 层分岔路线，含商店、事件和营地。","触摸板：轻触/按下点击，按住可拖牌。"};
        for(int i=0;i<10;i++)c.text(lines[i],16,41+i*17,12,i<8?White:Muted,291);c.centered("Enter / Esc 返回",0,216,320,10,Gold);
    }
    if(detail_card>=0)detail_screen(c);
    if(map_preview.open)reference_map_screen(c);
    if(notice_ticks&&!map_preview.open){c.rect(2,219,316,20,Panel);c.box(2,219,316,20,Gold);c.centered(notice,3,221,314,10,Gold);}
    pointer_overlay(c);
}
} // namespace spire
