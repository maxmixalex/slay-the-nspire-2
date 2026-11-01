struct TimelineTextContext {const App*a;unsigned id;};
static LocalizedValue timeline_text_value(const void*data,const char*key){
    const auto&v=*static_cast<const TimelineTextContext*>(data);const auto&epoch=timeline_epochs[v.id];
    const auto same=[](const char*a,const char*b){while(*a&&*a==*b){++a;++b;}return *a==*b;};
    if(same(key,"IsRevealed"))return {true,v.a->progress.timeline.states[v.id]==EpochState::Revealed};
    if(key[0]=='P'&&key[1]=='o'&&key[2]=='t'&&key[3]=='i'&&key[4]=='o'&&key[5]=='n'&&key[6]>='1'&&key[6]<='3'&&!key[7]){
        const unsigned i=unsigned(key[6]-'1');const auto*d=potion_definition(Potion(epoch.models[i]));
        return {true,0,1,d?v.a->content.world_title("potions",d->key):""};
    }
    if(same(key,"Event"))return {true,0,1,v.a->content.world_title("events",original_event_key(OriginalEventId(epoch.models[0])))};
    return {};
}
static const char* timeline_text(const App&a,unsigned id,unsigned field){return a.content.string(int(TimelineTextBase+id*4+field));}
static void timeline_screen(Canvas&c){
    auto&a=c.a;const bool zh=a.content.language()==2;const auto&t=a.progress.timeline;
    title_submenu_background(c,195);a.hit_count=0;
    const uint16_t cream=rgb(255,246,226),gold=rgb(239,200,81);
    profile_text_box(c,zh?"时间线":"Timeline",56,6,208,24,{12,source_label_face(a.content.original_label_fonts(),5),gold,64},true,false);
    if(!a.content.has_timeline_text()){
        profile_text_box(c,zh?"请安装配套的时间线资源。":"Install the matching Timeline resources.",15,85,290,60,{10,0,cream,128},true,true);
        native_original_back(c,a.progress_page_started);return;
    }
    if(!a.timeline_detail){
        char summary[96]{};TextBuffer line{summary,sizeof summary,0};line.number(timeline_obtained_count(t));line.text(" / 57   ");line.number(t.score);line.ch('/');line.number(timeline_next_threshold(t));
        profile_text_box(c,summary,10,29,300,16,{10,0,cream,64},true,false);
        uint8_t items[EpochCount];const unsigned n=a.timeline_items(items);unsigned selected=0;while(selected<n&&items[selected]!=a.timeline_selected)++selected;
        const unsigned first=selected/7*7;
        for(unsigned row=0;row<7&&first+row<n;++row){const unsigned id=items[first+row];const int y=49+int(row)*22;
            if(id==a.timeline_selected)c.rect(9,y-1,302,21,rgb(56,65,64));
            char label[512]{};TextBuffer text{label,sizeof label,0};text.number(timeline_epochs[id].era);text.text("  ");
            const auto state=t.states[id];text.text(state==EpochState::Revealed?"+ ":unsigned(state)>=3?"! ":"- ");text.text(timeline_text(a,id,0));
            profile_text_box(c,label,14,y,291,19,{10,0,state==EpochState::Revealed?cream:gold,128},false,false);
            c.hit(9,y-1,302,21,HitKind::TimelineEpoch,int(id));
        }
        if(t.flags&TimelineLegacyAccess)profile_text_box(c,zh?"旧版兼容开放，历史获得时间未知。":"Legacy access: content open; old obtain dates unknown.",48,207,264,30,{10,0,Muted,128},false,true);
        else profile_text_box(c,zh?"选择条目，按 Enter 查看。":"Select an Epoch; Enter to inspect.",61,210,240,21,{10,0,Muted,64},true,false);
    }else{
        const unsigned id=a.timeline_selected;const auto&epoch=timeline_epochs[id];const auto state=t.states[id];
        profile_text_box(c,timeline_text(a,id,0),11,29,297,36,{12,source_label_face(a.content.original_label_fonts(),5),gold,128},true,true);
        char body[4096]{},formatted[2048]{},date[80]{},clock[48]{};TextBuffer text{body,sizeof body,0};
        if(epoch.story<8){text.text(a.content.string(TimelineStoryBase+epoch.story));text.ch('\n');}
        text.text(zh?"获得时间：":"Obtained: ");progress_date_text(a,t.obtained_at[id],date,sizeof date,clock,sizeof clock);text.text(date);text.ch(' ');text.text(clock);text.text("\n\n");
        TimelineTextContext context{&a,id};LocalizedTemplate format(formatted,sizeof formatted,timeline_text_value,&context);format.for_language(a.content.language()).format(timeline_text(a,id,2));
        text.text(formatted);text.text("\n\n");
        if(state==EpochState::Revealed){
            text.text(timeline_text(a,id,1));text.text("\n\n");
            LocalizedTemplate unlock(formatted,sizeof formatted,timeline_text_value,&context);unlock.for_language(a.content.language()).format(timeline_text(a,id,3));text.text(formatted);
            for(unsigned i=0;i<epoch.count;++i){text.ch('\n');
                if(epoch.kind==unsigned(TimelineKind::Card))text.text(a.content.card_string(epoch.models[i]));
                else if(epoch.kind==unsigned(TimelineKind::Relic))text.text(a.content.world_title("relics",world_relic_definitions[epoch.models[i]].key));
                else if(epoch.kind==unsigned(TimelineKind::Potion)){const auto*d=potion_definition(Potion(epoch.models[i]));if(d)text.text(a.content.world_title("potions",d->key));}
                else if(epoch.kind==unsigned(TimelineKind::Event))text.text(a.content.world_title("events",original_event_key(OriginalEventId(epoch.models[i]))));
            }
        }
        Canvas page=c;page.pixels+=68*page.pitch;if(page.coverage)page.coverage+=68*page.pitch;page.clip_h=135;page.font_face=0;
        auto metric=[&](const RichChar&ch,int px){return page.text_metric(ch,px);};
        const int height=fitted_text::line_count(body,282,10,metric)*11+8;
        a.progress_page_scroll.configure(height,135);page.fitted(body,14,-a.progress_page_scroll.offset(),282,height,10,10,cream);
        c.hit(10,68,297,135,HitKind::ProgressScroll);if(a.progress_page_scroll.maximum){c.num(a.progress_page_scroll.offset()/11+1,300,181,8,Muted);}
        if(timeline_revealable(t,Epoch(id))){const char*label=a.content.string(T24_EPOCH_INSPECT_unlockButton);
            profile_text_box(c,label,164,207,145,29,{10,0,gold,64},true,false);c.action(164,207,145,29,UiAction::TimelineReveal);}
        else profile_text_box(c,state==EpochState::Revealed?(zh?"已揭示":"Revealed"):(zh?"尚未获得":"Not obtained"),145,207,164,29,{10,0,Muted,64},true,false);
    }
    native_original_back(c,a.progress_page_started);
}
