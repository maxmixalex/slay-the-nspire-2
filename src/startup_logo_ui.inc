static void startup_logo_screen(Canvas&c){
    auto&a=c.a;const auto frame=a.startup_logo.sample(a.map_clock_ms);
    if(frame.menu){
        const bool pointer=a.pointer.visible;a.pointer.visible=false;title_screen(c);a.pointer.visible=pointer;
    }else{
        c.rect(0,0,320,240,rgb(7*frame.background/4096,66*frame.background/4096,84*frame.background/4096));
        if(frame.alpha)c.ui(int(I12_IntroLogoFirst+frame.image),80,40+frame.y,160,160,frame.alpha);
    }
    if(frame.black){Canvas veil=c;veil.opacity=uint8_t(div255(unsigned(c.opacity)*frame.black));veil.rect(0,0,320,240,0);}
    a.hit_count=0;
}
