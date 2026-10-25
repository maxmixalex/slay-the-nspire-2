// Original portrait/option textures, reflowed around an unobscured left illustration.
#include "world_events_ui.inc"
static bool event_equal(const char*a,const char*b){while(*a&&*a==*b){++a;++b;}return *a==*b;}
static int event_body_text(const State&s){
    if(s.event_id==EventId::Baths&&s.event_page==1)return s.event_count<=1?46:48+mn(9,s.event_count-1);
    if(s.event_id==EventId::AromaOfChaos&&s.event_page==3&&s.character==Character::Silent)return AromaSilentText;
    int base=EventTextBase+(int)s.event_id*EventTextStride;
    return base+(s.event_page<2?1:s.event_page==2?2:3);
}
static const char* event_title(Canvas&c){return c.a.game.original_events.active?native_event_title(c.a):c.a.content.string(EventTextBase+(int)c.a.game.event_id*EventTextStride);}
static void event_option_text(Canvas&c,int option,char*out,size_t size){
    auto&s=c.a.game;TextBuffer b{out,size,0,&c.a.content};out[0]=0;
    if(s.event_id==EventId::Baths&&s.event_page==1&&option==1){b.text("保留已获得的最大生命");return;}
    const char*p=c.a.content.string(EventTextBase+(int)s.event_id*EventTextStride+5+option*2);
    while(*p){
        if(*p=='{'){
            const char*end=p+1;while(*end&&*end!='}')++end;
            if(*end){int value=0;char token[24]{};unsigned n=0;for(const char*q=p+1;q<end&&n<23;q++)token[n++]=*q;
                if(event_equal(token,"MaxHp"))value=s.event_id==EventId::Baths?2:5;
                else if(event_equal(token,"Damage"))value=3+s.event_count;
                else if(event_equal(token,"Heal"))value=s.event_id==EventId::Baths?10:s.max_hp*3/10;
                else if(event_equal(token,"Gold"))value=s.event_value;
                else if(event_equal(token,"HpLoss"))value=8;
                else if(event_equal(token,"Cards"))value=2;
                b.number(value);p=end+1;continue;
            }
        }
        b.ch(*p++);
    }
}
static void event_reader_screen(Canvas&c){
    auto&a=c.a;a.hit_count=0;c.dim(0,0,320,240,235);
    const bool crystal=a.game.phase==Phase::OriginalEventChoice&&a.game.original_events.selection==OriginalEventSelection::Crystal;
    const char*title=crystal?a.content.world_string("events","CRYSTAL_SPHERE.minigame.instructions.title"):event_title(c);
    c.ui(U_RewardPanel,4,5,312,230);c.centered(title,18,18,278,16,Gold);
    char full[12288]{};const char*text=a.content.string(event_body_text(a.game));
    if(crystal){a.localized_text("events","CRYSTAL_SPHERE.minigame.instructions.description",full,sizeof full);text=full;}
    else if(a.game.original_events.active){
        const bool ancient=ui_ancient_event(a.game.original_events.id);if(ancient)a.sync_ancient_dialogue();
        TextBuffer b{full,sizeof full,0};
        if(ancient&&a.ancient_dialogue.count){
            const auto&d=a.ancient_dialogue;
            for(unsigned i=0;i<d.count&&i<=d.current;++i){char line[8192]{};a.ancient_dialogue_text(i,line,sizeof line);
                if(i)b.text("\n\n");b.text("[gold]");
                b.text(d.character_lines&(1u<<i)?a.content.world_title("characters",character_definitions[unsigned(a.game.character)].key):native_event_title(a));
                b.text("[/gold]\n");b.text(line);}
        }else if(!ancient){a.original_event_text(-1,true,full,sizeof full);while(full[b.n])++b.n;}
        if(!ancient||a.ancient_dialogue.last())for(unsigned i=0;i<original_event_option_count(a.game);i++){char label[256]{},description[4096]{};native_event_option(a,i,false,label,sizeof label);native_event_option(a,i,true,description,sizeof description);b.text("\n\n[gold]");b.text(label);b.text("[/gold]\n");b.text(description);}text=full;
    }
    int lines=c.paragraph(text,20,49,277,0,0,12);
    a.event_reader_scroll=mn(mx(0,a.event_reader_scroll),mx(0,lines-10));
    c.paragraph(text,20,49,277,10,a.event_reader_scroll,12);
    c.text("‹",15,211,16,Gold);c.hit(8,205,34,31,HitKind::Scroll,-10,a.event_reader_scroll>0);
    c.text("›",294,211,16,Gold);c.hit(279,205,34,31,HitKind::Scroll,10,a.event_reader_scroll+10<lines);
    char page[64]{};TextBuffer t{page,sizeof page,0,&c.a.content};t.number(a.event_reader_scroll+1);t.text("–");t.number(mn(lines,a.event_reader_scroll+10));t.text(" / ");t.number(lines);
    c.centered(page,40,211,78,10,Muted);c.button("返回事件",130,207,144,true,26);c.action(124,204,153,32,UiAction::CloseEventReader);
}
static void event_screen(Canvas&c){
    if(c.a.game.original_events.active){native_event_screen(c);return;}
    auto&a=c.a;auto&s=a.game;int base=EventTextBase+(int)s.event_id*EventTextStride;
    c.ui(U_EventFull0+(int)s.event_id,0,22);
    // Gradient is confined to the text column, not an opaque rectangle over the illustration.
    for(int x=125;x<320;x++)c.dim(x,22,1,218,(unsigned)mn(115,(x-125)*4));
    c.centered(event_title(c),134,29,178,16,Gold);
    const char*text=a.content.string(event_body_text(s));int lines=c.paragraph(text,142,59,163,0,0,10);
    a.scroll=mn(mx(0,a.scroll),mx(0,lines-6));c.paragraph(text,142,59,163,6,a.scroll,10);
    // The inline scroll preserves the event context; no duplicate reader button.
    c.text("↑",271,139,10,a.scroll?Gold:Muted);c.hit(262,135,25,21,HitKind::Scroll,-1,a.scroll>0);
    c.text("↓",298,139,10,a.scroll+6<lines?Gold:Muted);c.hit(289,135,26,21,HitKind::Scroll,1,a.scroll+6<lines);
    if(lines>6){c.rect(312,59,1,74,rgb(68,66,61));c.rect(312,59+65*a.scroll/mx(1,lines-6),1,9,Gold);}
    if(s.event_page>=2){
        c.ui(U_RewardSkip,203,205,112,29);c.centered(s.event_id==EventId::Vegetation&&s.event_page==4?"战！":"继续前进 ›",203,209,112,12,Gold);
        c.hit(198,200,120,39,HitKind::Choice,0);return;
    }
    for(int i=0;i<2;i++){
        int y=158+i*40;bool enabled=event_option_enabled(s,i),selected=a.cursor==i;
        c.ui(U_EventButton,136,y,178,39,enabled?255:120);
        if(selected&&enabled)c.ui(U_EventOutline,136,y,178,39,160);
        const char*name=s.event_id==EventId::Baths&&s.event_page==1?(i?"离开浴场":"沉溺"):a.content.string(base+4+i*2);
        c.text(name,147,y+2,12,enabled?Gold:Muted,154);
        char description[256]{};event_option_text(c,i,description,sizeof description);
        if(!enabled){TextBuffer t{description,sizeof description,0,&c.a.content};t.text(s.event_id==EventId::DoorsOfLightAndDark?"至少保留一张牌":"当前无法选择");}
        c.paragraph(description,147,y+17,154,1,0,10,enabled?White:Muted);
        // Full rule text is shown when an option is focused, away from the art's focal point.
        c.hit(136,y,178,39,HitKind::Choice,i,enabled);
    }
    char focused[256]{};event_option_text(c,mn(mx(0,a.cursor),1),focused,sizeof focused);
    c.dim(5,178,125,60,190);c.paragraph(focused,11,183,111,4,0,10,White);
}
