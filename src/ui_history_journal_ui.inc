struct JournalTextValues {
 const char*first="",*second="",*choice="",*relic="",*potion="",*rarity="",*room="",*model="";
 uint32_t amount=0,hp=0,max_hp=0,gold=0,floor=0,turns=0;
};
static LocalizedValue journal_text_value(const void*context,const char*key){
 const auto&v=*static_cast<const JournalTextValues*>(context);
 if(wt_equal(key,"Icon"))return wt_string("");
 if(wt_equal(key,"Title")||wt_equal(key,"Title1"))return wt_string(v.first);
 if(wt_equal(key,"Title2"))return wt_string(v.second);
 if(wt_equal(key,"Quest"))return wt_string(v.first);
 if(wt_equal(key,"Choice"))return wt_string(v.choice);
 if(wt_equal(key,"Relic"))return wt_string(v.relic);
 if(wt_equal(key,"Potion"))return wt_string(v.potion);
 if(wt_equal(key,"Rarity"))return wt_string(v.rarity);
 if(wt_equal(key,"MapPointType"))return wt_string(v.room);
 if(wt_equal(key,"ModelTitle"))return wt_string(v.model);
 if(wt_equal(key,"Amount")||wt_equal(key,"Damage"))return {true,v.amount};
 if(wt_equal(key,"HP"))return {true,v.hp};if(wt_equal(key,"MaxHP"))return {true,v.max_hp};
 if(wt_equal(key,"Gold"))return {true,v.gold};if(wt_equal(key,"FloorNum"))return {true,v.floor};
 if(wt_equal(key,"Turns"))return {true,v.turns};return {};
}
static void journal_text(const App&a,const char*key,const char*fallback,const JournalTextValues&values,char*out,unsigned size){
 const char*format=a.content.world_string("run_history",key);
 LocalizedTemplate(out,size,journal_text_value,&values,"Energy","Stars",a.content.english()).format(*format?format:fallback);
}
static void journal_card_name(const App&a,unsigned token,char*out,unsigned size){
 const auto card=card_from_token(int(token));TextBuffer text{out,size,0};
 if(card.kind>=CardKinds)return;const char*title=a.content.card_string(card.kind);text.text(*title?title:definitions[card.kind].key);if(card.upgraded)text.ch('+');
}
static const char* journal_potion_name(const App&a,unsigned model){
 const auto*d=potion_definition(Potion(model));if(!d)return "";const char*name=a.content.world_title("potions",d->key);return *name?name:d->name;
}
void App::history_journal_line(const UiHistoryJournalEntry&e,char*out,unsigned size)const{
 if(!size)return;out[0]=0;JournalTextValues values;char first[256]{},second[256]{},choice[384]{};
 const char*key="HISTORY_ENTRY.obtained",*fallback="{Title}";using K=RunHistoryEvent;
 const bool card=e.kind>=K::CardObtained&&e.kind<=K::CardBought;
 if(card){const unsigned token=e.kind==K::CardUpgraded||e.kind==K::CardDowngraded?unsigned(card_token({card_from_token(e.model).kind,0})):e.model;journal_card_name(*this,token,first,sizeof first);values.first=first;}
 else if((e.kind>=K::RelicObtained&&e.kind<=K::RelicBought)||e.kind==K::AncientPicked||e.kind==K::AncientSkipped){
  values.first=e.model==NoWorldRelic?content.world_string("ancients","OROBAS.pages.INITIAL.options.OPTION_POOL_3_LOCKED.title"):relic_title(RelicId(e.model));
 }else if(e.kind>=K::PotionObtained&&e.kind<=K::PotionBought)values.first=journal_potion_name(*this,e.model);
 switch(e.kind){
 case K::CardTransformed:
  if(e.flags==1)return; // A suspended replacement has not committed yet.
  journal_card_name(*this,e.detail,second,sizeof second);values.second=second;key="HISTORY_ENTRY.transformed";fallback="{Title1} transformed into {Title2}.";break;
 case K::CardEnchanted:values.second=content.world_title("enchantments",event_enchantment_key(EventEnchantment(e.flags)));key="HISTORY_ENTRY.enchanted";fallback="{Title1} enchanted with {Title2}.";break;
 case K::CardRemoved:case K::RelicRemoved:case K::PotionDiscarded:key="HISTORY_ENTRY.removed";fallback="{Title} was removed.";break;
 case K::CardUpgraded:key="HISTORY_ENTRY.upgraded";fallback="{Title} was upgraded.";break;
 case K::CardDowngraded:key="HISTORY_ENTRY.downgraded";fallback="{Title} was downgraded.";break;
 case K::PotionUsed:key="HISTORY_ENTRY.used";fallback="{Title} was used.";break;
 case K::CardPicked:case K::AncientPicked:values.choice=values.first;key="MAP_POINT_HISTORY.chose";fallback="Chose {Choice}";break;
 case K::AncientSkipped:values.choice=values.first;key="MAP_POINT_HISTORY.skipped";fallback="Skipped {Choice}";break;
 case K::EventChoice:{
  const auto variable=HistoryChoiceVariable(e.flags);values.gold=e.detail;
  if(variable==HistoryChoiceVariable::Relic)values.relic=relic_title(RelicId(e.detail));
  else if(variable==HistoryChoiceVariable::Potion)values.potion=journal_potion_name(*this,e.detail);
  else if(variable==HistoryChoiceVariable::PotionRarity){const char*keys[]={"","POTION_RARITY.COMMON","POTION_RARITY.UNCOMMON","POTION_RARITY.RARE","POTION_RARITY.EVENT","POTION_RARITY.TOKEN"};if(e.detail<=5)values.rarity=content.world_string("gameplay_ui",keys[e.detail]);}
  LocalizedTemplate(choice,sizeof choice,journal_text_value,&values,"Energy","Stars",content.english()).format(content.string(e.model));
  values.choice=choice;key="MAP_POINT_HISTORY.chose";fallback="Chose {Choice}";break;
 }
 case K::ModifierChoice:values.choice=run_modifier_text(RunModifier(e.model));key="MAP_POINT_HISTORY.chose";fallback="Chose {Choice}";break;
 case K::RestChoice:{
  const char*keys[]={"OPTION_HEAL.name","OPTION_SMITH.name","OPTION_HATCH.name","OPTION_DIG.name","OPTION_LIFT.name","OPTION_KINDLE.name","OPTION_COOK.name","OPTION_CLONE.name"};
  values.choice=e.model<8?content.world_string("rest_site_ui",keys[e.model]):"";key="MAP_POINT_HISTORY.chose";fallback="Chose {Choice}";break;
 }
 case K::GoldSpent:key="HISTORY_ENTRY.goldSpent";fallback="Spent {Amount} Gold";break;
 case K::GoldLost:key="HISTORY_ENTRY.goldLost";fallback="Lost {Amount} Gold";break;
 case K::GoldStolen:key="HISTORY_ENTRY.goldStolen";fallback="{Amount} Gold was stolen";break;
 case K::DamageTaken:key="MAP_POINT_HISTORY.damageTaken";fallback="[red]{Damage} Damage[/red]";break;
 case K::Healed:key="MAP_POINT_HISTORY.healed";fallback="[green]Healed for {HP} HP[/green]";break;
 case K::MaxHpGained:key="MAP_POINT_HISTORY.maxHpGained";fallback="[green]Gained {HP} Max HP[/green]";break;
 case K::MaxHpLost:key="MAP_POINT_HISTORY.maxHpLost";fallback="[red]Lost {HP} Max HP[/red]";break;
 case K::QuestCompleted:journal_card_name(*this,unsigned(card_token({CardKind(e.model),0})),first,sizeof first);values.first=first;key="MAP_POINT_HISTORY.questCompleted";fallback="Completed the {Quest} Quest.";break;
 case K::FurCoat:values.first=relic_title(WorldRelicFurCoat);LocalizedTemplate(out,size,journal_text_value,&values).format(content.world_string("relics","FUR_COAT.historyEntry"));return;
 case K::None:
  if(e.flags==1){key="MAP_POINT_HISTORY.turnsTaken";fallback="{Turns} Turns";values.turns=e.amount();}
  else if(e.flags==2){key="HISTORY_ENTRY.goldGained";fallback="{Amount} Gold";values.amount=e.amount();}break;
 default:break;
 }
 if(e.kind>=K::GoldSpent&&e.kind<=K::MaxHpLost){values.amount=e.amount();values.hp=e.amount();}
 journal_text(*this,key,fallback,values,out,size);
 if(e.kind==K::CardBought||e.kind==K::RelicBought||e.kind==K::PotionBought){
  char spent[96]{};values.amount=e.detail;journal_text(*this,"HISTORY_ENTRY.goldSpent","Spent {Amount} Gold",values,spent,sizeof spent);
  TextBuffer text{out,size,0};while(out[text.n])++text.n;text.text(" · ");text.text(spent);
 }
}
static unsigned journal_rank(const UiHistoryJournalEntry&e){
 using K=RunHistoryEvent;
 switch(e.kind){
 case K::AncientPicked:return 0;case K::AncientSkipped:return 1;case K::EventChoice:return 2;case K::ModifierChoice:return 3;
 case K::RestChoice:return 4;case K::FurCoat:return 5;case K::MaxHpLost:return 6;case K::DamageTaken:return 7;
 case K::MaxHpGained:return 8;case K::Healed:return 9;case K::QuestCompleted:return 11;
 case K::PotionUsed:return 12;case K::PotionDiscarded:return 13;case K::GoldSpent:return 14;case K::GoldLost:return 15;case K::GoldStolen:return 16;
 case K::CardObtained:return 18;case K::RelicObtained:return 19;case K::PotionObtained:return 20;case K::CardRemoved:return 21;
 case K::RelicRemoved:return 22;case K::CardUpgraded:return 23;case K::CardDowngraded:return 24;case K::CardEnchanted:return 25;
 case K::CardTransformed:return e.flags==1?30:26;
 case K::CardSkipped:return 27;case K::RelicSkipped:return 28;case K::PotionSkipped:return 29;
 default:return 30; // Picked cards and purchases have their original metadata,
 }                 // but the source tooltip presents their gained models once.
}
static int journal_icon(const UiHistoryJournalEntry&e){
 using K=RunHistoryEvent;
 if(e.kind>=K::CardObtained&&e.kind<=K::CardBought)return I8_HistoryCard;
 if(e.kind>=K::RelicObtained&&e.kind<=K::RelicBought)return I8_HistoryChest;
 if(e.kind>=K::PotionObtained&&e.kind<=K::PotionBought)return I8_HistoryPotion;
 if((e.kind==K::None&&e.flags==2)||e.kind==K::GoldStolen)return I8_HistoryGold;
 return -1;
}
static UiHistoryJournalEntry journal_display_entry(const UiHistoryJournal&journal,unsigned index,const UiFloorHistory&point){
 if(index<UiJournalEntries)return journal.at(index);
 if(index==UiJournalEntries)return {point.turns,0,0,0,RunHistoryEvent::None,1};
 if(index==UiJournalEntries+1)return {uint16_t(point.gold_gained),uint16_t(point.gold_gained>>16),0,0,RunHistoryEvent::None,2};
 return {0,0,0,0,RunHistoryEvent::DamageTaken,0};
}
static void journal_floor_tooltip(Canvas&c,const UiRunHistory&run,unsigned floor,int node_x,int node_y,unsigned source){
 if(floor>=run.floor_count)return;auto&a=c.a;auto&motion=a.history_tooltip;const auto&point=run.floors[floor];
 motion.bind(run.serial,floor,source);motion.node_x=int16_t(node_x);motion.node_y=int16_t(node_y);
 const auto&journal=a.progress.journal;const int owner=journal.owner(run.serial);
 const bool partial=owner<0||journal.owners[owner].incomplete(floor);
 const auto room=WorldRoom(point.room);const bool combat_room=room==WorldRoom::Monster||room==WorldRoom::Elite||room==WorldRoom::Boss;
 // Each consumed order slot is written by its rank below. Clearing all
 // 4,099 slots would touch 8 KiB even for a floor with only one visible row.
 uint16_t order[UiJournalEntries+3],counts[30]{},positions[30];unsigned groups[3]{};
 bool observed_combat=false;unsigned first=journal.count,last=0;
 for(unsigned i=0;i<journal.count;++i){const auto&e=journal.at(i);if(e.owner==owner&&e.floor==floor){const unsigned rank=journal_rank(e);
  if(rank<30){++counts[rank];if(first==journal.count)first=i;last=i+1;}if(e.kind==RunHistoryEvent::CombatStarted)observed_combat=true;}}
 // Older profiles could copy a retained encounter into the following shop
 // or event. Only its real combat room or a new observed start proves that
 // this point contained combat. A partial event cannot identify its first
 // fight, so its legacy turn snapshot must remain unknown.
 const bool combat=combat_room||observed_combat,turns=combat_room||(!partial&&observed_combat);
 const bool zero_damage=combat&&!partial&&!counts[7];if(zero_damage)++counts[7];
 if(turns)++counts[10];if(point.gold_gained)++counts[17];
 unsigned count=0;for(unsigned i=0;i<30;++i){positions[i]=uint16_t(count);count+=counts[i];groups[i<17?0:i<27?1:2]+=counts[i];}
 // Normal observed floors occupy a short contiguous span. Keeping the
 // owner/floor filter also preserves any valid interleaved decoded journal.
 for(unsigned i=first;i<last;++i){const auto&e=journal.at(i);if(e.owner==owner&&e.floor==floor){const unsigned rank=journal_rank(e);if(rank<30)order[positions[rank]++]=uint16_t(i);}}
 if(turns)order[positions[10]++]=UiJournalEntries;
 if(point.gold_gained)order[positions[17]++]=UiJournalEntries+1;
 if(zero_damage)order[positions[7]++]=UiJournalEntries+2;
 // NMapPointHistoryHoverTip fills a whole first column, then the second.
 // It opens the second only above five entries and gives column one at
 // least five rows. This is not an alternating row-major rewards grid.
 const unsigned splits[3]={groups[0],groups[1]>5?mx(5u,(groups[1]+1)/2):groups[1],groups[2]>5?mx(5u,(groups[2]+1)/2):groups[2]};
 const unsigned row_count=splits[0]+splits[1]+splits[2];motion.maximum=int16_t(row_count?row_count-1:0);motion.scroll(0);
 constexpr int width=172,body_width=156,max_height=202;
 Canvas text=c;text.font_face=0;text.bold_face=5;Canvas bold=text;bold.font_face=5;
 JournalTextValues values;values.floor=point.floor;values.hp=point.hp;values.max_hp=point.max_hp;values.gold=point.gold;
 char title[192]{},player[192]{},room_line[512]{};
 journal_text(a,"MAP_POINT_HISTORY.header","Floor {FloorNum}",values,title,sizeof title);
 journal_text(a,"MAP_POINT_HISTORY.player_stats","{HP}/{MaxHP} HP    {Gold} Gold",values,player,sizeof player);
 const char*room_key=point.point==0?"ROOM_ANCIENT.title":room==WorldRoom::Event?"ROOM_EVENT.title":room==WorldRoom::Shop?"ROOM_MERCHANT.title":room==WorldRoom::Rest?"ROOM_REST.title":room==WorldRoom::Treasure?"ROOM_TREASURE.title":room==WorldRoom::Elite?"ROOM_ELITE.title":room==WorldRoom::Boss?"ROOM_BOSS.title":"ROOM_ENEMY.title";
 if(point.map_type==unsigned(reference::PointType::Unknown))room_key=room==WorldRoom::Monster?"ROOM_UNKNOWN_ENEMY.title":room==WorldRoom::Elite?"ROOM_UNKNOWN_ELITE.title":room==WorldRoom::Shop?"ROOM_UNKNOWN_MERCHANT.title":room==WorldRoom::Treasure?"ROOM_UNKNOWN_TREASURE.title":room_key;
 char room_label[128]{},model_title[192]{};TextBuffer room_text{room_label,sizeof room_label,0};room_text.text(a.content.world_string("static_hover_tips",room_key));values.room=room_label;
 TextBuffer model{model_title,sizeof model_title,0};
 if(room==WorldRoom::Event)model.text(a.content.world_title("events",original_event_key(OriginalEventId(point.event))));
 else if(room==WorldRoom::Monster||room==WorldRoom::Elite||room==WorldRoom::Boss)if(const auto*d=encounter_definition(EncounterId(point.encounter)))model.text(a.content.world_title("encounters",d->key));
 values.model=model_title;journal_text(a,"MAP_POINT_HISTORY.room_stats","{MapPointType}{ModelTitle:: {}|}",values,room_line,sizeof room_line);
 const int room_lines=mn(3,text.paragraph(room_line,0,0,body_width,0,0,5));int at=25+room_lines*8;
 const char*partial_text=a.content.world_string("game_over_ui","PARTIAL_HISTORY");if(!*partial_text)partial_text="Earlier run data is unavailable.";
 const int partial_lines=partial?text.paragraph(partial_text,0,0,body_width,0,0,5):0;at+=partial_lines*8+(partial?3:0);
 struct Row {uint16_t indices[2];uint8_t cells,group,header,columns;int16_t at,lines[2];};Row rows[32]{};unsigned planned=0,visible=unsigned(motion.offset);int previous_group=-1;
 // Only the visible page is formatted. Off-screen rows require bounded
 // integer scans and no localized-string or glyph-pixel reads.
 for(unsigned i=unsigned(motion.offset);i<row_count&&planned<32;++i){
  const unsigned group=i<splits[0]?0:i<splits[0]+splits[1]?1:2;
  const unsigned local=i-(group==2?splits[0]+splits[1]:group==1?splits[0]:0),base=group==2?groups[0]+groups[1]:group==1?groups[0]:0;
  const bool columns=group&&groups[group]>5,paired=columns&&local+splits[group]<groups[group],header=group&&previous_group!=int(group);
  const unsigned cells=paired?2:1;const int cell_width=columns?(body_width-4)/2:body_width;int lines[2]{},height=0;uint16_t indices[2]{order[base+local],0};
  if(paired)indices[1]=order[base+local+splits[group]];
  for(unsigned col=0;col<cells;++col){char line[768]{};const auto e=journal_display_entry(journal,indices[col],point);a.history_journal_line(e,line,sizeof line);
   lines[col]=text.paragraph(line,0,0,cell_width-(journal_icon(e)>=0?7:0),0,0,5);height=mx(height,lines[col]*8+2);}
  if(at+height+(header?10:0)>max_height-15&&planned)break;
  if(header)at+=10;rows[planned++]={{indices[0],indices[1]},uint8_t(cells),uint8_t(group),uint8_t(header),uint8_t(columns),int16_t(at),{int16_t(lines[0]),int16_t(lines[1])}};
  at+=height;visible=i+1;previous_group=int(group);if(at>=max_height-15)break;
 }
 const bool paging=motion.offset||visible<row_count;const int height=mn(max_height,mx(48,at+(paging?17:7)));
 const int x=mn(318-width,mx(2,node_x<160?node_x+4:node_x-width-4)),y=mn(238-height,mx(31,node_y-height/2));
 motion.x=int16_t(x);motion.y=int16_t(y);motion.w=width;motion.h=int16_t(height);
 const int skin=a.content.world_image("common","HOVER_TIP");
 if(skin>=0){Canvas shadow=c;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;shadow.nine_image(skin,x+2,y+2,width,height,28,22,46,16,64);c.nine_image(skin,x,y,width,height,28,22,46,16);}
 else c.nine_ui(U_HoverTip,x,y,width,height,7,7,7,7);
 bold.text(title,x+7,y+5,5,Gold,body_width);text.text(player,x+7,y+15,5,White,body_width);
 text.paragraph(room_line,x+7,y+25,body_width,room_lines,0,5);
 if(partial)text.paragraph(partial_text,x+7,y+25+room_lines*8,body_width,partial_lines,0,5,Muted);
 for(unsigned row=0;row<planned;++row){const auto&plan=rows[row];const int top=y+plan.at,cell_width=plan.columns?(body_width-4)/2:body_width;
  if(plan.header){const char*label=a.content.world_string("run_history",plan.group==1?"HISTORY_ENTRY.rewardsHeader":"HISTORY_ENTRY.skippedHeader");bold.text(*label?label:plan.group==1?"Rewards:":"Skipped:",x+7,top-10,5,Gold,body_width);}
  for(unsigned col=0;col<plan.cells;++col){const auto e=journal_display_entry(journal,plan.indices[col],point);char line[768]{};a.history_journal_line(e,line,sizeof line);const int left=x+7+int(col)*(cell_width+4),icon=journal_icon(e),indent=icon>=0?7:0;
   if(icon>=0&&a.content.version()>=HistoryJournalContentVersion)c.ui(icon,left,top,5,5);
   text.paragraph(line,left+indent,top,cell_width-indent,mn(int(plan.lines[col]),(y+height-(paging?15:5)-top)/8),0,5,White);}
 }
 if(paging){char position[48]{};TextBuffer label{position,sizeof position,0};label.number(unsigned(motion.offset)+1);label.text("–");label.number(visible);label.ch('/');label.number(row_count);text.text(position,x+7,y+height-11,5,Muted,body_width);}
 c.hit(x,y,width,height,HitKind::HistoryJournalScroll,int(floor));
}
