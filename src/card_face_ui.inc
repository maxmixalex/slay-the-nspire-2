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
static CardFaceFonts card_face_fonts(const Canvas&c,int width){
    const auto&content=c.a.content;
    const bool compact=content.version()>=HistoryUi8ContentVersion&&content.font_size(5)==5;
    if(!compact)return {width>=100?12:width>=70?10:8,8,width>=100?10:8,8,width>=100?12:width>=70?10:8,8,8,false};
    return {content.font_size(card_face::font_pixels(26,width)),content.font_size(card_face::font_pixels(16,width),5),
        content.font_size(card_face::font_pixels(21,width)),content.font_size(card_face::font_pixels(12,width)),
        content.font_size(card_face::font_pixels(32,width),5),content.font_size(card_face::font_pixels(22,width),5),5,true};
}
// One scene-based card renderer for every presentation size. Individual
// screens supply instance-aware costs/descriptions, never rearranged art.
static void render_card_face(Canvas&c,Card card,int x,int y,int w,int h,int fee=-2,int stars=-2,int instance=-1,int deck=-1,const EventCardFacts*facts=nullptr,const char*description=nullptr,bool*description_truncated=nullptr,bool known=true){
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
    const int title_y=l.title.y+mx(0,(l.title.h-title_size-2)/2);
    c.text_fitted(title,l.title.x,title_y,l.title.w,title_size,card.upgraded?Green:White,fonts.minimum,true,true);

    const char*type_text=card_type_label(type,&a.content);Canvas type_label=c;if(fonts.compact)type_label.font_face=5;
    const int type_size=fonts.type,plaque_h=fonts.compact?mx(type_size+3,l.type.h):mx(11,l.type.h);
    const int padding=fonts.compact?mx(2,card_face::scaled(17,w,300)):5;
    const int plaque_w=mn(w-6,mx(l.type.w,type_label.width(type_text,type_size)+padding));
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
    const auto description_layout=c.paragraph_fitted(description,l.description.x,l.description.y,l.description.w,l.description.h,body_size,White,fonts.body_minimum,true);
    if(description_truncated)*description_truncated=description_layout.truncated;

    c.picture(a.content.energy_image(card),l.energy.x,l.energy.y,l.energy.w,l.energy.h);
    const int energy=fee==-2?a.card_offer_cost(card,facts):fee;
    const int cost_size=fonts.energy;Canvas cost_label=c;if(fonts.compact)cost_label.font_face=5;
    char cost_text[16]{};TextBuffer cost_value{cost_text,sizeof cost_text,0,&a.content};if(!known)cost_value.ch('?');else if(energy==-1)cost_value.ch('X');else cost_value.number(energy);
    if(known)cost_label.energy_label(energy,l.energy.x+(l.energy.w-cost_label.width(cost_text,cost_size))/2,l.energy.y+(l.energy.h-cost_size-2)/2,cost_size);
    else cost_label.text("?",l.energy.x+(l.energy.w-cost_label.width("?",cost_size))/2,l.energy.y+(l.energy.h-cost_size-2)/2,cost_size,White);
    const int base_stars=card_star_cost(a.game,card);
    if(known&&(base_stars||(stars!=-2&&stars))){
        c.ui(U_Star,l.star.x,l.star.y,l.star.w,l.star.h);
        const int value=stars==-2?base_stars:stars;char label[16]{};TextBuffer count{label,sizeof label,0,&a.content};if(value<0)count.ch('X');else count.number(value);
        const int px=l.star.x+(l.star.w-cost_label.width(label,fonts.star))/2,py=l.star.y+(l.star.h-fonts.star-2)/2;
        for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy)if(dx||dy)cost_label.text(label,px+dx,py+dy,fonts.star,Ink);
        cost_label.text(label,px,py,fonts.star,White);
    }
}

