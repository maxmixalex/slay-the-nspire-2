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
// Fixed original front-end labels. Pixel fonts are selected once at the source
// band; hover scales that same face with its parent rather than fitting it to
// another size. The small main-menu helper is the documented4px adaptation.
struct ProfileTransform {
    int x=0,y=0,scale=4096;
    int horizontal(int value)const{return x+((value-x)*scale+((value-x)<0?-2048:2048))/4096;}
    int vertical(int value)const{return y+((value-y)*scale+((value-y)<0?-2048:2048))/4096;}
};
struct ProfileTextStyle {
    int size;unsigned face;uint16_t color;unsigned shadow=128;
    int outline=0;uint16_t outline_color=0;int shadow_outline=0;unsigned outline_opacity=255;
    int shadow_x=1,shadow_y=0;
    int tracking_source=0;
};
static void profile_glyph(Canvas&c,Glyph glyph,int x,int y,uint16_t color,const ProfileTransform&transform,
                          int left=0,int top=0,int right=320,int bottom=240){
    if(!glyph.pixels)return;
    if(transform.scale==4096&&x>=left&&y>=top){Canvas ink=c;ink.clip_w=mn(ink.clip_w,right);ink.clip_h=mn(ink.clip_h,bottom);ink.glyph(glyph,x,y,color);return;}
    unsigned column=0,row=0;
    const auto emit=[&](unsigned alpha){
        if(alpha){const int x0=transform.horizontal(x+int(column)),x1=transform.horizontal(x+int(column)+1);
            const int y0=transform.vertical(y+int(row)),y1=transform.vertical(y+int(row)+1);
            for(int yy=mx(mx(0,top),y0);yy<mn(mn(c.clip_h,bottom),y1);++yy)
                for(int xx=mx(mx(0,left),x0);xx<mn(mn(c.clip_w,right),x1);++xx)c.paint(yy*c.pitch+xx,color,alpha);}
        if(++column==glyph.w){column=0;++row;}
    };
    if(glyph.encoded_size){for(unsigned i=0;i+1<glyph.encoded_size&&row<glyph.h;i+=2)
        for(unsigned n=0;n<glyph.pixels[i]&&row<glyph.h;++n)emit(glyph.pixels[i+1]);}
    else for(unsigned i=0;i<unsigned(glyph.w)*glyph.h;++i)emit(glyph.pixels[i]);
}
static void profile_text_box(Canvas c,const char*text,int x,int y,int width,int height,const ProfileTextStyle&style,
                             bool centered,bool wrap,const ProfileTransform&transform={},bool clipped=false,bool top_aligned=false,int hard_line_step=0){
    const int size=c.a.content.font_size(style.size,style.face,false,true);c.font_face=uint8_t(style.face);c.bold_face=5;
    const auto metrics=[&](const RichChar&ch,int px){const auto g=c.a.content.glyph_metrics(ch.codepoint,px,c.font_style(ch));return fitted_text::Metric{g.advance,mx(int(g.w),int(g.advance))};};
    fitted_text::Plan plan;plan.size=size;plan.step=hard_line_step?hard_line_step:size+1;plan.total=wrap?fitted_text::line_count(text,width,size,metrics):1;
    if(!wrap&&hard_line_step)for(const char*p=text;*p;++p)if(*p=='\n')++plan.total;
    plan.rows=mn(plan.total,32);plan.truncated=false;
    int widths[32]{};
    const auto visit=[&](auto emit){if(wrap)fitted_text::visit(text,width,plan,metrics,emit);
        else{RichReader reader(text);RichChar ch{};int at=0,index=0,row=0;while(reader.next(ch)){if(ch.codepoint=='\n'){if(!hard_line_step||++row>=plan.rows)break;at=index=0;continue;}
            emit(ch,at+reference_viewport::project(index++*style.tracking_source),row,size);at+=metrics(ch,size).advance;}}};
    visit([&](const RichChar&ch,int at,int row,int px){widths[row]=at+metrics(ch,px).advance;});
    const int text_y=top_aligned?y:y+(height-(plan.rows*plan.step+1))/2;
    const int left=clipped?transform.horizontal(x):0,top=clipped?transform.vertical(y):0;
    const int right=clipped?transform.horizontal(x+width):c.clip_w,bottom=clipped?transform.vertical(y+height):c.clip_h;
    const auto draw=[&](int dx,int dy,unsigned opacity,uint16_t color,bool decoration){Canvas ink=c;ink.opacity=uint8_t(div255(unsigned(ink.opacity)*opacity));
        visit([&](const RichChar&ch,int at,int row,int px){if(ch.index>=c.text_visible_characters)return;
            const auto glyph=c.a.content.glyph(ch.codepoint,px,c.font_style(ch));
            const uint16_t colors[]={style.color,rgb(239,200,81),rgb(255,85,85),rgb(135,206,235),rgb(127,255,0),rgb(238,130,238),rgb(255,165,24),rgb(0,255,255),rgb(255,255,255),rgb(128,128,128)};
            const uint16_t rich=colors[ch.style.color];
            profile_glyph(ink,glyph,x+at+(centered?(width-widths[row])/2:0)+dx,text_y+row*plan.step+dy,decoration?color:rich,transform,left,top,right,bottom);});};
    if(style.shadow){const int radius=style.shadow_outline;
        for(int yy=-radius;yy<=radius;++yy)for(int xx=-radius;xx<=radius;++xx)
            if(xx*xx+yy*yy<=radius*radius)draw(xx+style.shadow_x,yy+style.shadow_y,style.shadow,0,true);}
    for(int yy=-style.outline;yy<=style.outline;++yy)for(int xx=-style.outline;xx<=style.outline;++xx)
        if((xx||yy)&&xx*xx+yy*yy<=style.outline*style.outline)draw(xx,yy,style.outline_opacity,style.outline_color,true);
    draw(0,0,255,style.color,false);
}
static void profile_image(Canvas&c,int image,int x,int y,int width,int height,const ProfileTransform&transform,unsigned opacity=255){
    const int left=transform.horizontal(x),top=transform.vertical(y);
    c.ui(image,left,top,mx(1,transform.horizontal(x+width)-left),mx(1,transform.vertical(y+height)-top),opacity);
}
static void profile_title_button(Canvas&c){
    auto&a=c.a;if(!a.callbacks.request_profile||!a.callbacks.profile_info)return;
    const unsigned profile=a.current_profile();char text[64]{};
    constexpr int x=3,y=3,width=33,height=12;
    const bool hover=a.pointer.visible&&a.pointer.x>=x&&a.pointer.x<x+width&&a.pointer.y>=y&&a.pointer.y<y+height;
    const int weight=a.profile_motion[6].observe(hover,a.map_clock_ms,true);
    const ProfileTransform transform{19,9,4096+82*weight/4096};
    if(a.content.version()>=ProfileContentVersion)profile_image(c,ProfileIcon1+int(profile)-1,5,5,9,9,transform);
    profile_text(a,"OPEN_PROFILE_SCREEN.title",a.content.english()?"Profile {Id}":"存档{Id}",{profile},text,sizeof text);
    profile_text_box(c,text,16,a.pointer.visible?4:6,48,7,{5,source_label_face(a.content.english(),5),rgb(239,200,81),0,2,0,0,64},false,false,transform);
    // The authored title has a25%-alpha black12px outline, not the large
    // regular8px label previously shared by all profile text.
    if(a.pointer.visible)profile_text_box(c,frontend_text(a,"main_menu_ui","OPEN_PROFILE_SCREEN.description",a.content.english()?"Click to edit":"点击编辑"),
        16,10,55,6,{4,0,rgb(135,206,235),64},false,false,transform);
    c.action(x,y,width,height,UiAction::OpenProfiles);
}
static void profiles_screen(Canvas&c){
    auto&a=c.a;title_background(c,false);c.dim(0,0,320,240,190);a.hit_count=0;
    profile_text_box(c,frontend_text(a,"main_menu_ui","PROFILE_SCREEN.BUTTON.chooseProfileMessage",a.content.english()?"Choose a save profile to play!":"选择一个存档进行游玩！"),
        60,24,201,18,{7,0,rgb(239,200,81),128},true,false);
    for(unsigned i=0;i<UiProfileCount;++i){
        const int x=profile_layout::left(i),y=profile_layout::Top;const auto&summary=a.profile_summaries[i];
        const bool interactive=!a.frontend_confirm&&!a.profile_pending;
        const bool focus=interactive&&(a.pointer.visible?(a.pointer.x>=x&&a.pointer.x<x+profile_layout::Width&&a.pointer.y>=y&&a.pointer.y<y+profile_layout::Height):a.profile_selection==i);
        const int weight=a.profile_motion[i].observe(focus,a.map_clock_ms);
        const ProfileTransform transform{x+profile_layout::Width/2,y+profile_layout::Height/2,4096+123*weight/4096};
        if(a.content.version()>=ProfileContentVersion){profile_image(c,ProfilePanel,x,y,profile_layout::Width,profile_layout::Height,transform);
            profile_image(c,ProfilePanelFocus,x,y,profile_layout::Width,profile_layout::Height,transform,weight*255/4096);}
        else profile_image(c,U_RewardPanel,x,y,profile_layout::Width,profile_layout::Height,transform);
        if(a.content.version()>=ProfileContentVersion){profile_image(c,ProfileIcon1+i,x+9,y+8,9,9,transform);
            if(i+1==a.current_profile())profile_image(c,ProfileCurrentMarker,x+profile_layout::Width/2-5,y-5,9,12,transform);}
        char text[512]{},playtime[32]{},date[64]{},time[32]{},updated[112]{};
        if(summary.exists&&!summary.last_saved_at&&!summary.play_seconds){TextBuffer t{playtime,sizeof playtime,0};t.text("—");}
        else progress_time_text(playtime,sizeof playtime,summary.play_seconds);
        progress_date_text(a,summary.last_saved_at,date,sizeof date,time,sizeof time);
        TextBuffer u{updated,sizeof updated,0};u.text(date);if(summary.last_saved_at){
            if(a.content.english()){u.ch(' ');u.text(time);}
            else{const unsigned hour=summary.last_saved_at/3600%24,minute=summary.last_saved_at/60%60;
                u.text(hour<12?"， 上午 ":"， 下午 ");u.number(hour%12?hour%12:12);u.ch(':');if(minute<10)u.ch('0');u.number(minute);}}
        const ProfileTextValues values{i+1,playtime,updated};
        profile_text(a,"PROFILE_SCREEN.BUTTON.title",a.content.english()?"Save Profile {Id}":"{Id}号存档",values,text,sizeof text);
        profile_text_box(c,text,x+20,y+9,47,8,{6,source_label_face(a.content.english(),5),rgb(239,200,81),64,2,rgb(51,47,7),2},true,false,transform);
        profile_text(a,summary.exists?"PROFILE_SCREEN.BUTTON.description":"PROFILE_SCREEN.BUTTON.empty",
            summary.exists?"{Playtime}\n\n{LastUpdatedTime}":a.content.english()?"Empty":"空存档",values,text,sizeof text);
        profile_text_box(c,text,x+10,y+29,65,69,{5,0,rgb(255,246,226),96},true,true,transform,true);
        if(interactive)c.action(x,y,profile_layout::Width,profile_layout::Height,UiAction(unsigned(UiAction::SelectProfile1)+i));
        if(!summary.exists)continue;
        const int center=profile_layout::center(i),trash_y=profile_layout::DeleteTop;
        const bool trash_focus=interactive&&(a.pointer.visible?(a.pointer.x>=center-8&&a.pointer.x<center+7&&a.pointer.y>=trash_y&&a.pointer.y<trash_y+15):a.profile_selection==i+3);
        const int trash=a.profile_motion[i+3].observe(trash_focus,a.map_clock_ms);
        const ProfileTransform trash_transform{center,trash_y+8,4096+410*trash/4096};
        if(a.content.version()>=ProfileContentVersion){profile_image(c,ProfileDelete,center-8,trash_y,15,15,trash_transform);
            profile_image(c,ProfileDeleteFocus,center-8,trash_y,15,15,trash_transform,trash*255/4096);}
        if(interactive)c.action(center-8,trash_y,15,15,UiAction(unsigned(UiAction::DeleteProfile1)+i));
        const unsigned alpha=a.profile_labels[i].observe(trash_focus,a.map_clock_ms),age=a.map_clock_ms-a.profile_labels[i].changed;
        // NDeleteProfileButton uses cubic-out from48 to78, with no overshoot.
        const int label_y=trash_y+reference_viewport::project(trash_focus?48+30*room_curve::cubic_out(age,200)/4096:78);
        if(alpha){Canvas label=c;label.opacity=uint8_t(div255(unsigned(c.opacity)*alpha));
            profile_text_box(label,frontend_text(a,"main_menu_ui","PROFILE_SCREEN.DELETE_BUTTON.label",a.content.english()?"Delete Profile":"删除存档"),
                center-40,label_y,80,10,{5,source_label_face(a.content.english(),5),rgb(255,100,100),128},true,false,trash_transform);}
    }
    if(!a.profile_pending)native_original_back(c,a.profiles_started);
}
