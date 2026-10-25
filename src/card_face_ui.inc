// The original unknown portrait is a separable Gaussian with a 32-texel
// radius. At the library's 53/67px card sizes its projected support is three
// pixels; a normalized seven-tap kernel keeps that blur local and bounded.
static void library_blurred_portrait(Canvas&c,int image,const card_face::Rect&r){
    if(r.w<=0||r.h<=0||r.w>80||r.h>80)return;
    uint16_t pixels[80*80]{},line[80];Canvas tile=c;tile.pixels=pixels;tile.pitch=r.w;tile.clip_w=r.w;tile.clip_h=r.h;
    tile.coverage=nullptr;tile.opacity=tile.brightness=tile.tint_red=tile.tint_green=tile.tint_blue=255;
    tile.picture(image,0,0,r.w,r.h);
    static constexpr unsigned weights[]={1,6,15,20,15,6,1};
    auto sample=[&](int at,int size){unsigned red=0,green=0,blue=0;
        for(int k=-3;k<=3;++k){const uint16_t color=line[mn(size-1,mx(0,at+k))];const unsigned w=weights[k+3];red+=(color>>11)*w;green+=((color>>5)&63)*w;blue+=(color&31)*w;}
        return uint16_t(((red+32)/64<<11)|((green+32)/64<<5)|(blue+32)/64);
    };
    for(int y=0;y<r.h;++y){for(int x=0;x<r.w;++x)line[x]=pixels[y*r.w+x];for(int x=0;x<r.w;++x)pixels[y*r.w+x]=sample(x,r.w);}
    for(int x=0;x<r.w;++x){for(int y=0;y<r.h;++y)line[y]=pixels[y*r.w+x];for(int y=0;y<r.h;++y)pixels[y*r.w+x]=sample(y,r.h);}
    c.picture(Picture{reinterpret_cast<const uint8_t*>(pixels),nullptr,nullptr,uint16_t(r.w),uint16_t(r.h),0},r.x,r.y);
}
struct CardFaceFonts {int title,type,body,body_minimum,energy,star,minimum;bool compact;};
// NCard and CardCostHelper distinguish modified costs from an inability to
// pay. Keep this in the face key: a player's resources can change while the
// card's numeric price and overall playability both remain unchanged.
enum CardResourceStyle : uint8_t {ResourceNormal,ResourceIncreased,ResourceDecreased,ResourceInsufficient};
static bool card_face_is_hand(const App&a,Card card,int instance){
    return !a.title&&!a.library&&!a.history_open&&instance>=0&&instance<a.game.card_count&&
        a.game.zones[instance]==Zone::Hand&&a.game.cards[instance].kind==card.kind;
}
static uint8_t card_face_resource_styles(const App&a,Card card,int energy,int stars,int instance,const EventCardFacts*facts,bool upgrade_preview=false){
    const auto style=[](int value,int base,int available){return uint8_t(value<0?ResourceNormal:value>available?ResourceInsufficient:base>=0&&value>base?ResourceIncreased:base>=0&&value<base?ResourceDecreased:ResourceNormal);};
    const bool pretend=a.game.phase==Phase::ExtendedChoice&&a.game.expansion.choice==XChoice::RDecisions&&!a.choice_peeking;
    uint8_t result=card_face_is_hand(a,card,instance)?uint8_t(style(energy,a.card_offer_cost(card,facts),pretend?9999:a.game.energy)|(style(stars,card_star_cost(a.game,card),pretend?9999:a.game.expansion.stars)<<2)):0;
    // NCard checks WasJustUpgraded before hand affordability. That flag is
    // cleared when an actual upgrade is finalized: only a preview copy whose
    // own cost changed receives the green value and matching green outline.
    if(upgrade_preview&&card.upgraded){const Card before{card.kind,0};
        const int base_energy=a.card_offer_cost(before,facts),upgraded_energy=a.card_offer_cost(card,facts);
        if(energy>=0&&base_energy>=0&&upgraded_energy>=0&&base_energy!=upgraded_energy)result=uint8_t((result&12)|ResourceDecreased);
        const int base_stars=card_star_cost(a.game,before),upgraded_stars=card_star_cost(a.game,card);
        if(stars>=0&&base_stars>=0&&upgraded_stars>=0&&base_stars!=upgraded_stars)result=uint8_t((result&3)|(ResourceDecreased<<2));
    }
    return result;
}
static bool card_face_resource_cross(const App&a,Card card,int energy,int stars,int instance){
    // Source HasResourceCostReason is energy OR stars: either shortage
    // suppresses both crosses even when another card rule also blocks play.
    if(a.game.phase==Phase::ExtendedChoice&&a.game.expansion.choice==XChoice::RDecisions&&!a.choice_peeking)return false;
    return card_face_is_hand(a,card,instance)&&energy<=a.game.energy&&stars<=a.game.expansion.stars&&
        card_has_nonresource_play_block(a.game,instance);
}
static void card_resource_cross(Canvas&c,int x,int y,int w,int h,bool star){
    const int image=c.a.content.version()>=CardResourceContentVersion?c.a.content.ui_image(I9_CardUnplayable):-1;
    if(image<0)return;
    // Both children are48px square: EnergyIcon+(8,8), StarIcon+(5,5).
    const auto rect=card_face::region(x,y,w,h,star?-31:-8,star?27:-8,48,48);
    c.picture(image,rect.x,rect.y,rect.w,rect.h);
}
static uint16_t card_energy_outline(Card card){
    if(definitions[card.kind].type==CardType::Quest)return rgb(67,30,20);
    switch(card_color(card.kind)){
    case CardColor::Ironclad:return rgb(128,32,32);case CardColor::Silent:return rgb(26,102,37);
    case CardColor::Defect:return rgb(29,86,115);case CardColor::Regent:return rgb(128,61,14);
    case CardColor::Necrobinder:return rgb(128,51,103);default:return rgb(92,84,64);}
}
static uint16_t card_title_outline(Card card,bool known){
    if(known&&card.upgraded)return rgb(27,97,49);
    switch(card_rarity(card.kind)){case 3:return rgb(0,92,117);case 4:return rgb(107,75,0);
    case 9:return rgb(85,11,158);case 10:return rgb(126,62,21);case 8:return rgb(79,82,47);
    case 6:return rgb(27,97,49);default:return rgb(77,75,64);}
}
static void card_resource_label(Canvas&c,const char*text,int x,int y,int size,unsigned style,uint16_t default_outline,bool star){
    uint16_t color=rgb(255,246,226),outline=default_outline;
    if(style==ResourceIncreased){color=rgb(64,255,255);outline=rgb(32,89,92);}
    else if(style==ResourceDecreased){color=rgb(127,255,0);outline=rgb(31,89,35);}
    else if(style==ResourceInsufficient){color=rgb(255,85,85);outline=rgb(80,23,23);}
    Canvas edge=c;if(star&&style==ResourceNormal)edge.opacity=uint8_t(div255(edge.opacity*220));
    // A cost has a colored stroke, not a lower-right drop shadow. Keep a
    // thin LCD pixel on all four sides without the heavy diagonal halo.
    edge.opacity=uint8_t(div255(unsigned(edge.opacity)*208));
    edge.text(text,x-1,y,size,outline);edge.text(text,x+1,y,size,outline);
    edge.text(text,x,y-1,size,outline);edge.text(text,x,y+1,size,outline);
    c.text(text,x,y,size,color);
}
static CardFaceFonts card_face_fonts(const Canvas&c,int width){
    const auto&content=c.a.content;
    const bool compact=content.version()>=HistoryUi8ContentVersion&&content.font_size(5)==5;
    if(!compact)return {width>=100?12:width>=70?10:8,8,width>=100?10:8,8,width>=100?12:width>=70?10:8,8,8,false};
    // A 5px glyph cannot carry a readable card description on this LCD.
    // Keep source artwork proportions; text uses a native 8px floor and
    // complete descriptions remain available in the paged hover/inspector.
    const int readable=width>=80?10:8;
    return {content.font_size(mx(readable,card_face::font_pixels(26,width))),content.font_size(width>=80?10:8,source_label_face(content.original_label_fonts(),5)),
        content.font_size(mx(readable,card_face::font_pixels(21,width))),content.font_size(readable),
        content.font_size(mx(width>=80?12:10,card_face::font_pixels(32,width)),5),content.font_size(mx(8,card_face::font_pixels(22,width)),5),8,true};
}
// The selected hand face can expose every description line within the same
// authored rectangle. Ordinary cards keep the static fitted/ellipsis path.
static void card_description_window(Canvas&c,const char*text,const card_face::Rect&r,int size,int skip){
 if(r.w<=0||r.h<=2)return;
 size=c.a.content.font_size(size,c.font_face);const int visible=mn(32,fitted_text::row_capacity(r.h,size));if(!visible)return;
 char localized[1024]{};text=c.a.ui_text(text,localized,sizeof localized);
 const auto metric=[&](const RichChar&ch,int px){return c.text_metric(ch,px);};
 fitted_text::Plan plan{};plan.size=size;plan.step=size+1;plan.rows=plan.total=fitted_text::line_count(text,r.w,size,metric);plan.truncated=false;
 int widths[32]{};fitted_text::visit(text,r.w,plan,metric,[&](const RichChar&ch,int at,int row,int px){if(row>=skip&&row<skip+visible)widths[row-skip]=at+metric(ch,px).width;});
 Canvas body=c;body.pixels+=r.y*body.pitch+r.x;if(body.coverage)body.coverage+=r.y*body.pitch+r.x;
 body.clip_w=mn(r.w,c.clip_w-r.x);body.clip_h=mn(r.h,c.clip_h-r.y);if(body.clip_w<=0||body.clip_h<=0)return;
 const uint16_t colors[]={White,Gold,rgb(244,122,102),Blue,Green,rgb(190,145,217),rgb(236,164,92),rgb(104,210,205),White,Muted};
 fitted_text::visit(text,r.w,plan,metric,[&](const RichChar&ch,int at,int row,int px){
  if(row<skip||row>=skip+visible)return;
  const auto glyph=c.a.content.glyph(ch.codepoint,px,c.font_style(ch));
  body.character(ch.codepoint,glyph,at+mx(0,(r.w-widths[row-skip])/2),(row-skip)*plan.step,rich_color(ch.style.color,colors));
 });
}
// One scene-based card renderer for every presentation size. Individual
// screens supply instance-aware costs/descriptions, never rearranged art.
static void render_card_face(Canvas&c,Card card,int x,int y,int w,int h,int fee=-2,int stars=-2,int instance=-1,int deck=-1,const EventCardFacts*facts=nullptr,const char*description=nullptr,bool*description_truncated=nullptr,bool known=true,bool upgrade_preview=false,int description_skip=-1){
    auto&a=c.a;if(!facts)facts=event_card_facts(a.game,card,instance,deck);
    const auto type=a.card_face_type(card,instance,deck,facts);
    const unsigned group=card_face::material_group(unsigned(card_rarity(card.kind)));
    const int base=U_CardBorderCommonAttack+int(group)*5;
    const int border=base+(type==CardType::Attack?0:type==CardType::Power?2:1);
    auto l=card_face::layout(x,y,w,h);const auto fonts=card_face_fonts(c,w);
    // Portrait precedes Frame and PortraitBorder in the original scene.
    // Drawing it last used to cover the frame's lower edge and description.
    if(known)c.picture(a.card_face_image(card,instance,deck,facts),l.portrait.x,l.portrait.y,l.portrait.w,l.portrait.h);
    else library_blurred_portrait(c,a.card_face_image(card,instance,deck,facts),l.portrait);
    c.picture(a.card_face_frame(card,instance,deck,facts),x,y,w,h);
    c.ui(border,l.border.x,l.border.y,l.border.w,l.border.h);
    int banner=a.content.ui_image(base+4);
    if(banner<0)banner=a.content.ui_image(group==2?U_RareBanner:group==1?U_BigBanner:U_CommonBanner);
    c.picture(banner,l.banner.x,l.banner.y,l.banner.w,l.banner.h);
    char title[160]{};TextBuffer name{title,sizeof title,0,&a.content};name.text(known?a.content.card_string(card.kind):a.content.locale_string("card_library","UNKNOWN.title"));if(known&&card.upgraded)name.ch('+');
    const int title_size=fonts.title;
    const int title_y=l.title.y+mx(0,(l.title.h-title_size-4)/2)-mx(1,h/100);
    Canvas title_label=c;title_label.font_face=uint8_t(source_label_face(a.content.original_label_fonts(),5));
    title_label.text_fitted(title,l.title.x,title_y,l.title.w,title_size,known&&card.upgraded?rgb(127,255,0):rgb(255,246,226),fonts.minimum,true,true,card_title_outline(card,known));

    const char*type_text=card_type_label(type,&a.content);Canvas type_label=c;if(fonts.compact)type_label.font_face=uint8_t(source_label_face(a.content.original_label_fonts(),5));
    const int type_size=fonts.type,plaque_h=fonts.compact?mx(type_size+3,l.type.h):mx(11,l.type.h);
    const int padding=fonts.compact?mx(2,card_face::scaled(17,w,300)):5;
    // Leave two pixels for the last glyph's ink beyond its advance. A
    // perfectly measured "Attack" must not be shortened to "Atta...".
    const int plaque_w=mn(w-6,mx(l.type.w,type_label.width(type_text,type_size)+padding+2));
    const int plaque_x=x+(w-plaque_w)/2,plaque_y=fonts.compact?l.type.y+(l.type.h-plaque_h)/2:l.type.y;
    int plaque=a.content.ui_image(base+3);
    if(plaque>=0)c.nine_image(plaque,plaque_x,plaque_y,plaque_w,plaque_h,4,0,4,0);
    if(fonts.compact)type_label.opacity=uint8_t(div255(type_label.opacity*192));
    type_label.text_fitted(type_text,plaque_x+padding/2,plaque_y+(plaque_h-type_size-3)/2,plaque_w-padding,type_size,fonts.compact?uint16_t(0):White,fonts.minimum,true);

    char body[1024]{};
    if(!known)description=a.content.locale_string("card_library","UNKNOWN.description");
    if(!description){
        if(facts)a.world_card_description(card,-1,body,sizeof body,instance,deck,facts);
        else a.card_description(card,-1,body,sizeof body,instance,deck);
        description=body;
    }
    // Legacy packs keep their available8px layout. Native small-font cards
    // retain the full authored136px description rectangle; the type label
    // no longer consumes its first lines. Unknown descriptions use source40.
    if(!fonts.compact){const int bottom=l.description.y+l.description.h;
        l.description.y=mx(l.description.y,l.type.y+plaque_h);l.description.h=mx(0,bottom-l.description.y);}
    const int body_size=!known&&fonts.compact?a.content.font_size(card_face::font_pixels(40,w)):fonts.body;
    if(description_skip>=0)card_description_window(c,description,l.description,body_size,description_skip);
    else{const auto description_layout=c.paragraph_fitted(description,l.description.x,l.description.y,l.description.w,l.description.h,body_size,White,fonts.body_minimum,true);
        if(description_truncated)*description_truncated=description_layout.truncated;}

    c.picture(a.content.energy_image(card,!known),l.energy.x,l.energy.y,l.energy.w,l.energy.h);
    const int energy=fee==-2?a.card_offer_cost(card,facts):fee;
    const int base_stars=card_star_cost(a.game,card),star_value=stars==-2?base_stars:stars;
    const unsigned resource_styles=known?card_face_resource_styles(a,card,energy,star_value,instance,facts,upgrade_preview):0;
    const int cost_size=fonts.energy;Canvas cost_label=c;if(fonts.compact)cost_label.font_face=5;
    char cost_text[16]{};TextBuffer cost_value{cost_text,sizeof cost_text,0,&a.content};if(!known)cost_value.ch('?');else if(energy==-1)cost_value.ch('X');else cost_value.number(energy);
    if(!known||energy!=-2)card_resource_label(cost_label,cost_text,l.energy.x+(l.energy.w-cost_label.width(cost_text,cost_size))/2,l.energy.y+(l.energy.h-cost_size-2)/2,cost_size,resource_styles&3,card_energy_outline(card),false);
    if(known&&(base_stars||(stars!=-2&&stars))){
        c.ui(U_Star,l.star.x,l.star.y,l.star.w,l.star.h);
        const int value=stars==-2?base_stars:stars;char label[16]{};TextBuffer count{label,sizeof label,0,&a.content};if(value<0)count.ch('X');else count.number(value);
        const int px=l.star.x+(l.star.w-cost_label.width(label,fonts.star))/2,py=l.star.y+(l.star.h-fonts.star-2)/2;
        card_resource_label(cost_label,label,px,py,fonts.star,resource_styles>>2,rgb(23,85,97),true);
    }
    if(known&&card_face_resource_cross(a,card,energy,star_value,instance)){
        if(a.content.energy_image(card)>=0)card_resource_cross(c,x,y,w,h,false);
        if(base_stars||(stars!=-2&&stars))card_resource_cross(c,x,y,w,h,true);
    }
}

