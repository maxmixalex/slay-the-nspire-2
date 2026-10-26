static void label_local_rainbow_pixels(){
    const char*text="[rainbow val=1]M[/rainbow]";
    const auto previous_clock=app.map_clock_ms;const auto previous_animation=app.animation;
    const auto previous_remainder=app.tick_remainder;
    const bool previous_effects=app.text_effects,previous_fast=app.fast_mode;
    static SaveData before,after;CHECK(save(app.game,before));
    app.text_effects=true;app.animation=0;Canvas c{app};
    const auto draw=[&](unsigned painter,int x){
        switch(painter){
        case 0:c.text(text,x,40,10);break;
        case 1:c.paragraph(text,x,40,100,2,0,10);break;
        case 2:c.fitted(text,x,40,100,30,10,10,White);break;
        case 3:source_hover_paragraph(c,text,x,40,100,2);break;
        case 4:native_room_compact_text(c,text,x,40,100,30);break;
        case 5:native_room_scroll_text(c,text,x,40,100,30,0,10);break;
        default:ancient_text(c,text,x,40,100,2);break;
        }
    };
    const auto crop=[&](int x){std::vector<uint16_t> pixels;
        for(int y=40;y<70;++y)for(int xx=x;xx<x+100;++xx)pixels.push_back(app.pixels[y*320+xx]);return pixels;};
    for(unsigned painter=0;painter<7;++painter){
        app.rich_label_clocks.reset();app.map_clock_ms=9000;clear();
        {RichLabelClocks::Frame frame{app.rich_label_clocks};draw(painter,20);}
        const auto created=crop(20);unsigned ink=0;for(auto pixel:created)ink+=pixel!=0x0321;CHECK(ink>8);
        app.map_clock_ms=9250;clear();
        {RichLabelClocks::Frame frame{app.rich_label_clocks};draw(painter,20);draw(painter,180);}
        CHECK(crop(20)!=created);CHECK(crop(180)==created);
        app.map_clock_ms=9500;clear();
        {RichLabelClocks::Frame frame{app.rich_label_clocks};draw(painter,180);}
        app.map_clock_ms=9750;clear();
        {RichLabelClocks::Frame frame{app.rich_label_clocks};draw(painter,20);draw(painter,180);}
        CHECK(crop(20)==created);CHECK(crop(180)!=created);
    }
    // A stable label owner retains phase while its rendered rectangle moves.
    app.rich_label_clocks.reset();c.text_effect_identity=0x12345678u;app.map_clock_ms=10000;
    clear();{RichLabelClocks::Frame frame{app.rich_label_clocks};draw(0,20);}
    app.map_clock_ms=10250;clear();{RichLabelClocks::Frame frame{app.rich_label_clocks};draw(0,180);}
    const auto moved=crop(180);c.text_effect_identity=0;app.rich_label_clocks.reset();
    app.map_clock_ms=10000;clear();{RichLabelClocks::Frame frame{app.rich_label_clocks};draw(0,180);}
    app.map_clock_ms=10250;clear();{RichLabelClocks::Frame frame{app.rich_label_clocks};draw(0,180);}
    CHECK(crop(180)==moved);
    // Use the real advance clock in both speed modes. Its elapsed duration
    // changes phase, while Fast's shorter command waits do not multiply it.
    std::vector<uint16_t> normal;
    for(bool fast:{false,true}){
        app.rich_label_clocks.reset();app.map_clock_ms=11000;app.fast_mode=fast;
        clear();{RichLabelClocks::Frame frame{app.rich_label_clocks};draw(0,20);}
        app.advance(125);app.advance(125);CHECK(app.map_clock_ms==11250);
        clear();{RichLabelClocks::Frame frame{app.rich_label_clocks};draw(0,20);}
        if(!fast)normal=crop(20);else CHECK(crop(20)==normal);
    }
    CHECK(save(app.game,after)&&before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
    app.map_clock_ms=previous_clock;app.animation=previous_animation;app.text_effects=previous_effects;app.fast_mode=previous_fast;
    app.tick_remainder=previous_remainder;
    app.rich_label_clocks.reset();
}
