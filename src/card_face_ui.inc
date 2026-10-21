// One scene-based card renderer for every presentation size. Individual
// screens supply instance-aware costs/descriptions, never rearranged art.
static void draw_card_face(Canvas&c,Card card,int x,int y,int w,int h,int fee=-2,int stars=-2,int instance=-1,int deck=-1,const EventCardFacts*facts=nullptr,const char*description=nullptr){
    auto&a=c.a;if(!facts)facts=event_card_facts(a.game,card,instance,deck);
    const auto type=a.card_face_type(card,instance,deck,facts);
    const unsigned group=card_face::material_group(unsigned(card_rarity(card.kind)));
    const int base=U_CardBorderCommonAttack+int(group)*5;
    const int border=base+(type==CardType::Attack?0:type==CardType::Power?2:1);
    auto l=card_face::layout(x,y,w,h);
    // Portrait precedes Frame and PortraitBorder in the original scene.
    // Drawing it last used to cover the frame's lower edge and description.
    c.picture(a.card_face_image(card,instance,deck,facts),l.portrait.x,l.portrait.y,l.portrait.w,l.portrait.h);
    c.picture(a.card_face_frame(card,instance,deck,facts),x,y,w,h);
    c.ui(border,l.border.x,l.border.y,l.border.w,l.border.h);
    int banner=a.content.ui_image(base+4);
    if(banner<0)banner=a.content.ui_image(group==2?U_RareBanner:group==1?U_BigBanner:U_CommonBanner);
    c.picture(banner,l.banner.x,l.banner.y,l.banner.w,l.banner.h);
    char title[160]{};TextBuffer name{title,sizeof title,0,&a.content};name.text(a.content.card_string(card.kind));if(card.upgraded)name.ch('+');
    const int title_size=w>=100?12:w>=70?10:8;
    const int title_y=l.title.y+mx(0,(l.title.h-title_size-2)/2);
    for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy)if(dx||dy)
        c.text_fitted(title,l.title.x+dx,title_y+dy,l.title.w,title_size,Ink,8,true);
    c.text_fitted(title,l.title.x,title_y,l.title.w,title_size,card.upgraded?Green:White,8,true);

    const char*type_text=card_type_label(type,&a.content);
    const int type_size=8,plaque_h=mx(11,l.type.h);
    const int plaque_w=mn(w-6,mx(l.type.w,c.width(type_text,type_size)+5));
    const int plaque_x=x+(w-plaque_w)/2;
    int plaque=a.content.ui_image(base+3);
    if(plaque>=0)c.nine_image(plaque,plaque_x,l.type.y,plaque_w,plaque_h,4,0,4,0);
    c.text_fitted(type_text,plaque_x+2,l.type.y,plaque_w-4,type_size,White,8,true);

    char body[1024]{};
    if(!description){
        if(facts)a.world_card_description(card,-1,body,sizeof body,instance,deck,facts);
        else a.card_description(card,-1,body,sizeof body,instance,deck);
        description=body;
    }
    // The type plaque needs at least an 8px label on this screen. Only that
    // readable minimum affects the description bounds; art stays authored.
    const int bottom=l.description.y+l.description.h;
    l.description.y=mx(l.description.y,l.type.y+plaque_h);
    l.description.h=mx(0,bottom-l.description.y);
    c.paragraph_fitted(description,l.description.x,l.description.y,l.description.w,l.description.h,w>=100?10:8,White,8,true);

    c.picture(a.content.energy_image(card),l.energy.x,l.energy.y,l.energy.w,l.energy.h);
    const int energy=fee==-2?a.card_offer_cost(card,facts):fee;
    const int cost_size=w>=100?12:w>=70?10:8;
    char cost_text[16]{};TextBuffer cost_value{cost_text,sizeof cost_text,0,&a.content};if(energy==-1)cost_value.ch('X');else cost_value.number(energy);
    c.energy_label(energy,l.energy.x+(l.energy.w-c.width(cost_text,cost_size))/2,l.energy.y+(l.energy.h-cost_size-2)/2,cost_size);
    const int base_stars=card_star_cost(a.game,card);
    if(base_stars||(stars!=-2&&stars)){
        c.ui(U_Star,l.star.x,l.star.y,l.star.w,l.star.h);
        const int value=stars==-2?base_stars:stars;char label[16]{};TextBuffer count{label,sizeof label,0,&a.content};if(value<0)count.ch('X');else count.number(value);
        const int px=l.star.x+(l.star.w-c.width(label,8))/2,py=l.star.y+(l.star.h-10)/2;
        for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy)if(dx||dy)c.text(label,px+dx,py+dy,8,Ink);
        c.text(label,px,py,8,White);
    }
}
