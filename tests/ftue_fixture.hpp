#pragma once
// Synthetic directory/pixels for modal behavior; no original game data.
static void put(Bytes&b,unsigned at,uint32_t value,unsigned size=4){CHECK(at+size<=b.size());for(unsigned i=0;i<size;++i)b[at+i]=uint8_t(value>>(8*i));}
static uint32_t crc(const Bytes&b){uint32_t c=~0u;for(uint8_t value:b){c^=value;for(int i=0;i<8;++i)c=(c>>1)^(0xedb88320u&uint32_t(-int32_t(c&1)));}return ~c;}
static unsigned group(Bytes&b,const Bytes&raw,unsigned type,unsigned id){
    if(b.size()%2)b.push_back(0);const unsigned at=unsigned(b.size());b.resize(at+32);
    const unsigned fields[]={0x31524754,type,id,unsigned(raw.size()),unsigned(raw.size())+4,crc(raw),1,0};
    for(unsigned i=0;i<8;++i)put(b,at+i*4,fields[i]);
    b.push_back(1);b.push_back(uint8_t((raw.size()-1)>>8));b.push_back(uint8_t(raw.size()-1));
    b.insert(b.end(),raw.begin(),raw.end());b.push_back(0);return at;
}
static Bytes fixture(){
    // Arbitrary solid shapes make picture placement and clipping observable;
    // none of these synthetic pixels or labels comes from the original game.
    constexpr unsigned images=200,glyphs=2*7*96;
    const unsigned glyph_at=40+images*16,string_at=glyph_at+glyphs*16;
    const unsigned clip_at=string_at+Ui7StringCount*8,card_at=clip_at+CharacterDeathAnimationKinds*16;
    const unsigned ui_at=card_at+CardKinds*16,base=ui_at+FtueAuxiliaryCount*4;
    Bytes b(base+96+images);put(b,0,0x3250534e);put(b,4,FtueContentVersion);put(b,8,0x41cef1ea);
    put(b,16,images);put(b,20,glyphs);put(b,24,Ui7StringCount);put(b,28,CharacterDeathAnimationKinds);put(b,32,CardKinds);
    auto picture=[&](unsigned id,unsigned w,unsigned h,uint16_t color,unsigned shape=0){
        if(b.size()%2)b.push_back(0);const unsigned at=unsigned(b.size());b.resize(at+w*h*3);
        put(b,40+id*16,at);put(b,44+id*16,w,2);put(b,46+id*16,h,2);put(b,48+id*16,1);put(b,52+id*16,w*h*3);
        for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){unsigned p=y*w+x;put(b,at+p*2,color,2);
            b[at+w*h*2+p]=shape==1?0:shape==2&&x>=7&&x<74&&y>=11&&y<61?0:shape==3&&x>2&&x+3<w&&y>2&&y+3<h?0:shape==4&&(x<4||x>=31)?0:255;}
    };
    picture(0,82,60,0,1);for(unsigned i=1;i<images;++i)for(unsigned k=0;k<16;++k)b[40+i*16+k]=b[40+k];
    picture(1,82,60,0x07e0);picture(2,80,112,0x001f,2);picture(3,110,28,0x2104);
    picture(4,320,22,0x2104);picture(5,30,28,0x39c7);picture(6,24,24,0xf800);picture(7,20,20,0xffff,3);
    picture(8,18,18,0xffe0);picture(9,18,18,0xf81f);picture(10,92,70,0x7bef,3);picture(11,32,16,0x7bef);picture(12,36,40,0xf800,4);
    unsigned index=0;
    for(unsigned style:{0u,5u})for(unsigned size:{4u,5u,6u,8u,10u,12u,16u})for(unsigned n=0;n<96;++n,++index){
        const unsigned cp=n<95?32+n:0x2026,w=cp==0x2026?size:(size+1)/2,h=size+2,at=glyph_at+index*16,off=unsigned(b.size());
        put(b,at,cp);put(b,at+4,off);put(b,at+8,size|((style&3)<<6)|((style&4)<<3),2);b[at+10]=uint8_t(w);b[at+11]=uint8_t(h);put(b,at+12,w,2);b.resize(off+w*h);
        for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x)b[off+y*w+x]=cp==' '?0:cp==0x2026?uint8_t(y==h-2&&x%3==0?255:0):255;
    }
    const unsigned empty=unsigned(b.size());b.push_back(0);for(unsigned i=0;i<Ui7StringCount;++i)put(b,string_at+i*8,empty);
    auto text=[&](unsigned id,const char*s){unsigned n=unsigned(std::strlen(s));put(b,string_at+id*8,unsigned(b.size()));put(b,string_at+id*8+4,n);b.insert(b.end(),s,s+n+1);};
    text(0,"Bounded card title");text(1,"[gold]Gain 3 Stars.[/gold] Deal 123 damage. This longer description must end visibly inside the card.");text(LocaleMarkerString,"eng");
    for(unsigned i=0;i<CharacterDeathAnimationKinds;++i){put(b,clip_at+i*16+4,1,2);put(b,clip_at+i*16+6,33,2);}
    for(unsigned i=0;i<CardKinds;++i){put(b,card_at+i*16,1);put(b,card_at+i*16+8,1);put(b,card_at+i*16+12,1);}
    for(int image:{U_BigAttack,U_BigSkill,U_BigPower,U_RedAttack,U_RedSkill,U_RedPower})put(b,ui_at+image*4,2);
    for(int image:{U_CommonBanner,U_BigBanner,U_RareBanner})put(b,ui_at+image*4,3);
    put(b,ui_at+U_TopBar*4,4);put(b,ui_at+U_Star*4,9);put(b,ui_at+U_TopBarBackdrop*4,5);put(b,ui_at+U_PotionPlaceholder*4,7);
    for(unsigned i=0;i<8;++i){for(unsigned j=0;j<3;++j)put(b,ui_at+(U_CardBorderCommonAttack+i*5+j)*4,10);put(b,ui_at+(U_CardBorderCommonAttack+i*5+3)*4,11);put(b,ui_at+(U_CardBorderCommonAttack+i*5+4)*4,3);}
    for(unsigned i=0;i<5;++i)put(b,ui_at+(U_TopBarIronclad+i)*4,6);
    for(int image:{U_CharPortrait,U_SilentPortrait,U_DefectPortrait,U_RegentPortrait,U_NecrobinderPortrait})put(b,ui_at+image*4,12);
    const unsigned fields[]={0x314c4954,1,16,14,1,1,1,1,base+80,base+64,770,85,524288,ResourceTransferCapacity,base+96,images};
    for(unsigned i=0;i<16;++i)put(b,base+i*4,fields[i]);
    for(unsigned i=0;i<images;++i)b[base+96+i]=1;
    for(unsigned i=FtueTextFirst;i<FtueTextFirst+FtueTextCount;++i)text(i,"SYNTHETIC TUTORIAL BODY");
    for(unsigned i=I9_FtuePopup;i<FtueAuxiliaryCount;++i)put(b,ui_at+i*4,3);
    Bytes tile(770);tile[0]=tile[1]=1;Bytes record(18);put(record,0,2);put(record,4,4);put(record,8,2);
    unsigned tiles=group(b,tile,1,0),indexes=group(b,record,2,1);put(b,base+64,tiles);put(b,base+68,32+unsigned(tile.size())+4);
    put(b,base+72,indexes);put(b,base+76,32+unsigned(record.size())+4);put(b,base+80,base);put(b,base+92,unsigned(record.size()));
    put(b,40+199*16,base+80);put(b,44+199*16,1,2);put(b,46+199*16,1,2);put(b,48+199*16,6);put(b,52+199*16,16);
    put(b,12,unsigned(b.size()));uint32_t hash=2166136261u;for(unsigned i=40;i<b.size();++i)hash=(hash^b[i])*16777619u;put(b,36,hash);return b;
}
