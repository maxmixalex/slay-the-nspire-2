static void progress_time_text(char*,size_t,uint32_t);
static void progress_date_text(const App&,uint32_t,char*,size_t,char*,size_t);
struct ProfileTextValues {unsigned id;const char*playtime=nullptr,*updated=nullptr;};
static LocalizedValue profile_text_value(const void*opaque,const char*key){
    const auto&v=*static_cast<const ProfileTextValues*>(opaque);
    if(wt_equal(key,"Id"))return {true,int64_t(v.id)};
    if(wt_equal(key,"Playtime"))return {true,0,1,v.playtime?v.playtime:"—"};
    if(wt_equal(key,"LastUpdatedTime"))return {true,0,1,v.updated?v.updated:"—"};return {};
}
static void profile_text(const App&a,const char*key,const char*fallback,const ProfileTextValues&values,char*out,size_t capacity){
    LocalizedTemplate text(out,capacity,profile_text_value,&values);
    text.format(frontend_text(a,"main_menu_ui",key,fallback));
}
static void profile_title_button(Canvas&c){
    auto&a=c.a;if(!a.callbacks.request_profile||!a.callbacks.profile_info)return;
    const unsigned profile=a.current_profile();char text[64]{};
    const bool hover=a.pointer.visible&&a.pointer.x>=4&&a.pointer.x<62&&a.pointer.y>=4&&a.pointer.y<23;
    const int weight=a.profile_motion[6].observe(hover,a.map_clock_ms),scale=4096+82*weight/4096;
    const int x=4-(scale-4096)*29/4096,y=4;
    if(a.content.version()>=ProfileContentVersion)c.ui(ProfileIcon1+int(profile)-1,x,y,11*scale/4096,11*scale/4096);
    profile_text(a,"OPEN_PROFILE_SCREEN.title",a.content.english()?"Profile {Id}":"存档{Id}",{profile},text,sizeof text);
    c.text_fitted(text,x+13,y,48,8,Gold,8);
    if(a.pointer.visible)c.text_fitted(frontend_text(a,"main_menu_ui","OPEN_PROFILE_SCREEN.description",a.content.english()?"Click to edit":"点击编辑"),x+13,y+9,55,8,White,8);
    c.action(2,2,65,23,UiAction::OpenProfiles);
}
static void profiles_screen(Canvas&c){
    auto&a=c.a;title_background(c,false);c.dim(0,0,320,240,190);a.hit_count=0;
    c.text_fitted(frontend_text(a,"main_menu_ui","PROFILE_SCREEN.BUTTON.chooseProfileMessage",a.content.english()?"Choose a save profile to play!":"选择一个存档进行游玩！"),60,29,200,10,Gold,8,true);
    for(unsigned i=0;i<UiProfileCount;++i){
        const int x=profile_layout::left(i),y=profile_layout::Top;const auto&summary=a.profile_summaries[i];
        const bool interactive=!a.frontend_confirm&&!a.profile_pending;
        const bool focus=interactive&&(a.pointer.visible?(a.pointer.x>=x&&a.pointer.x<x+profile_layout::Width&&a.pointer.y>=y&&a.pointer.y<y+profile_layout::Height):a.profile_selection==i);
        const int weight=a.profile_motion[i].observe(focus,a.map_clock_ms),scale=4096+123*weight/4096;
        const int w=profile_layout::Width*scale/4096,h=profile_layout::Height*scale/4096,left=x+(profile_layout::Width-w)/2,top=y+(profile_layout::Height-h)/2;
        if(a.content.version()>=ProfileContentVersion){c.ui(ProfilePanel,left,top,w,h);c.ui(ProfilePanelFocus,left,top,w,h,weight*255/4096);}
        else c.ui(U_RewardPanel,left,top,w,h);
        if(a.content.version()>=ProfileContentVersion){c.ui(ProfileIcon1+i,left+9,top+8,9,9);
            if(i+1==a.current_profile())c.ui(ProfileCurrentMarker,left+w/2-4,top-5,9,12);}
        char text[512]{},playtime[32]{},date[64]{},time[32]{},updated[112]{};
        if(summary.exists&&!summary.last_saved_at&&!summary.play_seconds){TextBuffer t{playtime,sizeof playtime,0};t.text("—");}
        else progress_time_text(playtime,sizeof playtime,summary.play_seconds);
        progress_date_text(a,summary.last_saved_at,date,sizeof date,time,sizeof time);
        TextBuffer u{updated,sizeof updated,0};u.text(date);if(summary.last_saved_at){u.ch(' ');u.text(time);}
        const ProfileTextValues values{i+1,playtime,updated};
        profile_text(a,"PROFILE_SCREEN.BUTTON.title",a.content.english()?"Save Profile {Id}":"{Id}号存档",values,text,sizeof text);
        c.text_fitted(text,left+20,top+9,w-30,8,Gold,8,true,true);
        profile_text(a,summary.exists?"PROFILE_SCREEN.BUTTON.description":"PROFILE_SCREEN.BUTTON.empty",
            summary.exists?"{Playtime}\n\n{LastUpdatedTime}":a.content.english()?"Empty":"空存档",values,text,sizeof text);
        c.fitted(text,left+9,top+29,w-18,h-39,8,8,White,true,true,true);
        if(interactive)c.action(x,y,profile_layout::Width,profile_layout::Height,UiAction(unsigned(UiAction::SelectProfile1)+i));
        if(!summary.exists)continue;
        const int center=profile_layout::center(i),trash_y=profile_layout::DeleteTop;
        const bool trash_focus=interactive&&(a.pointer.visible?(a.pointer.x>=center-10&&a.pointer.x<center+10&&a.pointer.y>=trash_y&&a.pointer.y<trash_y+20):a.profile_selection==i+3);
        const int trash=a.profile_motion[i+3].observe(trash_focus,a.map_clock_ms),size=15+2*trash/4096;
        if(a.content.version()>=ProfileContentVersion){c.ui(ProfileDelete,center-size/2,trash_y+(15-size)/2,size,size);c.ui(ProfileDeleteFocus,center-size/2,trash_y+(15-size)/2,size,size,trash*255/4096);}
        if(interactive)c.action(center-12,trash_y-2,24,22,UiAction(unsigned(UiAction::DeleteProfile1)+i));
        const unsigned alpha=a.profile_labels[i].observe(trash_focus,a.map_clock_ms),age=a.map_clock_ms-a.profile_labels[i].changed;
        const unsigned t=mn(4096,int(age*4096/200));
        const int label_y=trash_y+reference_viewport::project(trash_focus?48+30*room_curve::back_out(t)/4096:78);
        if(alpha){Canvas label=c;label.opacity=uint8_t(div255(unsigned(c.opacity)*alpha));
            label.text_fitted(frontend_text(a,"main_menu_ui","PROFILE_SCREEN.DELETE_BUTTON.label",a.content.english()?"Delete Profile":"删除存档"),center-40,label_y,80,8,rgb(255,100,100),8,true);}
    }
    if(!a.profile_pending)native_original_back(c,a.profiles_started);
}
