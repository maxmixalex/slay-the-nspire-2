// NDevConsole: command line, output, history and argument completion. The LCD
// keeps bounded buffers; model effects use the same transactions as gameplay.
namespace {
struct DebugCommandInfo{const char*name;const char*usage;};
static const DebugCommandInfo DebugCommands[]={
 {"help","help [command]"},{"card","card ID [Hand|Draw|Discard|Exhaust|Deck]"},
 {"remove_card","remove_card ID [Hand|Deck]"},{"upgrade","upgrade [hand-index=0]"},
 {"relic","relic [add|remove] ID"},{"potion","potion ID"},
 {"fight","fight encounter-ID"},{"event","event event-ID"},{"ancient","ancient ancient-ID [offering-ID]"},
 {"room","room Monster|Elite|Boss|Shop|RestSite|Treasure|Event|Map [ID]"},
 {"act","act 1..3|OVERGROWTH|UNDERDOCKS|HIVE|GLORY"},
 {"win","win"},{"kill","kill [enemy-index=0|all]"},{"die","die"},
 {"heal","heal amount [ally-index=0; 1=Osty]"},{"gold","gold signed-amount"},
 {"energy","energy amount (adds; 0..999)"},{"stars","stars amount (adds; 0..9999)"},
 {"draw","draw [count=1]"},{"block","block amount [actor-index=0]"},{"damage","damage amount [actor-index; default=all enemies]"},
 {"power","power ID amount actor-index"},{"enchant","enchant ID [amount=1] [hand-index=0]"},
 {"afflict","afflict ID [amount=1] [hand-index=0]"},
 {"godmode","godmode (toggle Strength/Regen/Buffer)"},{"travel","travel (toggle free map travel)"},
 {"bestiary","bestiary"},{"instant","instant (enable Fast Mode)"},
 {"unlock","unlock cards|relics|potions|monsters|events|acts|ascensions|all [IDs...]"},
 {"dump","dump [cards|relics|potions|powers|events|fights|acts] [page=1]"},
 {"clear","clear (console output)"},{"new","new [character] [seed] (Nspire shortcut)"},
 {"achievement","achievement: platform service unavailable"},{"cloud","cloud: network service unavailable"},
 {"getlogs","getlogs: desktop log export unavailable"},{"leaderboard","leaderboard: network service unavailable"},
 {"log","log: desktop logger unavailable"},{"multiplayer","multiplayer: unavailable in the solo port"},
 {"open","open: desktop file browser unavailable"},{"sentry","sentry: telemetry service unavailable"},
 {"art","art: desktop asset editor unavailable"},{"trailer","trailer: desktop capture mode unavailable"}
};
constexpr int DebugCommandCount=int(sizeof DebugCommands/sizeof DebugCommands[0]);
static unsigned debug_length(const char*s){unsigned n=0;while(s&&s[n])++n;return n;}
static char debug_upper(char c){return c>='a'&&c<='z'?char(c-32):c;}
static bool debug_equal(const char*a,const char*b){if(!a||!b)return false;for(;;++a,++b){if(debug_upper(*a)!=debug_upper(*b))return false;if(!*a)return true;}}
static bool debug_prefix(const char*text,const char*prefix){if(!text||!prefix)return false;for(;*prefix;++text,++prefix)if(debug_upper(*text)!=debug_upper(*prefix))return false;return true;}
static bool debug_contains(const char*text,const char*part){if(!text||!part)return false;for(;*text;++text)if(debug_prefix(text,part))return true;return !*part;}
static void debug_copy(char*out,unsigned size,const char*text){if(!size)return;unsigned n=0;while(text&&*text&&n+1<size)out[n++]=*text++;out[n]=0;}
static bool debug_number(const char*s,int low,int high,int&out){
    if(!s||!*s)return false;bool negative=false;if(*s=='-'||*s=='+'){negative=*s=='-';++s;}if(!*s)return false;
    unsigned value=0;for(;*s;++s){if(*s<'0'||*s>'9'||value>214748364u)return false;value=value*10+unsigned(*s-'0');if(value>2147483647u)return false;}
    const int result=negative?-int(value):int(value);if(result<low||result>high)return false;out=result;return true;
}
static void debug_output_line(App&a,const char*text,unsigned n,unsigned kind){
    if(a.debug_output_count==24){for(unsigned i=1;i<24;++i){debug_copy(a.debug_output[i-1],64,a.debug_output[i]);a.debug_output_kind[i-1]=a.debug_output_kind[i];}--a.debug_output_count;}
    char*out=a.debug_output[a.debug_output_count];n=mn(int(n),63);for(unsigned i=0;i<n;++i)out[i]=text[i];out[n]=0;a.debug_output_kind[a.debug_output_count++]=uint8_t(kind);
}
static void debug_print(App&a,const char*text,unsigned kind=0){
    // Console output and model IDs use ASCII in the original. Wrap at word
    // boundaries rather than silently replacing useful command text by '...'.
    a.debug_output_scroll=0;
    while(text&&*text){unsigned n=0,last_space=0;while(text[n]&&text[n]!='\n'&&n<50){if(text[n]==' ')last_space=n;++n;}
        if(n==50&&text[n]&&text[n]!='\n'&&last_space)n=last_space;
        debug_output_line(a,text,n,kind);text+=n;while(*text==' ')++text;if(*text=='\n')++text;
    }
}
static void set_debug_reply(App&a,const char*text,unsigned kind=0){debug_copy(a.debug_reply,sizeof a.debug_reply,text);debug_print(a,text,kind);}
static bool debug_failure(App&a,const char*text){set_debug_reply(a,text,2);return false;}
static void debug_begin(App&a,Character character=Character::Ironclad,uint32_t seed=1){
    new_run(a.game,seed,false,character);world_begin_run(a.game);
    a.title=a.character_select=a.library=a.paused=a.help=a.settings_open=false;
    a.compendium_open=a.statistics_open=a.history_open=a.bestiary.open=a.profiles_open=a.profile_pending=false;
    a.frontend_confirm=0;a.practice_entry=false;a.map_preview.open=false;a.forget_run_checkpoint();a.reset_presentation();
}
static bool debug_ancient_id(OriginalEventId id){switch(id){case OriginalEventId::Neow:case OriginalEventId::Darv:case OriginalEventId::Orobas:case OriginalEventId::Pael:case OriginalEventId::Tezcatara:case OriginalEventId::Tanx:case OriginalEventId::Nonupeipe:case OriginalEventId::Vakuu:return true;default:return false;}}
static int debug_card_id(const char*key){for(int i=0;i<CardKinds;++i)if(debug_equal(key,definitions[i].key))return i;return -1;}
static Potion debug_potion_id(const char*key){for(unsigned i=0;i<PotionKinds;++i)if(debug_equal(key,potion_definitions[i].key))return Potion(i+1);return Potion::Empty;}
static RelicId debug_relic_id(const char*key){
    for(unsigned i=0;i<WorldRelicKinds;++i)if(debug_equal(key,world_relic_key(RelicId(i))))return RelicId(i);
    for(unsigned i=0;i<WorldRelicKinds;++i)if(debug_prefix(world_relic_key(RelicId(i)),key))return RelicId(i);
    for(unsigned i=0;i<WorldRelicKinds;++i)if(debug_contains(world_relic_key(RelicId(i)),key))return RelicId(i);return NoWorldRelic;
}
static OriginalEventId debug_event_id(const char*key){for(unsigned i=1;i<unsigned(OriginalEventId::Count);++i)if(debug_equal(key,original_event_key(OriginalEventId(i))))return OriginalEventId(i);return OriginalEventId::None;}
static EncounterId debug_fight_id(const char*key){for(unsigned i=1;i<EncounterKinds;++i){const auto*d=encounter_definition(EncounterId(i));if(d&&debug_equal(key,d->key))return EncounterId(i);}return EncounterId::None;}
static bool debug_parse_pile(const char*key,DebugPile&out){
    static const char*const names[]={"Hand","Draw","Discard","Exhaust","Deck"};for(unsigned i=0;i<5;++i)if(debug_equal(key,names[i])){out=DebugPile(i);return true;}return false;
}
static const char*debug_affliction_key(unsigned i){static const char*const keys[]={"","BOUND","ENTANGLED","GALVANIZED","HEXED","TAINTED","RINGING","SMOG"};return i<unsigned(EnemyAffliction::Count)?keys[i]:nullptr;}
static const char*debug_act_key(unsigned i){static const char*const keys[]={"OVERGROWTH","UNDERDOCKS","HIVE","GLORY"};return i<4?keys[i]:nullptr;}
static void debug_remember(App&a,const char*command){
    if(!*command)return;
    if(!a.debug_history_count||!debug_equal(a.debug_history[0],command)){
        const unsigned last=mn(7,a.debug_history_count);for(unsigned i=last;i>0;--i)debug_copy(a.debug_history[i],128,a.debug_history[i-1]);
        debug_copy(a.debug_history[0],128,command);if(a.debug_history_count<8)++a.debug_history_count;
    }
    a.debug_history_index=-1;a.debug_draft[0]=0;
}
struct DebugCompletionContext{char command[32]{},first[64]{},prefix[96]{};unsigned argument=0,start=0;};
static DebugCompletionContext debug_completion_context(const char*line){
    DebugCompletionContext out;unsigned p=0,token=0;while(line[p]){
        while(line[p]==' ')++p;if(!line[p])break;unsigned start=p;while(line[p]&&line[p]!=' ')++p;const unsigned n=p-start;
        if(!token){for(unsigned i=0;i<n&&i<sizeof(out.command)-1;++i)out.command[i]=line[start+i];}
        if(token==1){for(unsigned i=0;i<n&&i<sizeof(out.first)-1;++i)out.first[i]=line[start+i];}
        out.start=start;out.argument=token;debug_copy(out.prefix,sizeof out.prefix,line+start);out.prefix[mn(int(n),int(sizeof out.prefix)-1)]=0;++token;
    }
    if(p&&line[p-1]==' '){out.start=p;out.argument=token;out.prefix[0]=0;}return out;
}
static const char* debug_completion_candidate(const DebugCompletionContext&context,unsigned ordinal,unsigned*total=nullptr){
    unsigned seen=0;struct CountResult{unsigned&seen;unsigned*out;~CountResult(){if(out)*out=seen;}}count_result{seen,total};
    auto consider=[&](const char*key)->const char*{if(key&&*key&&debug_prefix(key,context.prefix)){if(seen++==ordinal)return key;}return nullptr;};
    if(context.argument==0||debug_equal(context.command,"help")){for(const auto&command:DebugCommands)if(const char*key=consider(command.name))return key;return nullptr;}
    const char*cmd=context.command;
    if(debug_equal(cmd,"card")||debug_equal(cmd,"remove_card")){
        if(context.argument==1){for(const auto&card:definitions)if(const char*key=consider(card.key))return key;}
        else if(context.argument==2){const char*const names[]={"Hand","Draw","Discard","Exhaust","Deck"};for(const char*name:names)if((!debug_equal(cmd,"remove_card")||debug_equal(name,"Hand")||debug_equal(name,"Deck")))if(const char*key=consider(name))return key;}
    }else if(debug_equal(cmd,"relic")&&(context.argument==1||(context.argument==2&&(debug_equal(context.first,"add")||debug_equal(context.first,"remove"))))){
        if(context.argument==1){if(const char*key=consider("add"))return key;if(const char*key=consider("remove"))return key;}
        for(unsigned i=0;i<WorldRelicKinds;++i)if(const char*key=consider(world_relic_key(RelicId(i))))return key;
    }else if(debug_equal(cmd,"potion")&&context.argument==1){for(const auto&p:potion_definitions)if(const char*key=consider(p.key))return key;
    }else if(debug_equal(cmd,"power")&&context.argument==1){for(unsigned i=0;i<StatusKinds;++i){const char*key=debug_power_key(i);if(!key)continue;bool duplicate=false;for(unsigned j=0;j<i;++j)if(debug_equal(key,debug_power_key(j))){duplicate=true;break;}if(!duplicate)if(const char*found=consider(key))return found;}
    }else if(debug_equal(cmd,"enchant")&&context.argument==1){for(unsigned i=1;i<EventEnchantmentKinds;++i)if(const char*key=consider(event_enchantment_key(EventEnchantment(i))))return key;
    }else if(debug_equal(cmd,"afflict")&&context.argument==1){for(unsigned i=1;i<unsigned(EnemyAffliction::Count);++i)if(const char*key=consider(debug_affliction_key(i)))return key;
    }else if((debug_equal(cmd,"event")||debug_equal(cmd,"ancient"))&&context.argument==1){for(unsigned i=1;i<unsigned(OriginalEventId::Count);++i)if(debug_equal(cmd,"event")||debug_ancient_id(OriginalEventId(i)))if(const char*key=consider(original_event_key(OriginalEventId(i))))return key;
    }else if(debug_equal(cmd,"ancient")&&context.argument==2){const OriginalEventId event=debug_event_id(context.first);for(unsigned i=0;i<WorldRelicKinds;++i){const char*key=world_relic_key(RelicId(i));if(debug_ancient_offer(event,key)==i)if(const char*found=consider(key))return found;}
    }else if(debug_equal(cmd,"fight")&&context.argument==1){for(unsigned i=1;i<EncounterKinds;++i){const auto*d=encounter_definition(EncounterId(i));if(d)if(const char*key=consider(d->key))return key;}
    }else if(debug_equal(cmd,"room")&&context.argument==1){const char*const names[]={"Monster","Elite","Boss","Treasure","Shop","Event","RestSite","Map"};for(const char*name:names)if(const char*key=consider(name))return key;
    }else if(debug_equal(cmd,"act")&&context.argument==1){for(unsigned i=0;i<4;++i)if(const char*key=consider(debug_act_key(i)))return key;
    }else if(debug_equal(cmd,"new")&&context.argument==1){for(const auto&ch:character_definitions)if(const char*key=consider(ch.key))return key;
    }else if((debug_equal(cmd,"dump")||debug_equal(cmd,"unlock"))&&context.argument==1){const char*const names[]={"cards","relics","potions","powers","events","fights","acts","monsters","ascensions","all"};for(const char*name:names)if(const char*key=consider(name))return key;
    }else if(debug_equal(cmd,"kill")&&context.argument==1){if(const char*key=consider("all"))return key;}
    return nullptr;
}
static unsigned debug_completion_count(const DebugCompletionContext&context){unsigned n=0;debug_completion_candidate(context,~0u,&n);return n;}
static void debug_accept_completion(App&a){
    const auto context=debug_completion_context(a.debug_completion);const char*key=debug_completion_candidate(context,unsigned(mx(0,a.debug_selection)));if(!key){a.debug_completing=false;return;}
    debug_copy(a.debug_line,sizeof a.debug_line,a.debug_completion);a.debug_line[context.start]=0;unsigned n=context.start;
    for(;*key&&n+1<sizeof a.debug_line;++key)a.debug_line[n++]=*key;
    if(n+1<sizeof a.debug_line)a.debug_line[n++]=' ';a.debug_line[n]=0;a.debug_caret=uint8_t(n);a.debug_completing=false;a.debug_history_index=-1;
}
static void debug_complete(App&a){
    if(a.debug_completing){const unsigned count=a.debug_completion_total;if(count)a.debug_selection=(a.debug_selection+1)%int(count);return;}
    debug_copy(a.debug_completion,sizeof a.debug_completion,a.debug_line);const auto context=debug_completion_context(a.debug_completion);const unsigned count=debug_completion_count(context);
    if(!count){set_debug_reply(a,"No matching command or model ID.",2);return;}a.debug_completion_total=uint16_t(count);a.debug_selection=0;a.debug_completing=true;if(count==1)debug_accept_completion(a);
}
static void debug_history_move(App&a,int direction){
    if(!a.debug_history_count)return;int next=a.debug_history_index+direction;next=mx(-1,mn(int(a.debug_history_count)-1,next));
    if(a.debug_history_index<0&&next>=0)debug_copy(a.debug_draft,128,a.debug_line);
    a.debug_history_index=int8_t(next);debug_copy(a.debug_line,128,next<0?a.debug_draft:a.debug_history[next]);a.debug_caret=uint8_t(debug_length(a.debug_line));
}
static bool debug_unlock(App&a,char**args,unsigned count){
    if(count<2)return debug_failure(a,"unlock TYPE [model IDs...]; use help unlock");
    const bool all=debug_equal(args[1],"all");
    const bool acts=debug_equal(args[1],"acts"),ascensions=debug_equal(args[1],"ascensions");
    const bool cards=debug_equal(args[1],"cards"),relics=debug_equal(args[1],"relics"),potions=debug_equal(args[1],"potions");
    const bool monsters=debug_equal(args[1],"monsters"),events=debug_equal(args[1],"events");
    if(!(all||acts||ascensions||cards||relics||potions||monsters||events))
        return debug_failure(a,"Unknown unlock category. Timeline/epochs are not present in this port.");
    if(ascensions&&count>2)return debug_failure(a,"unlock ascensions takes no model IDs in the original.");
    // UnlockConsoleCmd validates every supplied model before changing the
    // profile. Validate first; never put a UiProgress rollback on the stack.
    if(!all&&!acts&&!ascensions)for(unsigned argument=2;argument<count;++argument){
        const char*id=args[argument];bool valid=false;
        if(cards)valid=debug_card_id(id)>=0;
        else if(potions)valid=debug_potion_id(id)!=Potion::Empty;
        else if(events)valid=debug_event_id(id)!=OriginalEventId::None;
        else if(relics){for(unsigned i=0;i<WorldRelicKinds;++i)if(debug_equal(id,world_relic_key(RelicId(i)))){valid=true;break;}}
        else if(monsters){for(unsigned i=0;i<EnemyKinds;++i){const auto*d=enemy_definition(EnemyId(i));if(d&&debug_equal(id,d->key)){valid=true;break;}}}
        if(!valid){char message[128]{};TextBuffer b{message,sizeof message,0};b.text("Unknown ");b.text(args[1]);b.text(" ID: ");b.text(id);b.text(". No entries changed.");return debug_failure(a,message);}
    }
    auto match=[&](const char*key){if(all||acts||count==2)return true;for(unsigned i=2;i<count;++i)if(debug_equal(key,args[i]))return true;return false;};
    bool category=false,changed=false;
    if(all||debug_equal(args[1],"cards")){category=true;for(unsigned i=0;i<CardKinds;++i)if(match(definitions[i].key)){a.progress.discovered_cards[i/8]|=uint8_t(1u<<(i%8));changed=true;}}
    if(all||debug_equal(args[1],"relics")){category=true;for(unsigned i=0;i<WorldRelicKinds;++i)if(match(world_relic_key(RelicId(i)))){a.progress.discovered_relics[i/8]|=uint8_t(1u<<(i%8));changed=true;}}
    if(all||debug_equal(args[1],"potions")){category=true;for(unsigned i=0;i<PotionKinds;++i)if(match(potion_definitions[i].key)){a.progress.discovered_potions[i/8]|=uint8_t(1u<<(i%8));changed=true;}}
    if(all||debug_equal(args[1],"events")){category=true;for(unsigned i=1;i<unsigned(OriginalEventId::Count);++i)if(match(original_event_key(OriginalEventId(i)))){a.progress.discovered_events[(i-1)/8]|=uint8_t(1u<<((i-1)%8));changed=true;}}
    if(all||monsters){category=true;for(unsigned i=0;i<EnemyKinds;++i){const auto*d=enemy_definition(EnemyId(i));if(d&&match(d->key)){
        bool has_fights=false;for(unsigned c=1;c<=CharacterKinds;++c){const auto&s=a.progress.enemies[i][c];has_fights|=s.wins||s.losses;}
        if(!has_fights){
            // The original explicit developer command adds one Ironclad win
            // only when FightStats is empty; Bestiary checks wins, not seen.
            auto&total=a.progress.enemies[i][0];auto&ironclad=a.progress.enemies[i][unsigned(Character::Ironclad)+1];
            // Merely spawning a monster increments this port's seen counter;
            // the source creates FightStats only after a win or loss.
            ironclad.wins=1;if(!ironclad.seen)ironclad.seen=1;total.wins=progress_add(total.wins,1);const unsigned seen=progress_add(total.wins,total.losses);if(total.seen<seen)total.seen=seen;
        }changed=true;
    }}}
    if(all||debug_equal(args[1],"acts")){category=true;for(unsigned i=0;i<4;++i)if(match(debug_act_key(i))){a.progress.discovered_acts|=uint8_t(1u<<i);changed=true;}}
    if(all||debug_equal(args[1],"ascensions")){category=true;for(unsigned i=0;i<CharacterKinds;++i)if(match(character_definitions[i].key)){a.progress.characters[i].highest_ascension=10;changed=true;}}
    if(!category)return debug_failure(a,"Unknown unlock category. Timeline/epochs are not present in this port.");
    if(!changed)return debug_failure(a,"No matching model IDs.");a.progress_dirty=true;a.persist_progress();set_debug_reply(a,"Unlocked the selected local collection entries.",1);return true;
}
static bool debug_dump(App&a,char**args,unsigned count){
    if(count>3)return debug_failure(a,"dump [cards|relics|potions|powers|events|fights|acts] [page]");
    const char*type=count>=2?args[1]:"cards";int page=1;if(count==3&&!debug_number(args[2],1,100,page))return debug_failure(a,"Page must be 1..100.");
    unsigned total=0;for(unsigned i=0;i<1000;++i){const char*key=nullptr;
        if(debug_equal(type,"cards")){if(i>=CardKinds)break;key=definitions[i].key;}
        else if(debug_equal(type,"relics")){if(i>=WorldRelicKinds)break;key=world_relic_key(RelicId(i));}
        else if(debug_equal(type,"potions")){if(i>=PotionKinds)break;key=potion_definitions[i].key;}
        else if(debug_equal(type,"powers")){if(i>=StatusKinds)break;key=debug_power_key(i);}
        else if(debug_equal(type,"events")){if(i+1>=unsigned(OriginalEventId::Count))break;key=original_event_key(OriginalEventId(i+1));}
        else if(debug_equal(type,"fights")){if(i+1>=EncounterKinds)break;const auto*d=encounter_definition(EncounterId(i+1));key=d?d->key:nullptr;}
        else if(debug_equal(type,"acts")){if(i>=4)break;key=debug_act_key(i);}
        else return debug_failure(a,"Unknown dump category.");
        if(!key||!*key)continue;if(total>=unsigned(page-1)*12&&total<unsigned(page)*12)debug_print(a,key);++total;
    }
    char message[80]{};TextBuffer b{message,sizeof message,0};b.text(type);b.text(" page ");b.number(page);b.ch('/');b.number((total+11)/12);b.text("; Tab completes IDs.");set_debug_reply(a,message,1);return true;
}
}
void App::debug_preset(){if(debug_completing)debug_accept_completion(*this);}
bool App::debug_command(const char*command){
    char text[128]{};unsigned n=0;if(!command)return debug_failure(*this,"Empty command.");
    for(;command[n]&&n<sizeof(text)-1;++n){if(uint8_t(command[n])<32||uint8_t(command[n])>126)return debug_failure(*this,"Commands use ASCII model IDs.");text[n]=command[n];}
    if(command[n])return debug_failure(*this,"Command is too long.");if(!n)return false;
    debug_remember(*this,text);char echo[132]{};echo[0]='>';echo[1]=' ';debug_copy(echo+2,sizeof echo-2,text);debug_print(*this,echo,3);
    char*args[10]{};unsigned count=0;char*p=text;while(*p){while(*p==' ')++p;if(!*p)break;if(count==10)return debug_failure(*this,"Too many arguments.");args[count++]=p;while(*p&&*p!=' ')++p;if(*p)*p++=0;}
    if(!count)return false;const char*cmd=args[0];int command_index=-1;for(int i=0;i<DebugCommandCount;++i)if(debug_equal(cmd,DebugCommands[i].name)){command_index=i;break;}
    if(command_index<0)return debug_failure(*this,"Unknown command. Type help or press Tab.");
    if(debug_equal(cmd,"help")){
        if(count>2)return debug_failure(*this,"help [command]");
        if(count==2){for(const auto&entry:DebugCommands)if(debug_equal(args[1],entry.name)){set_debug_reply(*this,entry.usage);return true;}return debug_failure(*this,"Unknown command.");}
        set_debug_reply(*this,"Commands (Tab completes names and model IDs):");
        for(unsigned i=0;i<32;i+=4){char row[128]{};TextBuffer b{row,sizeof row,0};for(unsigned j=i;j<i+4&&j<32;++j){if(j!=i)b.text("  ");b.text(DebugCommands[j].name);}debug_print(*this,row);}
        debug_print(*this,"Actors: 0 player, 1.. enemies. kill uses 0-based enemies. help COMMAND shows syntax.");return true;
    }
    if(debug_equal(cmd,"clear")){if(count!=1)return debug_failure(*this,"clear takes no arguments.");debug_output_count=0;debug_reply[0]=0;return true;}
    if(debug_equal(cmd,"dump"))return debug_dump(*this,args,count);
    if(debug_equal(cmd,"unlock"))return debug_unlock(*this,args,count);
    if(debug_equal(cmd,"bestiary")){if(count!=1)return debug_failure(*this,"bestiary takes no arguments.");if(!open_bestiary(true))return debug_failure(*this,"Bestiary is unavailable.");set_debug_reply(*this,"Bestiary opened. Close console to view it.",1);return true;}
    if(debug_equal(cmd,"instant")){if(count!=1)return debug_failure(*this,"instant takes no arguments.");set_fast_mode(true);set_debug_reply(*this,"Fast Mode enabled.",1);return true;}
    if(command_index>=32)return debug_failure(*this,DebugCommands[command_index].usage);
    if(debug_equal(cmd,"new")){
        if(count>3)return debug_failure(*this,DebugCommands[command_index].usage);Character character=Character::Ironclad;int seed=1;
        if(count>=2){int found=-1;for(int i=0;i<CharacterKinds;++i)if(debug_equal(args[1],character_definitions[i].key))found=i;if(found<0)return debug_failure(*this,"Unknown character ID.");character=Character(found);}
        if(count==3&&!debug_number(args[2],1,2147483647,seed))return debug_failure(*this,"Seed must be 1..2147483647.");
        debug_begin(*this,character,uint32_t(seed));set_debug_reply(*this,"Run started.",1);return validate(game);
    }
    if(title||!game.world.active)return debug_failure(*this,"A run is not in progress. Start a game or use new.");
    bool ok=false;int amount=0,target=0;
    if(debug_equal(cmd,"room")||debug_equal(cmd,"fight")||debug_equal(cmd,"event")||debug_equal(cmd,"ancient")){
        if(count<2||count>3)return debug_failure(*this,DebugCommands[command_index].usage);
        WorldRoom room=WorldRoom::None;EncounterId encounter=EncounterId::None;OriginalEventId event=OriginalEventId::None;RelicId forced_offer=NoWorldRelic;
        if(debug_equal(cmd,"fight")){if(count!=2)return debug_failure(*this,"fight encounter-ID");encounter=debug_fight_id(args[1]);const auto*d=encounter_definition(encounter);if(encounter==EncounterId::None||!d)return debug_failure(*this,"Unknown encounter ID.");room=d->type==EncounterClass::Boss?WorldRoom::Boss:d->type==EncounterClass::Elite?WorldRoom::Elite:WorldRoom::Monster;}
        else if(debug_equal(cmd,"event")||debug_equal(cmd,"ancient")){event=debug_event_id(args[1]);if(event==OriginalEventId::None)return debug_failure(*this,"Unknown event ID.");if(debug_equal(cmd,"ancient")&&!debug_ancient_id(event))return debug_failure(*this,"ID is not an Ancient.");if(count==3){if(!debug_equal(cmd,"ancient"))return debug_failure(*this,"event event-ID");forced_offer=debug_ancient_offer(event,args[2]);if(forced_offer==NoWorldRelic)return debug_failure(*this,"Unknown offering for this Ancient.");}room=WorldRoom::Event;}
        else{
            if(debug_equal(args[1],"Monster")||debug_equal(args[1],"Combat"))room=WorldRoom::Monster;
            else if(debug_equal(args[1],"Elite"))room=WorldRoom::Elite;else if(debug_equal(args[1],"Boss"))room=WorldRoom::Boss;
            else if(debug_equal(args[1],"Shop"))room=WorldRoom::Shop;else if(debug_equal(args[1],"RestSite")||debug_equal(args[1],"Rest")||debug_equal(args[1],"Camp"))room=WorldRoom::Rest;
            else if(debug_equal(args[1],"Treasure"))room=WorldRoom::Treasure;else if(debug_equal(args[1],"Event"))room=WorldRoom::Event;else if(debug_equal(args[1],"Map"))room=WorldRoom::Map;else return debug_failure(*this,"Unknown room type.");
            if(count==3){if(room==WorldRoom::Event){event=debug_event_id(args[2]);if(event==OriginalEventId::None)return debug_failure(*this,"Unknown event ID.");}
                else if(room==WorldRoom::Monster||room==WorldRoom::Elite||room==WorldRoom::Boss){encounter=debug_fight_id(args[2]);if(encounter==EncounterId::None)return debug_failure(*this,"Unknown encounter ID.");}
                else return debug_failure(*this,"This room has no model argument.");}
        }
        ok=debug_enter_room(game,room,encounter,event,forced_offer);
    }else if(debug_equal(cmd,"act")){
        if(count!=2)return debug_failure(*this,DebugCommands[command_index].usage);
        int act=-1;if(debug_number(args[1],1,3,act))ok=debug_enter_act(game,unsigned(act-1));else{int region=-1;for(int i=0;i<4;++i)if(debug_equal(args[1],debug_act_key(i)))region=i;if(region<0)return debug_failure(*this,"Unknown act number or ID.");ok=debug_replace_act(game,reference::ActKind(region));}
    }else if(debug_equal(cmd,"kill")||debug_equal(cmd,"win")){
        if(count>2||(debug_equal(cmd,"win")&&count!=1))return debug_failure(*this,DebugCommands[command_index].usage);
        target=debug_equal(cmd,"win")?-1:0;if(count==2){if(debug_equal(args[1],"all"))target=-1;else if(!debug_number(args[1],0,MaxEnemies-1,target))return debug_failure(*this,"kill uses 0-based enemy indices.");}ok=debug_kill_enemies(game,target);
    }else if(debug_equal(cmd,"card")||debug_equal(cmd,"remove_card")){
        if(count<2||count>3)return debug_failure(*this,DebugCommands[command_index].usage);const int id=debug_card_id(args[1]);if(id<0)return debug_failure(*this,"Unknown card ID.");DebugPile pile=DebugPile::Hand;
        if(count==3&&!debug_parse_pile(args[2],pile))return debug_failure(*this,"Supported piles: Hand, Draw, Discard, Exhaust, Deck.");
        ok=debug_card(game,CardKind(id),pile,debug_equal(cmd,"remove_card"));
    }else if(debug_equal(cmd,"upgrade")){
        if(count>2||(count==2&&!debug_number(args[1],0,MaxHand-1,target)))return debug_failure(*this,DebugCommands[command_index].usage);ok=debug_upgrade(game,unsigned(target));
    }else if(debug_equal(cmd,"relic")){
        if(count<2||count>3)return debug_failure(*this,DebugCommands[command_index].usage);bool remove=false;const char*key=args[1];
        if(debug_equal(key,"add")||debug_equal(key,"remove")){if(count!=3)return debug_failure(*this,"relic add|remove ID");remove=debug_equal(key,"remove");key=args[2];}else if(count!=2)return debug_failure(*this,"relic [add|remove] ID");
        const RelicId id=debug_relic_id(key);if(id==NoWorldRelic)return debug_failure(*this,"Unknown relic ID.");ok=debug_relic(game,id,remove);
    }else if(debug_equal(cmd,"potion")){
        if(count!=2)return debug_failure(*this,"potion ID");const Potion id=debug_potion_id(args[1]);if(id==Potion::Empty)return debug_failure(*this,"Unknown potion ID. Press Tab after potion.");ok=debug_potion(game,id);
    }else if(debug_equal(cmd,"power")){
        if(count!=4||!debug_number(args[2],-9999,9999,amount)||!debug_number(args[3],0,MaxEnemies,target))return debug_failure(*this,"power ID amount actor-index (0=player)");const int id=debug_power_id(args[1],target);if(id<0)return debug_failure(*this,"Unknown power ID; use the _POWER suffix.");ok=debug_power(game,unsigned(id),amount,target);
    }else if(debug_equal(cmd,"enchant")||debug_equal(cmd,"afflict")){
        amount=1;if(count<2||count>4||(count>=3&&!debug_number(args[2],1,9999,amount))||(count==4&&!debug_number(args[3],0,MaxHand-1,target)))return debug_failure(*this,DebugCommands[command_index].usage);
        if(debug_equal(cmd,"enchant")){int id=-1;for(unsigned i=1;i<EventEnchantmentKinds;++i)if(debug_equal(args[1],event_enchantment_key(EventEnchantment(i))))id=int(i);if(id<0)return debug_failure(*this,"Unknown enchantment ID.");ok=debug_enchant(game,EventEnchantment(id),amount,unsigned(target));}
        else{int id=-1;for(unsigned i=1;i<unsigned(EnemyAffliction::Count);++i)if(debug_equal(args[1],debug_affliction_key(i)))id=int(i);if(id<0)return debug_failure(*this,"Unknown affliction ID.");ok=debug_afflict(game,EnemyAffliction(id),amount,unsigned(target));}
    }else if(debug_equal(cmd,"godmode")){
        if(count!=1)return debug_failure(*this,"godmode takes no arguments.");const bool enabled=!debug_godmode(game);ok=debug_set_godmode(game,enabled);if(ok)set_debug_reply(*this,enabled?"Godmode activated (bounded Strength/Regen/Buffer).":"Godmode deactivated.",1);
    }else if(debug_equal(cmd,"travel")){
        if(count!=1)return debug_failure(*this,"travel takes no arguments.");const bool enabled=!debug_travel(game);debug_set_travel(game,enabled);set_debug_reply(*this,enabled?"Travel mode enabled. Choose any map point.":"Travel mode disabled.",1);ok=true;
    }else{
        DebugValue value=DebugValue::Heal;int low=0,high=9999;
        if(debug_equal(cmd,"die")){if(count!=1)return debug_failure(*this,"die takes no arguments.");value=DebugValue::Die;}
        else{
            if(debug_equal(cmd,"gold")){value=DebugValue::Gold;low=-9999;}
            else if(debug_equal(cmd,"energy")){value=DebugValue::Energy;high=999;}
            else if(debug_equal(cmd,"stars"))value=DebugValue::Stars;
            else if(debug_equal(cmd,"draw")){value=DebugValue::Draw;low=1;high=MaxCards;amount=1;}
            else if(debug_equal(cmd,"block"))value=DebugValue::Block;
            else if(debug_equal(cmd,"damage")){value=DebugValue::Damage;target=-1;}
            else if(!debug_equal(cmd,"heal"))return debug_failure(*this,"Command is unavailable.");
            if(count>3||(count<2&&value!=DebugValue::Draw)||(count>=2&&!debug_number(args[1],low,high,amount)))return debug_failure(*this,DebugCommands[command_index].usage);
            if(count==3){if(value!=DebugValue::Heal&&value!=DebugValue::Damage&&value!=DebugValue::Block)return debug_failure(*this,"Unexpected target argument.");if(!debug_number(args[2],0,value==DebugValue::Heal?1:MaxEnemies,target))return debug_failure(*this,"Invalid target index.");}
        }
        ok=debug_value(game,value,amount,target);
    }
    if(!ok)return debug_failure(*this,"Command cannot run here: check target, capacity and pending choices.");
    // Console commands do not autosave over the user's prior checkpoint.
    forget_run_checkpoint();title=character_select=library=paused=help=settings_open=false;
    compendium_open=statistics_open=history_open=bestiary.open=profiles_open=profile_pending=false;frontend_confirm=0;map_preview.open=false;reset_presentation();pointer_cancel();
    if(game.phase==Phase::Combat&&game.turn==1&&(debug_equal(cmd,"fight")||debug_equal(cmd,"room")))turn_banner_suppress_intro=false;
    if(!debug_equal(cmd,"godmode")&&!debug_equal(cmd,"travel"))set_debug_reply(*this,"OK",1);return true;
}
static void debug_submit(App&a){
    if(a.debug_completing){debug_accept_completion(a);return;}
    char line[128];debug_copy(line,sizeof line,a.debug_line);if(!*line)return;a.debug_command(line);a.debug_line[0]=0;a.debug_caret=0;a.debug_history_index=-1;a.debug_draft[0]=0;
}
bool App::debug_input(Key key){
    if(key==Key::Debug){debug_open=!debug_open;debug_editing=debug_open;debug_completing=false;pointer_cancel();physical_keys_cancel();pointer.visible=false;hit_count=0;
        if(debug_open&&!debug_output_count){debug_print(*this,"Slay the Spire 2 - Developer Console",1);debug_print(*this,"Type help. Tab completes commands and model IDs.");}return true;}
    if(!debug_open)return false;
    if(key==Key::Back){if(debug_completing)debug_completing=false;else{debug_open=debug_editing=false;pointer_cancel();}return true;}
    if(key==Key::Up||key==Key::Down){
        if(debug_completing){const unsigned count=debug_completion_total;if(count)debug_selection=(debug_selection+(key==Key::Up?-1:1)+int(count))%int(count);}
        else debug_history_move(*this,key==Key::Up?1:-1);
    }else if(key==Key::Left||key==Key::Right){debug_completing=false;debug_caret=uint8_t(mx(0,mn(int(debug_length(debug_line)),int(debug_caret)+(key==Key::Left?-1:1))));}
    else if(key==Key::Confirm)debug_submit(*this);
    else if(key==Key::Detail)debug_editing=true;
    return true;
}
void App::debug_text(int code){
    if(!debug_open||!debug_editing)return;
    if(code==27){debug_input(Key::Back);return;}if(code==13||code==10){debug_submit(*this);return;}if(code==9){debug_complete(*this);return;}
    const unsigned n=debug_length(debug_line);unsigned caret=mn(debug_caret,int(n));debug_completing=false;debug_history_index=-1;
    if(code==1){debug_caret=0;return;}if(code==5){debug_caret=uint8_t(n);return;}if(code==12){debug_output_count=0;return;}
    if(code==11){debug_line[caret]=0;return;}if(code==21){debug_line[0]=0;debug_caret=0;return;}
    unsigned from=caret,to=caret;
    if(code==8&&caret){from=caret-1;}
    else if(code==127&&caret<n){to=caret+1;}
    else if(code==23&&caret){from=caret;while(from&&debug_line[from-1]==' ')--from;while(from&&debug_line[from-1]!=' ')--from;}
    else if(code>=32&&code<=126&&n+1<sizeof debug_line){for(unsigned i=n+1;i>caret;--i)debug_line[i]=debug_line[i-1];debug_line[caret]=char(code);debug_caret=uint8_t(caret+1);return;}
    if(from!=to){for(unsigned i=to;i<=n;++i)debug_line[from+i-to]=debug_line[i];debug_caret=uint8_t(from);}
}
static void debug_screen(Canvas&c){
    auto&a=c.a;a.hit_count=0;c.dim(0,0,320,224,235);c.rect(0,0,320,26,rgb(23,25,30));c.text("Developer Console",8,7,12,Gold);c.close_button(293,3);
    const int old_w=c.clip_w,old_h=c.clip_h;c.clip_w=314;c.clip_h=191;
    if(a.debug_completing){
        const auto context=debug_completion_context(a.debug_completion);const unsigned count=a.debug_completion_total;
        const int first=mx(0,mn(a.debug_selection-5,int(count)-13));
        for(int row=0;row<13&&unsigned(first+row)<count;++row){const int index=first+row,y=32+row*12;const bool selected=index==a.debug_selection;
            if(selected)c.rect(6,y-1,307,12,rgb(65,58,34));const char*key=debug_completion_candidate(context,unsigned(index));if(key)c.text(key,10,y,10,selected?Gold:White);
            c.hit(6,y-1,307,12,HitKind::DebugItem,index);
        }
    }else{
        const unsigned maximum=a.debug_output_count>13?a.debug_output_count-13:0;
        const unsigned first=maximum-unsigned(mn(a.debug_output_scroll,int(maximum)));
        for(unsigned i=first;i<a.debug_output_count&&i<first+13;++i){const unsigned kind=a.debug_output_kind[i];c.text(a.debug_output[i],8,32+int(i-first)*12,10,kind==2?Red:kind==1?Green:kind==3?Gold:White);}
    }
    c.clip_w=old_w;c.clip_h=old_h;c.rect(5,194,310,25,Ink);c.box(5,194,310,25,rgb(115,112,94));c.text(">",10,200,12,Gold);
    char prefix[128];debug_copy(prefix,sizeof prefix,a.debug_line);prefix[mn(a.debug_caret,int(sizeof prefix)-1)]=0;
    const int caret_width=c.width(prefix,12),offset=mx(0,caret_width-279);Canvas input=c;input.pixels+=197*c.pitch+23;input.clip_w=288;input.clip_h=19;if(input.coverage)input.coverage+=197*c.pitch+23;
    input.text(a.debug_line,2-offset,3,12,White,2048);if((a.map_clock_ms/500u)%2==0)input.rect(2+caret_width-offset,3,1,13,Gold);
    c.hit(5,194,310,25,HitKind::Command,int(Key::Detail));
    c.rect(0,224,320,16,rgb(23,25,30));c.text(a.debug_completing?"Tab/arrows: select   Enter: accept   Esc: cancel":"Up/Down: history   Tab: complete   Flag/Esc: close",7,228,8,Muted);
}
