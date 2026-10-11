#include "app.hpp"
#include "rich_text.hpp"
#include "status.hpp"
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
    void ui(int id,int x,int y,int w=0,int h=0,unsigned alpha=255){picture(a.content.ui_image(id),x,y,w,h,alpha);}
    void action(int x,int y,int w,int h,UiAction act,bool enabled=true){hit(x,y,w,h,HitKind::UiAction,(int)act,enabled);}
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
        const auto&m=a.motion[actor];if(m.clip>=0&&(int32_t)(a.animation-m.start)>=0){int actual=m.clip;if(actor==0&&a.game.character==Character::Silent&&actual>=AnimHeroIdle&&actual<=AnimHeroCast)actual=AnimSilentIdle+actual;auto c=a.content.clip(actual);uint32_t t=(a.animation-m.start)*33;if(t<(uint32_t)c.count*c.frame_ms){id=m.clip;elapsed=t;}}
        if(actor==0&&a.game.character==Character::Silent&&id>=AnimHeroIdle&&id<=AnimHeroCast)id=AnimSilentIdle+(id-AnimHeroIdle);
        clip(id,x,y,elapsed+((id==AnimEnemyIdle)?actor*347:0));
    }
    int width(const char*p,int size){int w=0;while(*p&&*p!='\n')w+=a.content.glyph(utf8(p),size).advance;return w;}
    void glyph(Glyph g,int x,int y,uint16_t color){
        if(g.pixels&&g.encoded_size){
            unsigned position=0;
            for(unsigned at=0;at<g.encoded_size;at+=2){
                unsigned count=g.pixels[at],alpha=g.pixels[at+1];
                if(alpha)for(unsigned j=0;j<count;j++){
                    int xx=x+(int)((position+j)%g.w),yy=y+(int)((position+j)/g.w);
                    if(xx>=0&&xx<320&&yy>=0&&yy<240){auto&v=a.pixels[yy*320+xx];v=blend(v,color,alpha);}
                }
                position+=count;
            }return;
        }
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
    void energy_label(int value,int x,int y,int size=12){
        char text[16]={};TextBuffer t{text,sizeof text,0};if(value<0)t.text("X");else t.number(value);
        for(int i=-1;i<=1;i++)for(int j=-1;j<=1;j++)if(i||j)this->text(text,x+i,y+j,size,Ink);
        this->text(text,x,y,size,White);
    }
    void hp(int value,int total,int x,int y,int w){rect(x,y,w,5,Ink);rect(x,y,w*value/mx(1,total),5,Red);char b[24]={};TextBuffer t{b,sizeof b,0};t.number(value);t.ch('/');t.number(total);centered(b,x,y+3,w,10);}
    void button(const char*p,int x,int y,int w,bool selected,int h=25){
        picture(A_WideButton,x,y,w,h,selected?255:195);
        if(selected){rect(x+8,y+h-2,w-16,1,Gold);text("›",x+5,y+3,12,Gold);}centered(p,x+10,y+(h-18)/2,w-20,12,selected?Gold:White);
    }
};
bool App::init(const uint8_t*p,size_t n,Callbacks cb){callbacks=cb;return content.open(p,n)&&content.current_profile();}
void App::message(const char*s){notice=s;notice_ticks=60;}
void App::animate(int actor,int id,int delay){if(actor<0||actor>2)return;motion[actor]={id,animation+(uint32_t)mx(0,delay)};}
void App::reset_presentation(){detail_owned=-1;relic_flash_ticks=0;relic_flash_mask=0;seen_relic_revision=game.relic_revision;status_actor=-1;status_selected=0;deck_candidate=-1;pointer_cancel();choice_selected_count=0;choice_revision=game.revision;detail_card=detail_item=-1;reward_cards=false;cursor=target=panel=scroll=map_scroll=busy=card_fly=0;for(int i=0;i<3;i++){motion[i].clip=-1;last_hit[i]=hit_start[i]=0;}for(int i=0;i<game.enemy_count;i++)if(game.enemies[i].hp){target=i;break;}}
void App::advance(uint32_t ms){if(ms>250)ms=250;tick_remainder+=ms;while(tick_remainder>=33){tick_remainder-=33;tick();}}
void App::draw_frame(){render();}
void App::tick(){animation++;if(relic_flash_ticks>0)--relic_flash_ticks;if(notice_ticks>0)--notice_ticks;if(feedback>0)--feedback;if(busy>0)--busy;if(card_fly>0)--card_fly;}
void App::card_description(Card c,int t,char*out,size_t cap)const{
    if(!cap)return;out[0]=0;if(c.kind>=CardKinds)return;TextBuffer b{out,cap,0};const char*p=content.card_string(c.kind,c.upgraded?2:1);const auto&d=definitions[c.kind];
    while(*p){
        if(*p=='\x1e'){++p;if(library||!combat_phase(game.phase)){while(*p&&*p!='\x1f')++p;}continue;}
        if(*p=='\x1f'){++p;continue;}
        if(p[0]=='{'&&p[1]&&p[2]=='}'){
            int value=0;switch(p[1]){case 's':value=d.creates+(c.upgraded?d.creates_up:0);break;case 'r':value=(c.kind==BouncingFlask||c.kind==SwordBoomerang)?(c.upgraded?4:3):1;break;case 'h':value=d.hp_loss;break;case 'e':value=d.energy_gain+(c.upgraded?d.energy_up:0);break;case 'd':value=(!library&&combat_phase(game.phase))?damage(game,c,t):d.damage+(c.upgraded?d.damage_up:0);break;case 'b':value=(!library&&combat_phase(game.phase))?card_block(game,c):d.block+(c.upgraded?d.block_up:0);break;case 'n':value=d.draw+(c.upgraded?d.draw_up:0);break;case 'p':value=d.power+(c.upgraded?d.power_up:0);break;default:b.ch(*p++);continue;}
            b.number(value);p+=3;
        }else b.ch(*p++);}
}
static int choice_count(const App&a){
    switch(a.game.phase){case Phase::Map:return 3;case Phase::Shop:return ShopLeave+1;case Phase::Event:return a.game.event_page>=2?1:2;
    case Phase::Reward:return 4;case Phase::Camp:return 2;case Phase::Upgrade:case Phase::Remove:case Phase::Transform:return a.game.deck_count;
    case Phase::Treasure:return a.game.treasure_state?2:1;case Phase::ReturnChoice:return a.game.discard.count;default:return a.game.hand.count;}
}
static void next_target(App&a){for(int i=1;i<=a.game.enemy_count;i++){int t=(a.target+i)%a.game.enemy_count;if(a.game.enemies[t].hp){a.target=t;break;}}}
void App::input(Key k){
    if(!pointer.dispatching){pointer_cancel();pointer.visible=false;}
    if(status_input(k))return;
    if(reference_input(k))return;
    if(detail_item>=0){
        if(k==Key::Back||k==Key::Confirm||k==Key::Detail){detail_item=-1;detail_owned=-1;}
        else if(k==Key::Up)detail_scroll=mx(0,detail_scroll-1);
        else if(k==Key::Down)++detail_scroll;
        return;
    }
    if(detail_card>=0){
        if(k==Key::Back||k==Key::Confirm||k==Key::Detail)detail_card=-1;
        else if(k==Key::Up)detail_scroll=mx(0,detail_scroll-1);
        else if(k==Key::Down)++detail_scroll;
        else if(k==Key::Left||k==Key::Right)detail_card^=256;
        return;
    }
    if(screen_input(k))return;
    if(selection_input(k))return;
    if(deck_input(k))return;
    if(k==Key::Detail&&!title&&!paused&&!help&&!panel){
        if(game.phase==Phase::Reward&&reward_cards&&cursor<3){open_card_detail({game.reward[cursor],0});return;}
        if(game.phase==Phase::Treasure&&game.treasure_state&&game.treasure_relic){
            const auto*d=relic_definition(game.treasure_relic);
            if(d){detail_item=(int)(d-relic_definitions);detail_owned=-1;detail_scroll=0;}return;
        }
        if(game.phase==Phase::Shop){
            detail_owned=-1;detail_scroll=0;
            if(cursor<ShopCardCount)open_card_detail({game.shop_cards[cursor],0});
            else if(cursor<ShopPotionStart){for(int i=0;i<RelicKinds;i++)if(relic_definitions[i].id==game.shop_relics[cursor-ShopRelicStart])detail_item=i;}
            else if(cursor<ShopRemove)detail_item=RelicKinds+game.shop_potions[cursor-ShopPotionStart]-1;
            return;
        }
    }
    if(k==Key::Detail&&!title&&!paused&&!help&&!panel&&combat_phase(game.phase)&&game.hand.count){
        open_card_detail(game.cards[game.hand.ids[mn(cursor,game.hand.count-1)]]);return;
    }

    if(k==Key::Quit){quit=true;return;}
    if(help){if(k==Key::Back||k==Key::Confirm)help=false;return;}
    if(title){
        if(k==Key::Up||k==Key::Left)menu=(menu+6)%7;if(k==Key::Down||k==Key::Right)menu=(menu+1)%7;
        if(k==Key::Confirm){
            if(menu==0){practice_entry=false;character_select=true;}
            else if(menu==1){if(callbacks.load&&callbacks.load(game)){title=false;reset_presentation();message("已恢复存档");}else message("没有有效的存档");}
            else if(menu==2){library=true;library_page=library_selected=0;}
            else if(menu==3){practice_entry=true;character_select=true;}
            else if(menu==4){map_preview.open=true;if(!map_preview.generated)map_preview.generate();}
            else if(menu==5)help=true;else quit=true;
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
        if(reward_cards&&!panel){reward_cards=false;cursor=1;return;}
        if(panel){panel=0;scroll=map_scroll=0;}
        else if(game.phase==Phase::Remove){game.phase=Phase::Shop;cursor=ShopRemove;}
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
        if(panel==5){
            if(k==Key::Left)scroll=mx(0,scroll-1);
            if(k==Key::Right)scroll=mn(mx(0,game.relic_count-1),scroll+1);
            if(k==Key::Up)scroll=mx(0,scroll-6);
            if(k==Key::Down)scroll=mn(mx(0,game.relic_count-1),scroll+6);
            if(k==Key::Confirm||k==Key::Detail)open_owned_relic(scroll);
            return;
        }
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
        else if(game.phase==Phase::Shop){
            const int below[15]={5,6,7,8,9,13,13,10,11,12,14,14,14,0,0};
            const int above[15]={13,13,14,14,14,0,1,2,3,4,7,8,9,5,10};
            cursor=(k==Key::Down?below:above)[cursor];
        }
        else if(game.phase==Phase::Camp)cursor=1-cursor;
        else if(!library&&combat_phase(game.phase))next_target(*this);
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
            if(!acted)message("能量不足，或没有可打出的牌");else{animate(0,card.kind==Shiv?AnimSilentShiv:definitions[card.kind].type==CardType::Attack?AnimHeroAttack:AnimHeroCast);fly_card=card.kind;card_fly=12;busy=12;feedback=8;}break;}
        case Phase::ExhaustChoice:acted=choose_exhaust(game,cursor);break;
        case Phase::Map:if(map_scroll){map_scroll=0;message("已返回可选楼层，再次确认进入");}else{acted=choose_node(game,cursor);if(!acted)message("这条路线不相连，请选择亮起的节点");}break;
        case Phase::Shop:acted=cursor==ShopLeave?leave_shop(game):buy(game,cursor);if(!acted)message("金币不足、已售出，或物品槽已满");else if(game.phase==Phase::Shop)message("购买成功");break;
        case Phase::Remove:acted=remove_card(game,cursor);if(acted)message("已移除所选卡牌");break;
        case Phase::Event:acted=event_choice(game,cursor);scroll=0;break;
        case Phase::Transform:acted=transform_card(game,cursor);if(!acted)message("请选择另一张尚未变化的牌");break;
        case Phase::Treasure:
            if(!game.treasure_state)acted=open_treasure(game);
            else if(cursor==0&&game.treasure_state==1){acted=take_treasure(game);if(acted)cursor=1;}
            else if(cursor==1)acted=leave_treasure(game);
            break;
        case Phase::Reward:acted=choose_reward(game,cursor==3?-1:cursor);if(acted){reward_cards=false;cursor=1;}break;
        case Phase::Camp:acted=camp(game,cursor==1);if(!acted)message("没有可以升级的牌");break;
        case Phase::Upgrade:acted=upgrade_card(game,cursor);if(!acted)message("这张牌已经升级");break;
        default:break;
        }
    }
    if(acted){
        for(int i=0;i<3;i++){int hp=i?game.enemies[i-1].hp:game.hp;int hit=hp_before[i]-hp;if(hit>0){last_hit[i]=hit;hit_start[i]=(int)animation+6;if(i)animate(i,AnimEnemyHurt,6);}}
        if(game.phase!=before){reward_cards=false;cursor=0;scroll=map_scroll=0;if(game.phase==Phase::Map){for(int i=0;i<3;i++)if(reachable(game,game.next_row*3+i)){cursor=i;break;}}}
        cursor=mn(cursor,mx(0,choice_count(*this)-1));if(target>=game.enemy_count||game.enemies[target].hp==0){target=0;for(int i=0;i<game.enemy_count;i++)if(game.enemies[i].hp){target=i;break;}}
    }
}
static void small_card(Canvas&c,Card card,int x,int y,bool selected,bool playable,int number){
    int frame=c.a.content.frame_image(card);
    if(selected){c.rect(x-2,y-2,51,70,rgb(91,74,47));c.box(x-2,y-2,51,70,Gold);}
    c.picture(frame,x,y,47,66);c.picture(c.a.content.card_image(card.kind),x+3,y+15,41,30);c.ui(card_rarity(card.kind)==4?U_RareBanner:card_rarity(card.kind)==3?U_BigBanner:U_CommonBanner,x,y+1,47,11);
    // Card art and text are independent; the description remains legible below the stage.
    c.centered(c.a.content.card_string(card.kind),x+3,y+48,41,10,card.upgraded?Green:White);
    c.picture(c.a.content.energy_image(card),x-1,y-1,15,15);int energy=cost(card);c.energy_label(energy,x+4,y-2,12);
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
    case LogKind::PoisonDamage:t.text("中毒伤害 ");t.number(l.a);break;
    case LogKind::Poison:t.text("给予中毒 ");t.number(l.a);break;
    case LogKind::Discard:t.text("丢弃 ");t.text(a.content.card_string(l.a));break;
    case LogKind::Create:t.text("添加小刀 ");t.number(l.a);break;
    case LogKind::Dexterity:t.text("获得敏捷 ");t.number(l.a);break;
    case LogKind::Noxious:t.text("毒雾 ");t.number(l.a);break;
    case LogKind::Accuracy:t.text("精准 ");t.number(l.a);break;
    default:t.text("容量上限：本局已安全停止");break;}
}
#include "relic_ui.inc"
static void hud(Canvas&c){
    auto&s=c.a.game;c.rect(0,0,320,22,Panel);c.ui(U_TopBar,0,0,320,22);
    c.picture(A_Heart,5,4,13,13);char b[32]={};TextBuffer t{b,sizeof b,0};t.number(s.hp);t.ch('/');t.number(s.max_hp);c.text(b,20,3,10,White);
    c.picture(A_Coin,79,4,12,13);c.num(s.gold,94,3,10,Gold);
    c.ui(U_TopFloor,130,4,11,13);c.num(s.floor,144,3,10,White);
    for(int i=0;i<2;i++){
        int x=163+i*23;
        if(s.potions[i]!=Potion::Empty)c.picture(c.a.content.potion_image(s.potions[i]),x,1,18,20);
        else{c.box(x+4,5,10,12,rgb(92,90,83));c.rect(x+7,2,4,3,Muted);}
        c.hit(x,0,22,22,HitKind::Command,(int)Key::Potions);
    }
    if(s.relic_count){c.picture(c.a.content.relic_image(s.relic_order[0]),208,5,12,12);c.hit(208,0,13,22,HitKind::OwnedRelic,0);}
    c.ui(U_TopMap,224,2,18,18);c.hit(216,0,33,22,HitKind::Command,(int)Key::Map);
    c.ui(U_TopDeck,257,2,18,18);c.hit(249,0,34,22,HitKind::Command,(int)Key::Inspect);
    c.ui(U_TopSettings,291,2,18,18);c.hit(283,0,37,22,HitKind::Command,(int)Key::Back);
    if(!c.a.panel&&(combat_phase(s.phase)||s.phase==Phase::Shop||s.phase==Phase::Camp))owned_relic_strip(c);

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
            if(node.visited)c.picture(s.character==Character::Silent?a.content.ui_image(U_SilentMapMarker):A_MapMarker,xs[col]-12,y-11,23,23);
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
static const char* potion_name(int p){auto*d=potion_definition((Potion)p);return d?d->name:"空药水槽";}
static const char* potion_desc(int p){auto*d=potion_definition((Potion)p);return d?d->description:"药水栏为空。可在商店购买或领取战斗掉落。";}
static void shop_screen(Canvas&c){
    auto&a=c.a;auto&s=a.game;c.picture(A_ShopBackground,0,22,320,218);c.dim(0,22,320,218,35);
    c.ui(U_ShopRug,4,39,312,182);
    c.clip(AnimMerchantTop,258,26,a.animation*33,false);c.text("商店",10,42,16,Gold);
    // Five class cards on the upper shelf, two colorless cards below left.
    for(int i=0;i<ShopCardCount;i++){
        int x=i<5?16+i*56:16+(i-5)*56,y=i<5?65:143;
        bool sold=s.shop_sold&(1u<<i);
        small_card(c,{s.shop_cards[i],0},x,y,a.cursor==i,!sold,0);
        if(sold)c.centered("售罄",x,y+62,47,10,Muted);
        else {c.picture(A_Coin,x+2,y+64,10,10);c.num(s.shop_prices[i],x+15,y+62,10,s.gold>=s.shop_prices[i]?Gold:Red);}
        c.hit(x-2,y-2,51,79,HitKind::Choice,i,!sold);
    }
    for(int i=0;i<3;i++){
        int x=145+i*55,item=ShopRelicStart+i;unsigned id=s.shop_relics[i];int index=-1;
        for(int r=0;r<RelicKinds;r++)if(relic_definitions[r].id==id)index=r;
        bool sold=s.shop_sold&(1u<<item);
        if(a.cursor==item)c.box(x-4,143,43,37,Gold);
        if(index>=0)c.picture(a.content.relic_image(index),x+4,143,25,25,sold?60:255);
        if(sold)c.text("售罄",x+4,168,10,Muted);else c.num(s.shop_prices[item],x+3,166,10,s.gold>=s.shop_prices[item]?Gold:Red);
        c.hit(x-4,142,46,38,HitKind::Choice,item,!sold);
        item=ShopPotionStart+i;sold=s.shop_sold&(1u<<item);
        if(a.cursor==item)c.box(x-4,183,43,36,Gold);
        c.picture(a.content.potion_image((Potion)s.shop_potions[i]),x+6,182,20,24,sold?60:255);
        if(sold)c.text("售罄",x+4,207,10,Muted);else c.num(s.shop_prices[item],x+6,205,10,s.gold>=s.shop_prices[item]?Gold:Red);
        c.hit(x-4,181,46,38,HitKind::Choice,item,!sold);
    }
    c.rect(0,220,320,20,Panel);
    char remove[48]={};TextBuffer r{remove,sizeof remove,0};r.text("移除卡牌  ");r.number(remove_price(s));
    c.text((s.shop_sold&(1u<<ShopRemove))?"已移除卡牌":remove,8,221,10,a.cursor==ShopRemove?Gold:White,149);
    c.hit(0,220,163,20,HitKind::Choice,ShopRemove,!(s.shop_sold&(1u<<ShopRemove)));
    c.text("详情 I",181,221,10,Gold);c.hit(168,220,76,20,HitKind::Command,(int)Key::Detail,a.cursor<ShopRemove);
    c.text("离开 ›",266,221,10,a.cursor==ShopLeave?Gold:White);c.hit(250,220,70,20,HitKind::Choice,ShopLeave);
    // Item tooltip appears over the empty header, never over the target itself.
    const char* name="";int price=0;
    if(a.cursor<ShopCardCount){name=a.content.card_string(s.shop_cards[a.cursor]);price=s.shop_prices[a.cursor];}
    else if(a.cursor<ShopPotionStart){name=relic_name(s.shop_relics[a.cursor-ShopRelicStart]);price=s.shop_prices[a.cursor];}
    else if(a.cursor<ShopRemove){auto*d=potion_definition((Potion)s.shop_potions[a.cursor-ShopPotionStart]);name=d?d->name:"";price=s.shop_prices[a.cursor];}
    else name=a.cursor==ShopRemove?"永久移除一张牌":"继续前进";
    char info[100]={};TextBuffer t{info,sizeof info,0};t.text(name);if(price){t.text("  ");t.number(price);t.text(" 金币");}
    c.dim(68,41,226,20,200);c.centered(info,68,42,226,10,Gold);
    if(a.pointer.visible&&!a.pointer.held&&a.cursor>=ShopRelicStart&&a.cursor<ShopRemove){
        const char*desc=a.cursor<ShopPotionStart?relic_desc(s.shop_relics[a.cursor-ShopRelicStart]):potion_desc(s.shop_potions[a.cursor-ShopPotionStart]);
        c.rect(12,65,296,59,Panel);c.box(12,65,296,59,Gold);c.text(name,21,68,12,Gold,280);c.paragraph(desc,21,87,278,2,0,10);
    }
}

static void deck_select(Canvas&c){
    auto&a=c.a;auto&s=a.game;c.dim(0,20,320,220,225);
    const char* title=s.phase==Phase::Upgrade?"选择一张牌升级":s.phase==Phase::Remove?"移除一张牌":"选择一张牌变化";
    c.centered(title,0,25,320,16,Gold);
    a.cursor=mn(mx(0,a.cursor),s.deck_count-1);int first=a.cursor/10*10;
    for(int i=first;i<mn(s.deck_count,first+10);i++){
        Card card=s.deck[i];int x=24+(i-first)%5*55,y=54+(i-first)/5*73;
        bool allowed=s.phase==Phase::Upgrade?!card.upgraded:s.phase==Phase::Transform?i!=s.event_first:true;
        small_card(c,card,x,y,a.cursor==i,allowed,0);
        c.hit(x-3,y-2,53,71,HitKind::Choice,i,allowed);
    }
    if(s.phase==Phase::Transform){char text[64]={};TextBuffer b{text,sizeof text,0};b.text("还需变化 ");b.number(s.event_remaining);b.text(" 张 · 确认前不会扣除卡牌");c.centered(text,0,196,320,10,Muted);}
    else c.centered("方向键选牌 · I 详情 · Enter 预览并确认",0,196,320,10,Muted);
    int page=first/10,last=(s.deck_count-1)/10;
    c.text("‹",12,214,16,Gold);c.action(0,210,43,30,UiAction::PreviousDeckPage,page>0);
    c.text("›",296,214,16,Gold);c.action(277,210,43,30,UiAction::NextDeckPage,page<last);
    char label[24]={};TextBuffer b{label,sizeof label,0};b.number(page+1);b.text(" / ");b.number(last+1);c.centered(label,112,218,96,10,Muted);
}
#include "status_ui.inc"
static void combat_screen(Canvas&c){
    auto&a=c.a;auto&s=a.game;c.ui(U_BattleBackground,0,22);c.hit(0,42,320,118,HitKind::Battlefield);c.actor(0,60,133);
    if(s.block){c.picture(A_Block,7,120);c.num(s.block,23,119,12,Blue);}
    status_strip(c,0,6,47,126);
    for(int i=0;i<s.enemy_count;i++){
        const auto&e=s.enemies[i];int x=s.enemy_count==1?225:174+i*93;
        if(!e.hp){c.centered("已击败",x-34,104,68,10,Muted);continue;}
        c.hit(x-40,38,80,101,HitKind::Enemy,i);c.actor(i+1,x,119);c.picture(e.move==2?A_BuffIntent:A_AttackIntent,x-24,40);
        if(e.move==2)c.text("+2",x-3,39,12,Green);else c.num(enemy_damage(e),x-3,39,12,Gold);
        if(e.move==1){c.picture(A_Block,x+21,42,10,10);c.num(5,x+32,40,10,Blue);}
        status_strip(c,i+1,x-37,91,77);
        if(a.target==i)c.text("▼",x-5,60,10,Gold);c.hp(e.hp,e.max_hp,x-32,122,64);
        if(e.block){c.picture(A_Block,x+15,73,12,12);c.num(e.block,x+27,73,10,Blue);}
    }
    for(int i=0;i<3;i++){
        int age=(int)a.animation-a.hit_start[i];if(a.last_hit[i]&&age>=0&&age<24){int x=i==0?48:s.enemy_count==1?212:160+(i-1)*93;c.num(a.last_hit[i],x,83-age/2,16,i?Gold:Red);}
    }
    c.dim(3,140,314,30,210);
    if(s.hand.count){Card selected=s.cards[s.hand.ids[mn(a.cursor,s.hand.count-1)]];char b[320]={};a.card_description(selected,definitions[selected.kind].targeted?a.target:-1,b,sizeof b);c.paragraph(b,7,142,308,2);}
    else c.text("没有手牌，按空格结束回合",7,142,12,Muted);
    if(s.phase==Phase::ExhaustChoice){c.rect(0,21,320,18,Red);c.centered("选择一张手牌消耗 · Enter 确认",0,21,320,12);}
    c.picture(s.character==Character::Silent?a.content.ui_image(U_SilentOrb):A_Energy,5,173,42,42);c.num(s.energy,17,177,20,White);c.picture(A_DrawPile,5,205,14,14);c.num(s.draw.count,21,202,10,Gold);
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
static void large_card(Canvas&c,Card card,int x,int y,bool selected){
    int type=(int)definitions[card.kind].type;
    if(selected){c.box(x-2,y-2,84,116,Gold);c.box(x-1,y-1,82,114,Gold);}
    c.picture(c.a.content.frame_image(card),x,y,80,112);
    c.picture(c.a.content.card_image(card.kind),x+5,y+20,70,51);
    c.ui(card_rarity(card.kind)==4?U_RareBanner:card_rarity(card.kind)==3?U_BigBanner:U_CommonBanner,x,y+1,80,17);
    const char*name=c.a.content.card_string(card.kind);
    for(int dx=-1;dx<=1;dx++)for(int dy=-1;dy<=1;dy++)if(dx||dy)c.centered(name,x+5+dx,y+1+dy,70,10,Ink);
    c.centered(name,x+5,y+1,70,10,card.upgraded?Green:White);
    c.picture(c.a.content.energy_image(card),x-3,y-5,19,19);
    int energy=cost(card);c.energy_label(energy,x+3,y-4,12);
    char text[320]={};c.a.card_description(card,-1,text,sizeof text);c.dim(x+5,y+73,70,29,135);c.paragraph(text,x+7,y+74,66,2,0,10,White);
    c.centered(type==0?"攻击":type==1?"技能":"能力",x+4,y+99,72,10,Muted);
}
static void reward_screen(Canvas&c){
    auto&a=c.a;auto&s=a.game;c.ui(U_BattleBackground,0,22);c.actor(0,54,130);c.dim(0,22,320,218,155);
    if(a.reward_cards){
        c.centered("选择一张牌",0,36,320,20,Gold);
        for(int i=0;i<3;i++){int x=24+i*96;large_card(c,{s.reward[i],0},x,77,a.cursor==i);c.hit(x-3,72,86,121,HitKind::Choice,i);}
        c.button("详情 I",14,209,91,false,25);c.hit(10,205,99,31,HitKind::Command,(int)Key::Detail,a.cursor<3);
        c.ui(U_RewardSkip,211,209,92,25);c.centered("跳过",211,211,92,12,a.cursor==3?Gold:White);c.hit(207,205,104,31,HitKind::Choice,3);
        return;
    }
    c.ui(U_RewardPanel,50,34,220,192);c.ui(U_RewardBanner,38,32,244,54);c.centered("战利品",60,47,200,20,Gold);
    for(int i=0;i<3;i++){
        int y=98+i*33;bool claimed=s.reward_claimed&(1<<i);
        bool available=i!=2||s.reward_potion!=Potion::Empty;
        c.ui(U_RewardButton,70,y,180,29,(claimed||!available)?110:255);
        if(a.cursor==i&&!claimed&&available)c.box(68,y-1,184,31,Gold);
        if(i==0)c.ui(U_RewardGold,76,y+3,23,23);
        if(i==1)c.ui(U_RewardCard,76,y+3,23,23);
        if(i==2&&available)c.picture(a.content.potion_image(s.reward_potion),78,y+2,20,25);
        char text[96]={};TextBuffer t{text,sizeof text,0};
        if(claimed)t.text("已领取");
        else if(i==0){t.number(s.last_gold);t.text(" 金币");}
        else if(i==1)t.text("选择一张牌");
        else if(available)t.text(potion_definition(s.reward_potion)->name);
        else t.text("没有药水掉落");
        c.text(text,107,y+6,12,claimed?Muted:White,137);
        const UiAction actions[]={UiAction::RewardGold,UiAction::RewardCards,UiAction::RewardPotion};
        c.action(68,y-1,184,31,actions[i],!claimed&&available);
    }
    c.ui(U_RewardSkip,213,211,99,25);c.centered("前进 ›",213,214,99,12,a.cursor==3?Gold:White);c.action(209,208,107,30,UiAction::LeaveRewards);
}

static void camp_screen(Canvas&c){
    auto&a=c.a;c.picture(A_CampBackground,0,22,320,218);c.clip(a.game.character==Character::Silent?AnimSilentCamp:AnimCampHero,188,61,a.animation*33,false);
    c.picture(a.content.ui_image(U_CampFlames)+(int)((a.animation*33/125)%160),130,76,46,58);
    c.picture(A_CampFire,128,111,55,34,(unsigned)(225+(a.animation%16<8?a.animation%8:15-a.animation%16)*4));
    c.centered("休息处",0,43,320,20,Gold);
    const int xs[2]={54,189};
    for(int i=0;i<2;i++){
        bool enabled=!i||has_upgrade(a.game);int x=xs[i];
        if(a.cursor==i)c.ui(U_RestOutline,x-4,137,88,62);
        c.ui(i?U_RestSmith:U_RestHeal,x,140,80,57,enabled?255:100);
        c.centered(i?"锻造":"休息",x-4,193,88,16,a.cursor==i?Gold:White);
        c.hit(x-9,136,98,80,HitKind::Choice,i,enabled);
    }
    c.dim(0,219,320,21,200);char info[128]={};TextBuffer t{info,sizeof info,0};
    if(a.cursor==0){t.text("回复 ");t.number(rest_heal_amount(a.game));t.text((a.game.relics&RegalPillow)?" 点生命（包含皇家枕头）":" 点生命（最大生命的 30%）");}
    else t.text("升级牌组中的一张牌。");
    c.centered(info,0,221,320,10,White);
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
        owned_relic_panel(c);
    }else if(a.panel==4){
        c.centered("药水",0,30,320,16,Gold);
        for(int i=0;i<2;i++){
            int x=43+i*133;c.hit(x,59,103,57,HitKind::Potion,i);if(a.scroll==i)c.box(x,59,103,57,Gold);
            int p=(int)s.potions[i];if(p)c.picture(a.content.potion_image((Potion)p),x+38,63,25,29);
            c.centered(potion_name(p),x+2,96,99,10);
        }
        c.paragraph(potion_desc((int)s.potions[a.scroll]),19,126,282,3,0,12);
        const auto*d=potion_definition(s.potions[a.scroll]);
        if(d&&d->targeted&&s.phase==Phase::Combat){
            for(int i=0;i<s.enemy_count;i++){int x=19+i*147;char label[24]={};TextBuffer t{label,sizeof label,0};t.text("敌人 ");t.number(i+1);
                c.button(label,x,176,133,a.target==i,23);c.action(x,174,133,26,i?UiAction::TargetSecond:UiAction::TargetFirst,s.enemies[i].hp>0);}
        }
        c.button("丢弃",28,205,113,false,25);c.action(25,204,119,28,UiAction::DiscardPotion,d!=nullptr);
        bool usable=can_use_potion(s,a.scroll,a.target);c.button(usable?"使用":"无法使用",176,205,113,usable,25);
        c.hit(174,204,117,28,HitKind::Command,(int)Key::Confirm,usable);
    }
    c.close_button();
    if(a.panel==2){c.text("↑",299,53,12,Gold);c.hit(294,50,23,24,HitKind::Scroll,-1);c.text("↓",299,157,12,Gold);c.hit(294,153,23,24,HitKind::Scroll,1);}
}
static void detail_screen(Canvas&c){
    auto&a=c.a;Card card{(uint8_t)(a.detail_card&255),(uint8_t)(a.detail_card>>8)};
    a.hit_count=0;c.dim(0,0,320,240,170);c.rect(7,22,306,211,Panel);c.box(7,22,306,211,Gold);
    char title[128]={};TextBuffer name{title,sizeof title,0};name.text(a.content.card_string(card.kind));if(card.upgraded)name.text("+");
    c.centered(title,18,28,270,16,card.upgraded?Green:Gold);
    int type=(int)definitions[card.kind].type;
    c.picture(a.content.frame_image(card),18,61,94,132);
    c.picture(c.a.content.card_image(card.kind),24,90,82,60);c.picture(a.content.energy_image(card),18,60,26,26);
    int energy=cost(card);if(energy<0)c.text("X",27,64,12);else c.num(energy,27,64,12);
    c.centered(title,21,161,87,12,card.upgraded?Green:White);
    char description[512]={};a.card_description(card,a.target,description,sizeof description);
    int lines=c.paragraph(description,123,66,177,0);a.detail_scroll=mn(a.detail_scroll,mx(0,lines-8));
    c.paragraph(description,123,66,177,8,a.detail_scroll);
    c.text(type==0?"攻击":type==1?"技能":"能力",124,48,10,Muted);
    c.button("返回",21,204,98,false,25);c.hit(20,202,101,28,HitKind::DetailClose);
    c.button(card.upgraded?"查看基础":"预览升级",166,204,132,true,25);c.action(163,202,139,28,UiAction::ToggleCardUpgrade);
    c.close_button(290,26);c.hit(288,24,25,24,HitKind::DetailClose);
    if(lines>8){c.hit(285,55,25,25,HitKind::Scroll,-1);c.hit(285,174,25,25,HitKind::Scroll,1);}
}
#include "reference_map_ui.inc"
#include "ui_frontend.inc"
#include "card_choice_ui.inc"
#include "deck_confirm_ui.inc"
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
    Canvas c{*this};sync_relic_feedback();hit_count=0;c.rect(0,0,320,240,Ink);
    // Opaque screens do not redraw the hidden title/combat scene or its clips.
    if(map_preview.open){reference_map_screen(c);pointer_overlay(c);return;}
    if(library){library_screen(c);if(detail_card>=0)detail_screen(c);if(detail_item>=0)item_detail_screen(c);pointer_overlay(c);return;}
    if(character_select){character_screen(c);pointer_overlay(c);return;}
    if(title){title_screen(c);
    }else{
        switch(game.phase){
        case Phase::Map:map_screen(c);break;
        case Phase::Shop:shop_screen(c);break;
        case Phase::Event:event_screen(c);break;
        case Phase::Camp:camp_screen(c);break;
        case Phase::Upgrade:case Phase::Remove:case Phase::Transform:c.picture(A_Title,0,0);deck_select(c);break;
        case Phase::Reward:reward_screen(c);break;
        case Phase::Combat:case Phase::ExhaustChoice:case Phase::DiscardChoice:case Phase::ExhaustManyChoice:case Phase::ReturnChoice:combat_screen(c);break;
        case Phase::Treasure:treasure_screen(c);break;
        default:c.picture(A_Title,0,0);c.dim(0,20,320,220,120);c.centered(game.phase==Phase::Complete?"远征完成":game.phase==Phase::Dead?"倒在了高塔下":"达到安全容量上限",0,78,320,20,Gold);c.centered("Enter 返回标题",0,150,320,12);c.hit(0,130,320,65,HitKind::Command,(int)Key::Confirm);c.centered("原型路线结束，并非完整原版通关。",0,210,320,10,Muted);break;
        }
        if(panel)panels(c);hud(c);
        if(!panel&&!library&&choice_phase(game.phase))card_choice_screen(c);
        if(paused){hit_count=0;c.dim(0,0,320,240,185);c.centered("暂停",0,22,320,20,Gold);
            const char*items[]={"继续游戏","保存游戏","读取存档","操作说明","返回主菜单","退出游戏"};
            for(int i=0;i<6;i++){int y=60+i*28;c.ui(U_RewardButton,63,y,194,25);if(menu==i)c.box(62,y-1,196,27,Gold);
                c.centered(items[i],65,y+3,190,12,menu==i?Gold:White);c.hit(60,y-1,200,27,HitKind::Menu,i);}}

    }
    if(help){
        hit_count=0;c.hit(6,7,308,226,HitKind::Close);c.rect(6,7,308,226,Panel);c.box(6,7,308,226,Gold);c.centered("操作说明",6,13,308,16,Gold);
        const char*lines[]={"方向键 / 4、6、8、2：导航","Enter / 5：出牌、选路或确认","空格：结束回合","Tab：牌组与牌堆    L：记录","M：地图  P：药水  R：遗物  I：详情","S：保存  O：载入  V：能力状态","Esc：返回或打开菜单","事件中：↑↓ 阅读，←→ 选项","12 层分岔路线，含商店、事件和营地。","触摸板：轻触/按下点击，按住可拖牌。"};
        for(int i=0;i<10;i++)c.text(lines[i],16,41+i*17,12,i<8?White:Muted,291);c.centered("Enter / Esc 返回",0,216,320,10,Gold);
    }
    if(character_select)character_screen(c);
    if(library)library_screen(c);
    if(deck_candidate>=0&&!paused&&!help&&!panel)deck_confirm_screen(c);
    if(detail_card>=0)detail_screen(c);
    if(detail_item>=0)item_detail_screen(c);
    if(status_actor>=0)status_screen(c);
    if(map_preview.open)reference_map_screen(c);
    if(notice_ticks&&detail_item<0&&detail_card<0&&!panel&&game.phase!=Phase::Treasure&&status_actor<0&&!map_preview.open&&!library&&!character_select&&deck_candidate<0&&!choice_phase(game.phase)){c.rect(2,219,316,20,Panel);c.box(2,219,316,20,Gold);c.centered(notice,3,221,314,10,Gold);}
    owned_relic_tooltip(c);
    pointer_overlay(c);
}
} // namespace spire
