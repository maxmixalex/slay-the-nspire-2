// Original scene textures and interaction flow, reflowed for 320x240.
static const char* frontend_text(const App&a,const char*table,const char*key,const char*fallback){const char*text=a.content.world_string(table,key);return *text?text:fallback;}
static const char* frontend_label(const App&a,FrontMenu item){
 static const char*keys[]={"SINGLE_PLAYER","CONTINUE","ABANDON_RUN","COMPENDIUM","SETTINGS","QUIT"};
 static const char*fallback[]={"单人游戏","继续游戏","放弃游戏","图鉴","设置","退出游戏"};
 return frontend_text(a,"main_menu_ui",keys[item],fallback[item]);
}
static void frontend_number(TextBuffer&b,const char*label,int value){
 b.text(label);RichReader reader(label);RichChar character{};uint32_t last=0;
 while(reader.next(character))last=character.codepoint;
 if(last&&last!=' '&&last!='\t'&&last!='\n')b.ch(' ');
 b.number(value);
}
// A clipped view of a full-width atlas band scales one authored sprite. The
// view shares decoded storage; no per-particle image request or bitmap exists.
static void title_particle(Canvas&c,Picture band,const title_scene::ParticleSample&p,int source_x,int source_width,unsigned mode){
 const int left=mx(0,p.x),right=mn(c.clip_w,p.x+p.width),top=mx(0,p.y),bottom=mn(c.clip_h,p.y+p.height);
 if(!p.opacity||left>=right||top>=bottom||!band.pixels)return;
 Canvas layer=c;layer.pixels+=top*c.pitch+left;
 if(layer.coverage)layer.coverage+=top*c.pitch+left;
 layer.clip_w=right-left;layer.clip_h=bottom-top;
 layer.tint_red=uint8_t(div255(unsigned(c.tint_red)*p.red));
 layer.tint_green=uint8_t(div255(unsigned(c.tint_green)*p.green));
 layer.tint_blue=uint8_t(div255(unsigned(c.tint_blue)*p.blue));
 layer.picture(band,p.x-left-source_x*p.width/source_width,p.y-top,
     int(band.w)*p.width/source_width,p.height,p.opacity,mode);
}
static void title_background(Canvas&c,bool particles=true){
 auto&a=c.a;
 if(!particles||a.content.version()<TitleEffectsContentVersion){c.ui(U_TitleBackground,0,0);return;}
 c.ui(U_TitleBackdrop,0,0);
 if(particles){
  const auto atlas=a.content.image(a.content.ui_image(U_TitleParticles));
  if(atlas.w==title_scene::ParticleAtlasWidth&&atlas.h==title_scene::ParticleAtlasHeight){
   const auto dot=Canvas::picture_rows(atlas,title_scene::DotAtlasY,title_scene::DotSize);
   for(unsigned i=0;i<title_scene::StarCount;++i)
    title_particle(c,dot,title_scene::star_sample(i,a.map_clock_ms),title_scene::DotAtlasX,title_scene::DotSize,presentation::Normal);
   Picture fog[4];for(unsigned row=0;row<4;++row)fog[row]=Canvas::picture_rows(atlas,int(row)*title_scene::FogFrameHeight,title_scene::FogFrameHeight);
   for(unsigned layer=0;layer<2;++layer)for(unsigned i=0;i<title_scene::FogCount;++i){
    auto particle=title_scene::fog_sample(layer,i,a.map_clock_ms);
    // The 320px crop concentrates many source particles in one small band.
    // Bound their composite opacity so the sea and menu remain visible.
    particle.opacity=(particle.opacity*112u+127u)/255u;
    title_particle(c,fog[particle.frame/2],particle,int(particle.frame%2)*title_scene::FogFrameWidth,
        title_scene::FogFrameWidth,presentation::Add);
   }
  }
 }
 c.ui(U_TitleForeground,0,0);
}
static bool title_source_backstop(const Canvas&c){return c.a.title&&c.a.content.has_settings_ui10_assets();}
static void title_submenu_background(Canvas&c,unsigned legacy_darkness){
 const bool source=title_source_backstop(c);title_background(c,source);
 if(source){
  auto&motion=c.a.main_menu_backdrop;motion.observe(true,c.a.map_clock_ms);
  main_menu_backdrop::apply_progress(c.pixels,c.pitch,motion.sample(c.a.map_clock_ms));
 }else c.dim(0,0,320,240,legacy_darkness);
}
static void title_logo(Canvas&c){
 const auto logo=c.a.content.image(A_Logo);
 if(logo.w!=title_scene::AtlasWidth||(logo.h!=title_scene::LegacyAtlasHeight&&logo.h!=title_scene::AtlasHeight)){c.picture(A_Logo,3,5);return;}
 if(logo.h==title_scene::AtlasHeight){
  const auto glow=title_scene::glow_sample(c.a.map_clock_ms);
  Canvas light=c;light.brightness=uint8_t(div255(unsigned(light.brightness)*glow.brightness));
  light.tint_red=64;light.tint_green=205;light.tint_blue=215;
  light.picture(Canvas::picture_rows(logo,title_scene::GlowAtlasY,title_scene::GlowHeight),
      title_scene::GlowCenterX-glow.width/2,title_scene::GlowCenterY-glow.height/2,
      glow.width,glow.height,title_scene::GlowOpacity,presentation::Add);
 }
 c.picture_region(logo,title_scene::TextX,title_scene::TextY,0,0,title_scene::TextWidth,title_scene::TextHeight);
 const unsigned frame=c.a.map_clock_ms/title_scene::FireFrameMs%title_scene::FireFrames;
 c.picture_region(logo,title_scene::FireX,title_scene::FireY,int(frame)*title_scene::FireWidth,title_scene::FireAtlasY,
     title_scene::FireWidth,title_scene::FireHeight);
}
#include "profiles_ui.inc"
static int title_label_width(Canvas&c,const char*text){
    Canvas label=c;label.font_face=0;
    return label.width(text,title_scene::MenuFont);
}
static void title_menu_labels(Canvas&c){
    auto&a=c.a;const int count=a.title_menu_count();auto&presentation=a.title_presentation;presentation.bind(count);
    int focused=-1,focus_left=0,focus_width=0,focus_top=0;
    for(int i=0;i<count;++i){
        const auto item=front_item(a,i);const char*label=frontend_label(a,item);const int y=title_scene::menu_y(count,i),height=title_scene::menu_height(count,i);
        const bool inside=a.pointer.visible&&a.pointer.x>=title_scene::MenuHitLeft&&a.pointer.x<title_scene::MenuHitLeft+title_scene::MenuHitWidth&&a.pointer.y>=y&&a.pointer.y<y+height;
        const bool selected=inside||(!a.pointer.visible&&a.menu==i);
        const bool pressed=inside&&a.pointer.held&&a.pointer.captured.kind==HitKind::Menu&&a.pointer.captured.value==i&&a.pointer.context==a.input_context();
        const auto visual=presentation.labels[item].observe(selected,pressed,a.map_clock_ms);
        const int width=title_label_width(c,label),left=title_scene::MenuCenter-width/2,center_y=title_scene::menu_center_y(count,i);
        const ProfileTransform transform{title_scene::MenuCenter,center_y,visual.scale};
        Canvas text=c;text.opacity=uint8_t(div255(unsigned(text.opacity)*visual.alpha));
        profile_text_box(text,label,left,center_y-7,width,15,{title_scene::MenuFont,0,rgb(visual.red,visual.green,visual.blue),0,1,0,0,96,1,0,0},false,false,transform);
        if(selected){focused=i;focus_left=left;focus_width=width;focus_top=center_y-4;}
        c.hit(title_scene::MenuHitLeft,y,title_scene::MenuHitWidth,height,HitKind::Menu,i);
    }
    auto&motion=presentation.reticle;motion.observe(focused,focus_left,focus_top,focus_width,a.map_clock_ms);
    const unsigned alpha=motion.alpha(a.map_clock_ms);if(!alpha)return;
    const int mark=a.content.world_image("menu","HIGHLIGHT");if(mark<0)return;
    const int travel=motion.travel(a.map_clock_ms);
    const int left=motion.left-reference_viewport::project(26+travel),right=motion.left+motion.width+reference_viewport::project(-14+travel);
    // One8px sample is shared by the mirrored right-hand TextureRect.
    uint16_t pixels[64]{};uint8_t coverage[64]{};Canvas sample{a,pixels,8,8,8,coverage};sample.picture(mark,0,0,8,8);
    const Picture image{reinterpret_cast<const uint8_t*>(pixels),coverage,nullptr,8,8,1};
    Canvas reticle=c;reticle.opacity=uint8_t(div255(unsigned(c.opacity)*alpha));
    const unsigned tint=motion.tint(a.map_clock_ms);
    reticle.tint_red=uint8_t(title_menu_motion::mix(255,239,tint));reticle.tint_green=uint8_t(title_menu_motion::mix(255,200,tint));reticle.tint_blue=uint8_t(title_menu_motion::mix(255,81,tint));
    reticle.picture_region(image,left,motion.top,0,0,8,8);reticle.picture_region(image,right,motion.top,0,0,8,8,true);
}
static void continue_saved_time(const App&a,char*out,size_t capacity){
 TextBuffer text{out,capacity,0};if(!a.saved_at){text.text("—");return;}
 const auto value=ui_calendar(a.saved_at);
 // The LCD uses an unambiguous compact date instead of a clipped weekday.
 text.number(value.year);text.ch('-');if(value.month<10)text.ch('0');text.number(value.month);
 text.ch('-');if(value.day<10)text.ch('0');text.number(value.day);text.ch(' ');
 if(value.hour<10)text.ch('0');text.number(value.hour);text.ch(':');if(value.minute<10)text.ch('0');text.number(value.minute);
}
static LocalizedValue continue_saved_value(const void*context,const char*key){
    return wt_equal(key,"LastSavedTime")?LocalizedValue{true,0,1,static_cast<const char*>(context)}:LocalizedValue{};
}
static void continue_run_info(Canvas&c){
 auto&a=c.a;const bool focused=a.has_save&&front_item(a,a.menu)==FrontContinue&&
  (!a.pointer.visible||(a.pointer.x>=title_scene::MenuHitLeft&&a.pointer.x<title_scene::MenuHitLeft+title_scene::MenuHitWidth&&
   a.pointer.y>=title_scene::menu_y(a.title_menu_count(),0)&&a.pointer.y<title_scene::menu_y(a.title_menu_count(),0)+title_scene::MenuRowHeight));
 const auto visual=a.title_presentation.run_info.observe(focused,a.map_clock_ms);if(!visual.alpha||!a.has_save)return;
 Canvas panel=c;panel.opacity=uint8_t(div255(unsigned(panel.opacity)*visual.alpha));
 constexpr int x=184,width=130;const int y=97+(visual.y+2048)/4096,height=a.saved_ascension?107:94;
 const int background=a.content.world_image("menu","CONTINUE_PANEL");
 if(background>=0)panel.nine_image(background,x,y,width,height,2,2,2,2);else panel.dim(x,y,width,height,176);
 const ProfileTextStyle style{8,0,rgb(255,246,226),40,0,0,0,112,1,1};
 char row[192]{},saved_time[48]{};continue_saved_time(a,saved_time,sizeof saved_time);
 LocalizedTemplate date(row,sizeof row,continue_saved_value,saved_time);date.for_language(a.content.language()).format(frontend_text(a,"main_menu_ui","CONTINUE_RUN_INFO.saved","{LastSavedTime}"));
 profile_text_box(panel,row,x+7,y+6,116,27,style,false,true,{},true,true);
 int portrait=a.content.ui_image(U_TopBarIronclad+int(a.saved_character));if(portrait<0)portrait=a.content.ui_image(character_portrait(a.saved_character));
 panel.picture(portrait,x+7,y+39,11,11);TextBuffer b{row,sizeof row,0,&a.content};row[0]=0;
 if(a.saved_world){const char*act=a.content.world_title("acts",map_colors(a.saved_current_region).region);if(*act){b.text(act);b.ch(' ');}}
 b.text("[blue]");frontend_number(b,frontend_text(a,"main_menu_ui","CONTINUE_RUN_INFO.floor",a.content.language()==2?"楼层":"Floor"),a.saved_floor);b.text("[/blue]");
 profile_text_box(panel,row,x+23,y+37,100,24,style,false,true,{},true,true);
 if(a.saved_ascension){row[0]=0;b.n=0;frontend_number(b,frontend_text(a,"main_menu_ui","CONTINUE_RUN_INFO.ascension",a.content.language()==2?"进阶":"Ascension"),a.saved_ascension);
  profile_text_box(panel,row,x+7,y+64,116,12,style,false,false);}
 const int bottom=a.saved_ascension?84:70;row[0]=0;b.n=0;b.text("[red]");b.number(a.saved_hp);b.ch('/');b.number(a.saved_max_hp);b.text("[/red]");
 panel.picture(A_Heart,x+7,y+bottom,11,11);profile_text_box(panel,row,x+22,y+bottom,48,12,style,false,false);
 row[0]=0;b.n=0;b.text("[gold]");b.number(a.saved_gold);b.text("[/gold]");panel.picture(A_Coin,x+76,y+bottom,11,11);profile_text_box(panel,row,x+91,y+bottom,33,12,style,false,false);
}
static void title_screen(Canvas&c){
    auto&a=c.a;
    if(a.settings_open&&title_source_backstop(c)){title_submenu_background(c,205);return;}
    if(a.has_save&&a.content.version()>=Ui12ContentVersion&&!a.character_departure.active()){
        const int mask=a.content.ui_image(I12_CharacterTransitionFirst+(a.saved_character==Character::Silent?1:a.saved_character==Character::Necrobinder?2:0));
        a.content.prepare_images(&mask,1);(void)a.content.image(mask);
    }
    title_background(c);
    if(title_source_backstop(c)){a.main_menu_backdrop.observe(false,a.map_clock_ms);
        main_menu_backdrop::apply_progress(c.pixels,c.pitch,a.main_menu_backdrop.sample(a.map_clock_ms));}
    title_logo(c);profile_title_button(c);
    title_menu_labels(c);continue_run_info(c);
    c.text("nSpire II 0.17",235,229,8,White,83);
}
static void native_pause_screen(Canvas&c){
 using namespace pause_menu_layout;auto&a=c.a;a.hit_count=0;
 if(!a.pause_visible){a.pause_visible=true;a.pause_opened=a.map_clock_ms;}
 // run.tscn's shared OverlayBackstop is black at .85, independent of
 // the unused blue Backstop child of pause_menu.tscn.
 Canvas backstop=c;backstop.opacity=uint8_t(217*room_curve::cubic_out(a.map_clock_ms-a.pause_opened,500)/4096);backstop.rect(0,0,320,240,0);
 Canvas title=c;title.font_face=uint8_t(source_label_face(a.content.original_label_fonts(),4));
 title.text_fitted(frontend_text(a,"gameplay_ui","PAUSE_MENU.PAUSED","暂停"),0,TitleY,320,TitleFont,rgb(239,200,81),TitleFont,true,true,0);
 const char*keys[]={"PAUSE_MENU.RESUME","PAUSE_MENU.SETTINGS","PAUSE_MENU.COMPENDIUM","PAUSE_MENU.GIVE_UP","PAUSE_MENU.SAVE_AND_QUIT"};
 const char*fallback[]={"继续游戏","设置","图鉴","放弃游戏","保存并返回主菜单"};
 for(int i=0;i<5;i++){const int y=row_y(unsigned(i));const bool enabled=i!=3||resumable_run(a.game),selected=a.menu==i;
  const char*label=frontend_text(a,"gameplay_ui",keys[i],fallback[i]);
  const auto visual=c.control_visual(label,X,y,W,H,ButtonFamily::OriginalPause,selected,enabled,0,i!=0);
  constexpr int PW=224,PH=40;const int w=mn(PW,mx(1,W*visual.scale/4096)),h=mn(PH,mx(1,H*visual.scale/4096)),left=X+(W-w)/2,top=y+(H-h)/2+(visual.y+2048)/4096;
  // Reuse the original untinted reward plaque and its bounded cache. The
  // Give Up material's h=.5 negates YIQ chroma, without a replacement skin.
  uint16_t pixels[PW*PH];uint8_t alpha[PW*PH]{};Canvas tile{a,pixels,PW,w,h,alpha};tile.ui(U_RewardButton,0,0,w,h);
  for(int yy=0;yy<h;++yy)for(int xx=0;xx<w;++xx){const int at=yy*PW+xx;if(alpha[at])c.paint((top+yy)*c.pitch+left+xx,presentation::yiq_material(pixels[at],i==3?-int(visual.saturation):int(visual.saturation),visual.value,visual.shade),alpha[at]);}
  Canvas text=c;text.font_face=uint8_t(source_label_face(a.content.original_label_fonts(),5));if(!enabled)text.brightness=128;
  const int label_y=y+(visual.y+2048)/4096;
  const uint16_t ink=blend(rgb(255,246,226),rgb(239,200,81),unsigned(visual.hover)*255/4096);
  Canvas shadow=text;shadow.opacity=uint8_t(div255(text.opacity*32));shadow.fitted(label,X+7,label_y+2,W-12,H-2,Font,Font,0,true);
  text.fitted(label,X+6,label_y+1,W-12,H-2,Font,Font,ink,true,false,true,false,i==3?rgb(92,50,37):rgb(37,84,92));
  c.hit(X,y,W,H,HitKind::Menu,i,enabled);
 }
 native_original_back(c,a.pause_opened);
}
#include "settings_ui10.inc"
#include "frontend_popup_ui10.inc"
static void frontend_confirm_screen(Canvas&c){
 auto&a=c.a;const bool quitting=a.frontend_confirm==3,reset=a.frontend_confirm==4,erase_profile=a.frontend_confirm==5;a.hit_count=0;
 c.dim(0,0,320,240,a.content.version()>=Ui6ContentVersion?a.popup_backdrop.sample(a.map_clock_ms):210);
 const char*header=frontend_text(a,reset?"settings_ui":"main_menu_ui",reset?"RESET_CONFIRMATION.header":quitting?"QUIT_CONFIRM_POPUP.header":"ABANDON_RUN_CONFIRMATION.header",reset?"重置设置":quitting?"退出游戏":"你确定吗？");
 const char*body=frontend_text(a,reset?"settings_ui":"main_menu_ui",reset?"RESET_GAMEPLAY_CONFIRMATION.body":quitting?"QUIT_CONFIRM_POPUP.body":"ABANDON_RUN_CONFIRMATION.body",reset?"恢复默认设置？":quitting?"你确定要退出吗？":"放弃游戏会被视为本局失败。");
 char profile_header[128]{},profile_body[1024]{};
 if(erase_profile){profile_text(a,"PROFILE_SCREEN.DELETE_CONFIRM_POPUP.title",a.content.language()!=2?"Delete Save Profile {Id}?":"是否删除{Id}号存档？",{a.profile_target},profile_header,sizeof profile_header);
  profile_text(a,"PROFILE_SCREEN.DELETE_CONFIRM_POPUP.description",a.content.language()!=2?"All data in Save Profile {Id} will be deleted. This cannot be undone.":"{Id}号存档内的所有数据将被删除。这一操作无法逆转。",{a.profile_target},profile_body,sizeof profile_body);header=profile_header;body=profile_body;}
 if(a.content.version()>=Ui6ContentVersion){
  native_frontend_popup(c,header,body,erase_profile);
 }else{
  c.ui(U_RewardPanel,29,43,262,165);c.centered(header,41,63,238,16,Gold);c.paragraph_fitted(body,51,98,218,45,12,White,12);
  c.button(frontend_text(a,"main_menu_ui","GENERIC_POPUP.cancel","取消"),39,161,109,!a.frontend_choice,31);
  c.button(frontend_text(a,"main_menu_ui","GENERIC_POPUP.confirm","确认"),172,161,109,a.frontend_choice,31);
  c.action(36,151,115,46,UiAction::FrontendCancel);c.action(169,151,115,46,UiAction::FrontendAccept);
 }
}
#include "character_select_ui7.inc"
#include "card_library_ui.inc"
#include "collection_ui.inc"

#include "item_inspect_ui.inc"