namespace {
// Small-card grids repaint the same eight/ten faces at 30Hz. Store the exact
// destination before and after composition, rather than flattening translucent
// layers and changing RGB565 rounding. A cache hit compares every background
// pixel, so moving actors, selection highlights and overlapping cards remain
// exact. Eight protected slots cover a full library page and eight of ten
// selector cards without a cyclic LRU miss storm.
constexpr int SmallCardCacheW=84,SmallCardCacheH=108,SmallCardCacheCount=8;
struct SmallCardFaceCache {
    const Content* owner=nullptr;
    uint32_t generation=0,seen=0;
    Card card{};
    int portrait=-1,frame=-1,energy_image=-1,energy=0,stars=0;
    CardType type=CardType::Attack;
    uint8_t width=0,height=0,resource_styles=0,offset_x=0,offset_y=0;
    bool star_visible=false,resource_cross=false,valid=false;
    char title[128]{},description[512]{};
    uint16_t background[SmallCardCacheW*SmallCardCacheH]{};
    uint16_t pixels[SmallCardCacheW*SmallCardCacheH]{};
};
static SmallCardFaceCache small_card_faces[SmallCardCacheCount];
static uint32_t small_card_frame=0;
static_assert(sizeof small_card_faces+sizeof small_card_frame<=300u*1024u,
    "Small-card cache must fit its bounded 300KiB ARM allowance");
static bool card_cache_text_equal(const char*a,const char*b){while(*a&&*a==*b){++a;++b;}return *a==*b;}
static bool card_cache_text_fits(const char*s,unsigned capacity){unsigned n=0;while(s[n])if(++n>=capacity)return false;return true;}
static void card_cache_text_copy(char*out,const char*in){while((*out++=*in++)){};}
static bool card_cache_static_text(const char*s){
    RichReader reader(s);RichChar ch{};
    while(reader.next(ch))if(ch.style.wave||ch.style.jitter||ch.style.rainbow||ch.style.thinky_dots)return false;
    return true;
}
static bool card_cache_label_fits(Canvas&c,card_face::Rect icon,int value,bool star,int left,int top,int size,unsigned face){
    if(!star&&value==-2)return true;
    char label[16]{};TextBuffer text{label,sizeof label,0};if(star?value<0:value==-1)text.ch('X');else text.number(value);
    Canvas text_layer=c;text_layer.font_face=uint8_t(face);
    const int origin=icon.x+(icon.w-text_layer.width(label,size))/2,y=icon.y+(icon.h-size-2)/2;
    int pen64=0;uint32_t previous=0;
    for(const char*p=label;*p;){const uint32_t cp=utf8(p);const auto g=c.a.content.glyph_metrics(cp,size,face);
        pen64+=c.a.content.glyph_kerning64(previous,cp,size,face);previous=cp;
        const int x=origin+text_pixel(pen64)+g.bearing_x;
        if(g.w&&g.h&&(x-1<left||y-1<top||x+int(g.w)+1>left+SmallCardCacheW||y+int(g.h)+1>top+SmallCardCacheH))return false;
        pen64+=g.advance64?g.advance64:int(g.advance)*64;}
    return true;
}
}
static void invalidate_card_face_cache(){for(auto&entry:small_card_faces)entry.valid=false;}
static void begin_card_face_frame(){if(++small_card_frame==0){invalidate_card_face_cache();++small_card_frame;}}
static void draw_card_face(Canvas&c,Card card,int x,int y,int w,int h,int fee=-2,int stars=-2,int instance=-1,int deck=-1,const EventCardFacts*facts=nullptr,const char*description=nullptr,bool*description_truncated=nullptr,bool known=true,bool upgrade_preview=false){
    // NCard.Model marks a local run/combat model when it is bound. Canonical
    // collection/history/title previews do not constitute such a binding.
    // This is an O(1) first-observation bit check, never a catalog scan.
    if(known&&!c.a.library&&!c.a.history_open&&!c.a.title&&c.a.game.world.active&&!c.a.game.practice)c.a.discover_card(card.kind);
    // Keep the fixed cache window inside the destination, including the
    // rightmost merchant slot. Its card fits on the LCD, but the usual six
    // pixels of left padding placed the old 84px window five pixels outside.
    // The face's offset within this window is part of its identity: the same
    // card on a plain background must not replay a differently padded copy.
    const int left=mn(mx(0,x-6),c.clip_w-SmallCardCacheW);
    const int top=mn(mx(0,y-4),c.clip_h-SmallCardCacheH);
    const bool supported_size=(w==65&&h==91)||(w==69&&h==96)||(w==69&&h==97)||(w==68&&h==96)||(w==46&&h==64)||(w==47&&h==66)||(w==43&&h==61)||(w==36&&h==50)||(w==44&&h==62);
    const bool candidate=known&&!description_truncated&&c.a.content.healthy()&&supported_size&&!c.coverage&&c.opacity==255&&c.brightness==255&&
        c.tint_red==255&&c.tint_green==255&&c.tint_blue==255&&!c.color_matrix&&left>=0&&top>=0&&
        x>=left&&y>=top&&x+w<=left+SmallCardCacheW&&y+h<=top+SmallCardCacheH;
    if(!candidate){render_card_face(c,card,x,y,w,h,fee,stars,instance,deck,facts,description,description_truncated,known,upgrade_preview);return;}
    auto&a=c.a;if(!facts)facts=event_card_facts(a.game,card,instance,deck);
    char body[1024]{},localized[1024]{};
    if(!description){
        if(facts)a.world_card_description(card,-1,body,sizeof body,instance,deck,facts);
        else a.card_description(card,-1,body,sizeof body,instance,deck);
        description=body;
    }
    description=a.ui_text(description,localized,sizeof localized);
    char title[160]{};TextBuffer name{title,sizeof title,0,&a.content};name.text(a.content.card_string(card.kind));if(card.upgraded)name.ch('+');
    if(!card_cache_text_fits(title,sizeof small_card_faces[0].title)||
       !card_cache_text_fits(description,sizeof small_card_faces[0].description)||
       !card_cache_static_text(title)||!card_cache_static_text(description)){
        render_card_face(c,card,x,y,w,h,fee,stars,instance,deck,facts,description,nullptr,known,upgrade_preview);return;
    }
    const int portrait=a.card_face_image(card,instance,deck,facts),frame=a.card_face_frame(card,instance,deck,facts);
    const auto type=a.card_face_type(card,instance,deck,facts);
    const int energy_image=a.content.energy_image(card),energy=fee==-2?a.card_offer_cost(card,facts):fee;
    const int base_stars=card_star_cost(a.game,card),star_count=stars==-2?base_stars:stars;
    const bool star_visible=base_stars||(stars!=-2&&stars);
    const unsigned resource_styles=card_face_resource_styles(a,card,energy,star_count,instance,facts,upgrade_preview);
    const bool resource_cross=card_face_resource_cross(a,card,energy,star_count,instance);
    const auto geometry=card_face::layout(x,y,w,h);const auto fonts=card_face_fonts(c,w);
    // Large dynamic costs can extend beyond the fixed cache, just as their
    // uncached labels do. Keep those exact without increasing the ARM pool.
    if(!a.content.healthy()||!card_cache_label_fits(c,geometry.energy,energy,false,left,top,fonts.energy,fonts.compact?5u:c.font_face)||
       (star_visible&&!card_cache_label_fits(c,geometry.star,star_count,true,left,top,fonts.star,fonts.compact?5u:c.font_face))){
        render_card_face(c,card,x,y,w,h,fee,stars,instance,deck,facts,description,nullptr,known,upgrade_preview);return;}
    SmallCardFaceCache* entry=nullptr;
    for(auto&slot:small_card_faces)if(slot.valid&&slot.owner==&a.content&&slot.generation==a.content.generation()&&slot.width==w&&slot.height==h&&
        slot.offset_x==x-left&&slot.offset_y==y-top&&
        slot.card.kind==card.kind&&slot.card.upgraded==card.upgraded&&slot.portrait==portrait&&slot.frame==frame&&slot.type==type&&
        slot.energy_image==energy_image&&slot.energy==energy&&slot.stars==star_count&&slot.star_visible==star_visible&&slot.resource_styles==resource_styles&&slot.resource_cross==resource_cross&&
        card_cache_text_equal(slot.title,title)&&card_cache_text_equal(slot.description,description)){
        bool matches=true;
        for(int yy=0;yy<SmallCardCacheH&&matches;++yy)for(int xx=0;xx<SmallCardCacheW;++xx)
            if(slot.background[yy*SmallCardCacheW+xx]!=c.pixels[(top+yy)*c.pitch+left+xx]){matches=false;break;}
        if(matches){
            slot.seen=small_card_frame;
            for(int yy=0;yy<SmallCardCacheH;++yy)for(int xx=0;xx<SmallCardCacheW;++xx)
                c.pixels[(top+yy)*c.pitch+left+xx]=slot.pixels[yy*SmallCardCacheW+xx];
            return;
        }
        // A matching face with a changed backdrop may be rebuilt unless
        // another occurrence has already used its slot in this same frame.
        if(slot.seen!=small_card_frame)entry=&slot;
    }
    if(!entry)for(auto&slot:small_card_faces)if(!slot.valid||slot.seen!=small_card_frame){
        if(!entry||!slot.valid||slot.seen<entry->seen)entry=&slot;
        if(!slot.valid)break;
    }
    if(!entry){render_card_face(c,card,x,y,w,h,fee,stars,instance,deck,facts,description,nullptr,known,upgrade_preview);return;}
    entry->valid=false;entry->owner=&a.content;entry->generation=a.content.generation();entry->seen=small_card_frame;
    entry->card=card;entry->portrait=portrait;entry->frame=frame;entry->type=type;entry->width=uint8_t(w);entry->height=uint8_t(h);
    entry->offset_x=uint8_t(x-left);entry->offset_y=uint8_t(y-top);
    entry->energy_image=energy_image;entry->energy=energy;entry->stars=star_count;entry->star_visible=star_visible;entry->resource_cross=resource_cross;
    entry->resource_styles=uint8_t(resource_styles);
    card_cache_text_copy(entry->title,title);card_cache_text_copy(entry->description,description);
    for(int yy=0;yy<SmallCardCacheH;++yy)for(int xx=0;xx<SmallCardCacheW;++xx)
        entry->background[yy*SmallCardCacheW+xx]=c.pixels[(top+yy)*c.pitch+left+xx];
    render_card_face(c,card,x,y,w,h,fee,stars,instance,deck,facts,description,nullptr,known,upgrade_preview);
    for(int yy=0;yy<SmallCardCacheH;++yy)for(int xx=0;xx<SmallCardCacheW;++xx)
        entry->pixels[yy*SmallCardCacheW+xx]=c.pixels[(top+yy)*c.pitch+left+xx];
    entry->valid=a.content.healthy();
}
