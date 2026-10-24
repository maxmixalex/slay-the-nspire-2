// This entire translation-unit fragment is absent from ordinary builds.
namespace {
constexpr int DebugPresetCount=14;
static bool debug_equal(const char*a,const char*b){
    for(;;++a,++b){char x=*a,y=*b;if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;
        if(x!=y)return false;if(!x)return true;}
}
static bool debug_number(const char*s,int low,int high,int&out){
    if(!s||!*s)return false;unsigned value=0;
    for(;*s;++s){if(*s<'0'||*s>'9'||value>unsigned(high)/10)return false;value=value*10+unsigned(*s-'0');if(value>unsigned(high))return false;}
    if(value<unsigned(low))return false;out=int(value);return true;
}
static void set_debug_reply(App&a,const char*text){a.debug_reply[0]=0;TextBuffer b{a.debug_reply,sizeof a.debug_reply,0};b.text(text);}
static bool debug_failure(App&a,const char*text){set_debug_reply(a,text);return false;}
static void debug_begin(App&a,Character character=Character::Ironclad,uint32_t seed=1){
    new_run(a.game,seed,false,character);world_begin_run(a.game);
    a.title=a.character_select=a.library=a.paused=a.help=a.settings_open=false;
    a.frontend_confirm=0;a.practice_entry=false;a.map_preview.open=false;
    a.forget_run_checkpoint();a.reset_presentation();
}
static void debug_change_variant(App&a,int direction){
    if(a.debug_selection>=4&&a.debug_selection<=6){
        int candidate=a.debug_variant;
        for(unsigned i=0;i<unsigned(EncounterKinds);++i){candidate=(candidate+direction+EncounterKinds)%EncounterKinds;
            if(!candidate){a.debug_variant=0;return;}const auto*d=encounter_definition(EncounterId(candidate));if(!d)continue;
            if((a.debug_selection==4&&(d->type==EncounterClass::Normal||d->type==EncounterClass::Weak))||
               (a.debug_selection==5&&d->type==EncounterClass::Elite)||(a.debug_selection==6&&d->type==EncounterClass::Boss)){
                a.debug_variant=candidate;return;}}
    }else if(a.debug_selection==10){a.debug_variant=(a.debug_variant+direction+int(OriginalEventId::Count))%int(OriginalEventId::Count);}
    else if(a.debug_selection==12){a.debug_variant=(a.debug_variant+direction+CharacterKinds)%CharacterKinds;}
    else if(a.debug_selection==13){a.debug_variant=(a.debug_variant+direction+3)%3;}
}
}
void App::debug_preset(){
    static const char*const commands[]={"kill all","heal","gold 1000","energy 99","room monster","room elite","room boss",
        "room shop","room rest","room treasure","room event","room map","new","act"};
    debug_line[0]=0;TextBuffer out{debug_line,sizeof debug_line,0};out.text(commands[debug_selection]);
    if(debug_selection>=4&&debug_selection<=6&&debug_variant){if(const auto*d=encounter_definition(EncounterId(debug_variant))){out.ch(' ');out.text(d->key);}}
    else if(debug_selection==10&&debug_variant){out.ch(' ');out.text(original_event_key(OriginalEventId(debug_variant)));}
    else if(debug_selection==12){out.ch(' ');out.text(character_definitions[debug_variant].key);}
    else if(debug_selection==13){out.ch(' ');out.number(debug_variant+1);}
}
bool App::debug_command(const char*command){
    // Copy first: callers may pass debug_line itself. No prefix matching or
    // partial execution; malformed/extra arguments fail before mutation.
    char text[128]{};unsigned n=0;
    if(!command)return debug_failure(*this,"Empty command");
    for(;command[n]&&n<sizeof(text)-1;++n){if(uint8_t(command[n])<32||uint8_t(command[n])>126)return debug_failure(*this,"Commands use ASCII model keys");text[n]=command[n];}
    if(command[n])return debug_failure(*this,"Command is too long");
    char*args[5]{};unsigned count=0;char*p=text;
    while(*p){while(*p==' ')++p;if(!*p)break;if(count==5)return debug_failure(*this,"Too many arguments");args[count++]=p;while(*p&&*p!=' ')++p;if(*p)*p++=0;}
    if(!count||debug_equal(args[0],"help")){set_debug_reply(*this,"kill [all|1..8]; room monster|elite|boss [ID]; room event [ID]; room shop|rest|treasure|map; act 1..3; new [CHAR] [SEED]; heal [N]; gold N; energy N; potion 1..64");return count!=0;}
    bool ok=false,reset=false;
    if(debug_equal(args[0],"new")){
        if(count>3)return debug_failure(*this,"new [ironclad|silent|defect|regent|necrobinder] [SEED]");
        Character character=Character::Ironclad;int seed=1;
        if(count>=2){int found=-1;for(int i=0;i<CharacterKinds;++i)if(debug_equal(args[1],character_definitions[i].key))found=i;
            if(found<0)return debug_failure(*this,"Unknown character");character=Character(found);}
        if(count==3&&!debug_number(args[2],1,2147483647,seed))return debug_failure(*this,"Seed must be 1..2147483647");
        debug_begin(*this,character,uint32_t(seed));ok=validate(game);reset=true;
    }else if(debug_equal(args[0],"room")){
        if(count<2||count>3)return debug_failure(*this,"room TYPE [encounter/event ID]");
        WorldRoom room=WorldRoom::None;EncounterId encounter=EncounterId::None;OriginalEventId event=OriginalEventId::None;
        if(debug_equal(args[1],"monster")||debug_equal(args[1],"combat"))room=WorldRoom::Monster;
        else if(debug_equal(args[1],"elite"))room=WorldRoom::Elite;else if(debug_equal(args[1],"boss"))room=WorldRoom::Boss;
        else if(debug_equal(args[1],"shop"))room=WorldRoom::Shop;else if(debug_equal(args[1],"rest")||debug_equal(args[1],"camp"))room=WorldRoom::Rest;
        else if(debug_equal(args[1],"treasure"))room=WorldRoom::Treasure;else if(debug_equal(args[1],"event"))room=WorldRoom::Event;
        else if(debug_equal(args[1],"map"))room=WorldRoom::Map;else return debug_failure(*this,"Unknown room type");
        if(count==3){if(room==WorldRoom::Event){event=original_event_id(args[2]);if(event==OriginalEventId::None)return debug_failure(*this,"Unknown event ID");}
            else if(room==WorldRoom::Monster||room==WorldRoom::Elite||room==WorldRoom::Boss){encounter=encounter_id(args[2]);if(encounter==EncounterId::None)return debug_failure(*this,"Unknown encounter ID");}
            else return debug_failure(*this,"This room has no model argument");}
        if(title||!game.world.active)debug_begin(*this);
        ok=debug_enter_room(game,room,encounter,event);reset=ok;
    }else if(debug_equal(args[0],"act")){
        int act;if(count!=2||!debug_number(args[1],1,3,act))return debug_failure(*this,"act 1..3");
        if(title||!game.world.active)debug_begin(*this);ok=debug_enter_act(game,unsigned(act-1));reset=ok;
    }else{
        if(title)return debug_failure(*this,"Start a run first (new or room)");
        UiCoreObserver observer(*this);
        if(debug_equal(args[0],"kill")){
            int target=-1;if(count>2)return debug_failure(*this,"kill [all|1..8]");
            if(count==2&&!debug_equal(args[1],"all")){if(!debug_number(args[1],1,MaxEnemies,target))return debug_failure(*this,"Invalid target index");--target;}
            ok=debug_kill_enemies(game,target);
        }else if(debug_equal(args[0],"heal")){
            int amount=9999;if(count>2||(count==2&&!debug_number(args[1],0,9999,amount)))return debug_failure(*this,"heal [0..9999]");
            if(game.hp<=0)return debug_failure(*this,"The run has ended");heal(game,amount);++game.revision;ok=true;
        }else if(debug_equal(args[0],"gold")){
            int amount;if(count!=2||!debug_number(args[1],0,9999,amount))return debug_failure(*this,"gold 0..9999");
            gain_gold(game,amount);++game.revision;ok=true;
        }else if(debug_equal(args[0],"energy")){
            int amount;if(count!=2||!debug_number(args[1],0,999,amount))return debug_failure(*this,"energy 0..999");
            if(game.phase!=Phase::Combat)return debug_failure(*this,"Energy requires combat");game.energy=int16_t(amount);++game.revision;ok=true;
        }else if(debug_equal(args[0],"potion")){
            int id;if(count!=2||!debug_number(args[1],1,PotionKinds,id))return debug_failure(*this,"potion 1..64");ok=procure_potion(game,Potion(id));
        }else return debug_failure(*this,"Unknown command. Use help");
    }
    if(!ok)return debug_failure(*this,"Command cannot run in the current state");
    if(reset){title=character_select=library=paused=help=settings_open=false;frontend_confirm=0;map_preview.open=false;reset_presentation();}
    // Debug commands never autosave or replace a user's previous checkpoint.
    forget_run_checkpoint();debug_open=debug_editing=false;pointer_cancel();set_debug_reply(*this,"OK");return true;
}
bool App::debug_input(Key key){
    if(key==Key::Debug){debug_open=!debug_open;debug_editing=false;pointer_cancel();pointer.visible=false;hit_count=0;debug_preset();return true;}
    if(!debug_open)return false;
    if(key==Key::Back){debug_open=debug_editing=false;pointer_cancel();return true;}
    if(key==Key::Up||key==Key::Down){debug_selection=(debug_selection+(key==Key::Up?-1:1)+DebugPresetCount)%DebugPresetCount;debug_variant=0;debug_editing=false;debug_preset();}
    else if(key==Key::Left||key==Key::Right){debug_change_variant(*this,key==Key::Left?-1:1);debug_preset();}
    else if(key==Key::Detail){debug_editing=true;}
    else if(key==Key::Confirm){debug_command(debug_line);}
    return true;
}
void App::debug_text(int code){
    if(!debug_open||!debug_editing)return;
    if(code==27){debug_editing=false;return;}if(code==13){debug_command(debug_line);return;}
    unsigned n=0;while(debug_line[n])++n;
    if(code==8){if(n)debug_line[n-1]=0;}else if(code>=32&&code<=126&&n+1<sizeof(debug_line)){debug_line[n]=char(code);debug_line[n+1]=0;}
}
static void debug_screen(Canvas&c){
    auto&a=c.a;a.hit_count=0;c.rect(0,0,320,240,Panel);
    c.text("DEBUG",12,9,16,Gold);c.text(a.content.english()?"Developer commands":"调试指令",82,12,12,White);c.close_button(291,7);
    const char*const english[]={"Defeat all enemies","Heal","Gain 1000 gold","Set energy to 99","Monster room","Elite room","Boss room","Merchant room","Rest site","Treasure room","Event room","Map","New run","Act"};
    const char*const chinese[]={"击杀全部敌人","回复生命","增加 1000 金币","设为 99 能量","普通敌人房间","精英房间","首领房间","商人房间","休息处","宝箱房间","事件房间","地图","开始新局","幕数"};
    const int first=mn(mx(0,a.debug_selection-2),DebugPresetCount-6);
    for(int i=first;i<first+6;++i){const int y=38+(i-first)*19;const bool selected=i==a.debug_selection;
        if(selected)c.rect(8,y-1,304,18,rgb(70,60,42));c.text((a.content.english()?english:chinese)[i],16,y,12,selected?Gold:White);
        c.hit(8,y-1,304,18,HitKind::DebugItem,i);}
    c.rect(9,155,302,40,Ink);c.box(9,155,302,40,a.debug_editing?Green:Muted);c.paragraph(a.debug_line,15,159,290,3,0,10,White);
    if(*a.debug_reply)c.text_fitted(a.debug_reply,12,199,296,10,Gold,8);
    c.text(a.content.english()?"Arrows: choose  Enter: run  I: type":"方向键选择  Enter 执行  I 编辑",12,216,10,Muted);
    c.hit(9,155,302,40,HitKind::Command,int(Key::Detail));
}
