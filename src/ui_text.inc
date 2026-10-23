// UI labels use the same bounded formatter as cards/events. Resolve from the
// actual selection, never a guessed zero or the number currently highlighted.
struct UiTextContext {const App* app;const char* hotkey;};
static LocalizedValue ui_text_value(const void* opaque,const char* name){
    const auto& context=*static_cast<const UiTextContext*>(opaque);
    const auto& a=*context.app;const auto& s=a.game;
    if(wt_equal(name,"Hotkey")||wt_equal(name,"HotKey"))return wt_string(context.hotkey);
    if(wt_equal(name,"Amount")){
        if(s.phase==Phase::OriginalEventChoice||s.phase==Phase::WorldRelicChoice)return wt_number(int(a.world_choice_max()));
        if(s.phase==Phase::ExhaustChoice||s.phase==Phase::ExhaustManyChoice||s.phase==Phase::DiscardChoice||
           s.phase==Phase::ReturnChoice||s.phase==Phase::ExtendedChoice)return wt_number(int(choice_max(s)));
        if(s.phase==Phase::Upgrade||s.phase==Phase::Remove||s.phase==Phase::Transform||s.phase==Phase::Shop||a.grimoire_select)return wt_number(1);
        return {};
    }
    if(wt_equal(name,"ascension"))return wt_number(a.character_select||a.run_settings?a.selected_ascension:world_ascension(s));
    if(wt_equal(name,"actNumber"))return wt_number(s.world.active?s.world.act_index+1:1);
    if(wt_equal(name,"singleStarIcon"))return wt_string(a.content.english()?"Stars":"辉星");
    return {};
}
const char* App::ui_text(const char* source,char* buffer,size_t capacity)const{
    const char* raw=content.ui_text(source);bool has_template=false;
    for(const char* p=raw;*p;++p)if(*p=='{'){has_template=true;break;}
    if(!has_template||!buffer||!capacity)return raw;
    // Inspect is Tab on both real hosts. The exhaust pile is opened through
    // the pile UI and has no independent keyboard action; None deliberately
    // selects the original optional-hint branch instead of inventing a key.
    const char* hotkey="None";
    if(wt_equal(source,"牌组")||wt_equal(raw,content.ui_text("牌组")))hotkey="Tab";
    UiTextContext context{this,hotkey};
    LocalizedTemplate(buffer,capacity,ui_text_value,&context,content.english()?"Energy":"能量",content.english()?"Stars":"辉星",content.english()).format(raw);
    return buffer;
}
