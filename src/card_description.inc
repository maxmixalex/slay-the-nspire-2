// Card model text comes from the private reference pack. Values are resolved
// against the selected instance, not whichever equal model was last played.
void App::card_description(Card c,int t,char*out,size_t cap,int instance,int deck_index)const{
    if(!cap)return;out[0]=0;if(c.kind>=CardKinds)return;
    if(c.kind>=EventCardBadLuck){world_card_description(c,t,out,cap,instance,deck_index,detail_has_facts&&detail_card==card_token(c)?&detail_facts:nullptr);return;}
    TextBuffer b{out,cap,0};const char*p=content.card_string(c.kind,c.upgraded?2:1);const auto&d=definitions[c.kind];
    const bool permanent=!library&&deck_index>=0&&deck_index<game.deck_count&&game.deck[deck_index].kind==c.kind;
    const bool live=!library&&!permanent&&combat_phase(game.phase);
    while(*p){
        if(*p=='\x1c'){++p;if(!live||t<0||t>=game.enemy_count||!game.enemies[t].hp){while(*p&&*p!='\x1d')++p;}continue;}
        if(*p=='\x1d'){++p;continue;}
        if(*p=='\x1e'){++p;if(!live){while(*p&&*p!='\x1f')++p;}continue;}
        if(*p=='\x1f'){++p;continue;}
        if(*p=='{'){
            // The converter embeds the exact catalog value alongside the
            // named live variable. Unknown live variables stay visibly
            // unresolved so content/UI regressions reject an incomplete hook.
            const char*start=p;const char*q=p+1;char variable[48]={};unsigned length=0;
            while(((*q>='A'&&*q<='Z')||(*q>='a'&&*q<='z')||(*q>='0'&&*q<='9'))&&length+1<sizeof variable)variable[length++]=*q++;
            if(length&&*q=='='){
                ++q;bool negative=*q=='-';if(negative)++q;int value=0;bool digits=false;
                while(*q>='0'&&*q<='9'&&value<1000000){digits=true;value=value*10+(*q++-'0');}
                if(digits&&*q=='}'){
                    if(negative)value=-value;
                    if(permanent)character_deck_description_value(game,c,deck_index,variable,value);
                    if(!live||character_description_value(game,c,t,instance,variable,value)){b.number(value);p=q+1;continue;}
                    while(start<=q)b.ch(*start++);p=q+1;continue;
                }
            }
        }
        if(p[0]=='{'&&p[1]&&p[2]=='}'){
            int value=0;
            switch(p[1]){
            case 's':value=d.creates+(c.upgraded?d.creates_up:0);break;
            case 'r':value=(c.kind==BouncingFlask||c.kind==SwordBoomerang)?(c.upgraded?4:3):1;break;
            case 'h':value=d.hp_loss;break;
            case 'e':value=d.energy_gain+(c.upgraded?d.energy_up:0);break;
            case 'd':value=live?damage(game,c,t,instance):d.damage+(c.upgraded?d.damage_up:0);if(permanent)character_deck_description_value(game,c,deck_index,"Damage",value);break;
            case 'b':value=live?card_block(game,c,instance):d.block+(c.upgraded?d.block_up:0);if(permanent)character_deck_description_value(game,c,deck_index,"Block",value);break;
            case 'n':value=d.draw+(c.upgraded?d.draw_up:0);break;
            case 'p':value=d.power+(c.upgraded?d.power_up:0);break;
            default:b.ch(*p++);continue;
            }
            b.number(value);p+=3;
        }else b.ch(*p++);
    }
    if(live&&c.kind==Shiv&&!card_targets_enemy(game,c))b.text("\n[gold]作用于所有敌人。[/gold]");
    if(live&&c.kind==SovereignBlade){
        if(game.regent.powers[(int)RPower::SeekingEdge])b.text("\n[gold]作用于所有敌人。[/gold]");
        if(game.regent.powers[(int)RPower::Parry]){b.text("\n获得 [blue]");b.number(card_block(game,c,instance));b.text("[/blue] 点格挡。");}
        if(game.regent.powers[(int)RPower::SwordSage]){b.text("\n剑圣：额外打出 [gold]");b.number(game.regent.powers[(int)RPower::SwordSage]);b.text("[/gold] 次。");}
    }
    if(live&&instance>=0&&instance<game.card_count&&game.cards[instance].kind==c.kind){
        const unsigned keywords=c.upgraded?d.upgraded_keywords:d.keywords;
        if(game.silent.inky[instance])b.text("\n[gold]墨影[/gold]：该牌的目标额外获得 [blue]1[/blue] 层虚弱。");
        if(game.silent.sly[instance]&&!(keywords&16))b.text("\n[gold]奇巧。[/gold]");
        if(game.silent.retain[instance]&&!card_retains(c))b.text("\n[gold]保留。[/gold]");
        if(game.necro.ethereal[instance]&&!(keywords&8))b.text("\n[gold]虚无。[/gold]");
        if(game.expansion.replay[instance]){b.text("\n[gold]重放 ");b.number(game.expansion.replay[instance]);b.text("。[/gold]");}
    }
}