namespace {
// Small-card grids repaint the same eight/ten faces at 30Hz. Store the exact
// destination before and after composition, rather than flattening translucent
// layers and changing RGB565 rounding. A cache hit compares every background
// pixel, so moving actors, selection highlights and overlapping cards remain
// exact. Eight protected slots cover a full library page and eight of ten
// selector cards without a cyclic LRU miss storm.
constexpr int SmallCardCacheW=56,SmallCardCacheH=70,SmallCardCacheCount=8;
struct SmallCardFaceCache {
    const Content* owner=nullptr;
    uint32_t generation=0,seen=0;
    Card card{};
    int portrait=-1,frame=-1,energy_image=-1,energy=0,stars=0;
    CardType type=CardType::Attack;
    uint8_t width=0,height=0;
    bool star_visible=false,valid=false;
    char title[128]{},description[512]{};
    uint16_t background[SmallCardCacheW*SmallCardCacheH]{};
    uint16_t pixels[SmallCardCacheW*SmallCardCacheH]{};
};
static SmallCardFaceCache small_card_faces[SmallCardCacheCount];
static uint32_t small_card_frame=0;
static_assert(sizeof small_card_faces+sizeof small_card_frame<=130u*1024u,
    "Small-card cache must fit its bounded 130KiB ARM allowance");
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
    int x=icon.x+(icon.w-text_layer.width(label,size))/2,y=icon.y+(icon.h-size-2)/2;
    for(const char*p=label;*p;){const auto g=c.a.content.glyph_metrics(utf8(p),size,face);
        if(g.w&&g.h&&(x-1<left||y-1<top||x+int(g.w)+1>left+SmallCardCacheW||y+int(g.h)+1>top+SmallCardCacheH))return false;
        x+=g.advance;}
    return true;
}
}
static void invalidate_card_face_cache(){for(auto&entry:small_card_faces)entry.valid=false;}
static void begin_card_face_frame(){if(++small_card_frame==0){invalidate_card_face_cache();++small_card_frame;}}
static void draw_card_face(Canvas&c,Card card,int x,int y,int w,int h,int fee=-2,int stars=-2,int instance=-1,int deck=-1,const EventCardFacts*facts=nullptr,const char*description=nullptr,bool*description_truncated=nullptr,bool known=true){
    // NCard.Model marks a local run/combat model when it is bound. Canonical
    // collection/history/title previews do not constitute such a binding.
    // This is an O(1) first-observation bit check, never a catalog scan.
    if(known&&!c.a.library&&!c.a.history_open&&!c.a.title&&c.a.game.world.active&&!c.a.game.practice)c.a.discover_card(card.kind);
    // Energy labels retain a1px outline above the source energy artwork.
    const int left=x-6,top=y-4;
    const bool supported_size=(w==46&&h==64)||(w==47&&h==66)||(w==43&&h==61)||(w==36&&h==50)||(w==44&&h==62);
    const bool candidate=known&&!description_truncated&&c.a.content.healthy()&&supported_size&&!c.coverage&&c.opacity==255&&c.brightness==255&&
        c.tint_red==255&&c.tint_green==255&&c.tint_blue==255&&left>=0&&top>=0&&
        left+SmallCardCacheW<=c.clip_w&&top+SmallCardCacheH<=c.clip_h;
    if(!candidate){render_card_face(c,card,x,y,w,h,fee,stars,instance,deck,facts,description,description_truncated,known);return;}
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
        render_card_face(c,card,x,y,w,h,fee,stars,instance,deck,facts,description);return;
    }
    const int portrait=a.card_face_image(card,instance,deck,facts),frame=a.card_face_frame(card,instance,deck,facts);
    const auto type=a.card_face_type(card,instance,deck,facts);
    const int energy_image=a.content.energy_image(card),energy=fee==-2?a.card_offer_cost(card,facts):fee;
    const int base_stars=card_star_cost(a.game,card),star_count=stars==-2?base_stars:stars;
    const bool star_visible=base_stars||(stars!=-2&&stars);
    const auto geometry=card_face::layout(x,y,w,h);const auto fonts=card_face_fonts(c,w);
    // Large dynamic costs can extend beyond the fixed cache, just as their
    // uncached labels do. Keep those exact without increasing the ARM pool.
    if(!a.content.healthy()||!card_cache_label_fits(c,geometry.energy,energy,false,left,top,fonts.energy,fonts.compact?5u:c.font_face)||
       (star_visible&&!card_cache_label_fits(c,geometry.star,star_count,true,left,top,fonts.star,fonts.compact?5u:c.font_face))){
        render_card_face(c,card,x,y,w,h,fee,stars,instance,deck,facts,description);return;}
    SmallCardFaceCache* entry=nullptr;
    for(auto&slot:small_card_faces)if(slot.valid&&slot.owner==&a.content&&slot.generation==a.content.generation()&&slot.width==w&&slot.height==h&&
        slot.card.kind==card.kind&&slot.card.upgraded==card.upgraded&&slot.portrait==portrait&&slot.frame==frame&&slot.type==type&&
        slot.energy_image==energy_image&&slot.energy==energy&&slot.stars==star_count&&slot.star_visible==star_visible&&
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
    if(!entry){render_card_face(c,card,x,y,w,h,fee,stars,instance,deck,facts,description);return;}
    entry->valid=false;entry->owner=&a.content;entry->generation=a.content.generation();entry->seen=small_card_frame;
    entry->card=card;entry->portrait=portrait;entry->frame=frame;entry->type=type;entry->width=uint8_t(w);entry->height=uint8_t(h);
    entry->energy_image=energy_image;entry->energy=energy;entry->stars=star_count;entry->star_visible=star_visible;
    card_cache_text_copy(entry->title,title);card_cache_text_copy(entry->description,description);
    for(int yy=0;yy<SmallCardCacheH;++yy)for(int xx=0;xx<SmallCardCacheW;++xx)
        entry->background[yy*SmallCardCacheW+xx]=c.pixels[(top+yy)*c.pitch+left+xx];
    render_card_face(c,card,x,y,w,h,fee,stars,instance,deck,facts,description);
    for(int yy=0;yy<SmallCardCacheH;++yy)for(int xx=0;xx<SmallCardCacheW;++xx)
        entry->pixels[yy*SmallCardCacheW+xx]=c.pixels[(top+yy)*c.pitch+left+xx];
    entry->valid=a.content.healthy();
}
