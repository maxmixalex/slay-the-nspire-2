// Card model text comes from the private reference pack. Values are resolved
// against the selected instance, not whichever equal model was last played.
void App::card_description(Card c,int t,char*out,size_t cap,int instance)const{
    if(!cap)return;out[0]=0;if(c.kind>=CardKinds)return;
    TextBuffer b{out,cap,0};const char*p=content.card_string(c.kind,c.upgraded?2:1);const auto&d=definitions[c.kind];
    const bool live=!library&&combat_phase(game.phase);
    while(*p){
        if(*p=='\x1e'){++p;if(!live){while(*p&&*p!='\x1f')++p;}continue;}
        if(*p=='\x1f'){++p;continue;}
        if(p[0]=='{'&&p[1]&&p[2]=='}'){
            int value=0;
            switch(p[1]){
            case 's':value=d.creates+(c.upgraded?d.creates_up:0);break;
            case 'r':value=(c.kind==BouncingFlask||c.kind==SwordBoomerang)?(c.upgraded?4:3):1;break;
            case 'h':value=d.hp_loss;break;
            case 'e':value=d.energy_gain+(c.upgraded?d.energy_up:0);break;
            case 'd':value=live?damage(game,c,t,instance):d.damage+(c.upgraded?d.damage_up:0);break;
            case 'b':value=live?card_block(game,c):d.block+(c.upgraded?d.block_up:0);break;
            case 'n':value=d.draw+(c.upgraded?d.draw_up:0);break;
            case 'p':value=d.power+(c.upgraded?d.power_up:0);break;
            default:b.ch(*p++);continue;
            }
            b.number(value);p+=3;
        }else b.ch(*p++);
    }
}
