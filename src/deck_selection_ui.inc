// Original shared bottom selection prompt. The rich label is fit-content,
// single-line 24px Kreon (5px here); its parent supplies a 16px/6px margin.
// Combat selection screens use the same authored label and can share this.
static void card_selection_prompt(Canvas&c,const char*text){
    using namespace deck_selection_layout;
    if(!text||!*text)return;
    Canvas label=c;label.bold_face=5;
    const int width=mn(312,mx(24,label.width(text,PromptSize)+2*PromptPadding));
    Canvas background=c;background.opacity=uint8_t(div255(unsigned(background.opacity)*192));
    background.rect(160-width/2,PromptY,width,PromptH,0);
    Canvas shadow=label;shadow.opacity=uint8_t(div255(unsigned(shadow.opacity)*64));
    shadow.fitted(text,161-width/2+PromptPadding,PromptY+1,width-2*PromptPadding,PromptH,PromptSize,PromptSize,0,true,true);
    label.fitted(text,160-width/2+PromptPadding,PromptY+1,width-2*PromptPadding,PromptH,PromptSize,PromptSize,rgb(255,246,226),true,true);
}
static LocalizedValue deck_selection_amount(const void*,const char*key){
    return wt_equal(key,"Amount")?LocalizedValue{true,1,1,nullptr}:LocalizedValue{};
}
static void deck_selection_prompt(Canvas&c){
    const char*key=c.a.game.phase==Phase::Upgrade?"TO_UPGRADE":c.a.game.phase==Phase::Transform?"TO_TRANSFORM":"TO_REMOVE";
    char prompt[256]{};LocalizedTemplate formatted(prompt,sizeof prompt,deck_selection_amount,nullptr);
    formatted.format(c.a.content.world_string("card_selection",key));card_selection_prompt(c,prompt);
}
static void card_selection_upgrades_control(Canvas&c){
    auto&a=c.a;
    using namespace deck_selection_layout;
    const char*label=a.content.world_string("card_selection","VIEW_UPGRADES");
    Canvas text=c;text.font_face=5;
    const int width=ToggleIcon+mn(140,text.width(label,ToggleSize));
    c.ui(a.deck_selection.show_upgrades?F_SettingsTicked:F_SettingsUnticked,ToggleX+1,ToggleY,ToggleIcon,ToggleIcon);
    // The original Label is fixed 27px inside a .75 parent: native 4px.
    // Card fitting has a 5px floor, so this authored label must bypass it.
    // Its 12px black outline at .5 alpha projects to a two-pixel radius.
    const int x=ToggleX+ToggleIcon+1,y=ToggleY+2,radius=reference_viewport::project(9);
    Canvas outline=text;outline.opacity=uint8_t(div255(unsigned(text.opacity)*128));
    for(int dy=-radius;dy<=radius;++dy)for(int dx=-radius;dx<=radius;++dx)
        if((dx||dy)&&dx*dx+dy*dy<=radius*radius)outline.text(label,x+dx,y+dy,ToggleSize,0,140,1);
    text.text(label,x,y,ToggleSize,rgb(239,200,81),140,1);
    c.action(ToggleX,ToggleY,width+2,ToggleIcon,UiAction::DeckViewUpgrades);
}
static void deck_selection_upgrades(Canvas&c){
    if(c.a.game.phase==Phase::Upgrade&&c.a.pointer.visible)card_selection_upgrades_control(c);
}
static void card_selection_upgrade_arrows(Canvas&c,int center_x=160,int center_y=120,int scale=1000){
    const int arrow=c.a.content.ui_image(I9_UpgradeArrow);if(arrow<0)return;
    Canvas shadow=c;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;
    const auto px=[&](int value){return CardRewardPresentation::rounded(value*scale,1000);};
    static constexpr int faces[]={-19,-7,5},shadows[]={-18,-5,7};
    for(int i=0;i<3;++i){shadow.picture(arrow,center_x+px(shadows[i]),center_y+px(-5),px(12),px(12),64);c.picture(arrow,center_x+px(faces[i]),center_y+px(-6),px(12),px(12));}
}
static void card_selection_upgrade_preview(Canvas&c,Card before,int instance=-1,int deck=-1,int scale=1000,int center_y=120,int before_scale=1000){
    using namespace deck_selection_layout;
    const auto px=[&](int value){return CardRewardPresentation::rounded(value*scale,1000);};
    const int y=center_y+px(PreviewY-120),width=px(PreviewW),height=px(PreviewH);
    const int before_width=CardRewardPresentation::rounded(width*before_scale,1000),before_height=CardRewardPresentation::rounded(height*before_scale,1000);
    draw_card_face(c,before,160+px(BeforeX-160)+(width-before_width)/2,y+(height-before_height)/2,before_width,before_height,-2,-2,instance,deck);
    Card after=before;after.upgraded=1;
    draw_card_face(c,after,160+px(AfterX-160),y,width,height,-2,-2,instance,deck,nullptr,nullptr,nullptr,true,true);
    card_selection_upgrade_arrows(c,160,center_y,scale);
}
