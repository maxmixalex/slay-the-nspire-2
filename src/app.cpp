#include "app.hpp"
#include "ui12_assets.hpp"
#include "enemy_command_timing.hpp"
#include "rich_text.hpp"
#include "source_locale_fonts.hpp"
#include "settings_layout.hpp"
#include "run_setup_layout.hpp"
#include "status.hpp"
#include "combat_layout.hpp"
#include "hand_card_highlight.hpp"
#include "power_strip_layout.hpp"
#include "event_assets.hpp"
#include "world_run.hpp"
#include "original_events.hpp"
#include "localized_template.hpp"
#include "ui_core_observer.hpp"
#include "world_room_assets.hpp"
#include "world_rest_actors.hpp"
#include "presentation_blend.hpp"
#include "world_card_factory.hpp"
#include "nine_patch.hpp"
#include "loading_screen.hpp"
#include "card_hud_assets.hpp"
#include "feedback_assets.hpp"
#include "frontend_ui7_assets.hpp"
#include "character_select_layout.hpp"
#include "card_face_layout.hpp"
#include "menu_overlay_layout.hpp"
#include "card_resource_assets.hpp"
#include "card_keyword_text.hpp"
#include "fitted_text.hpp"
#include "inplace_reset.hpp"
#include "map_drawing_raster.hpp"
#include "run_score.hpp"
#include "game_over_ui.hpp"
#if SPIRE_DEBUG_COMMANDS
#include "debug_commands.hpp"
#endif
namespace spire {
#include "world_enemy_assets.inc"
#include "character_ui.inc"
#include "combat_feedback_ui.inc"
#include "canvas.inc"
#include "small_material_ui7.inc"
#include "room_secondary_vfx_ui11.inc"
static void world_region_image_key(const State&s,char*out,size_t size){
 // Run models retain names such as Overgrowth; both original-image families
 // register uppercase keys. Normalize only at the presentation boundary.
 if(size)out[0]=0;TextBuffer b{out,size,0};
 for(const char*p=world_region_key(s);*p;++p)b.ch(*p>='a'&&*p<='z'?char(*p-'a'+'A'):*p);
}
#include "native_controls.inc"
#include "world_text.inc"
#include "source_hover_ui.inc"
#include "ui_text.inc"
#include "ui_history_journal_ui.inc"
#include "world_enemy_ui.inc"
#include "turn_banner_ui.inc"
#include "combat_preparation.inc"
#include "world_map_motion.inc"
static bool ui_ancient_event(OriginalEventId);
static void crystal_screen(Canvas&);
#include "ancient_banner_ui.inc"
#include "app_input.inc"
#if SPIRE_DEBUG_COMMANDS
#include "debug_ui.inc"
#endif
#include "reward_presentation.inc"
static const char* card_type_label(CardType type,const Content*content=nullptr){
    const char*keys[]={"CARD_TYPE.ATTACK","CARD_TYPE.SKILL","CARD_TYPE.POWER","CARD_TYPE.STATUS","CARD_TYPE.CURSE","CARD_TYPE.QUEST"};
    if(content&&unsigned(type)<6){const char*text=content->world_string("gameplay_ui",keys[unsigned(type)]);if(*text)return text;}
    switch(type){case CardType::Attack:return "攻击";case CardType::Skill:return "技能";
    case CardType::Power:return "能力";case CardType::Status:return "状态";case CardType::Curse:return "诅咒";case CardType::Quest:return "任务";}
    return "";
}
#include "card_face_ui.inc"
static void small_card(Canvas&c,Card card,int x,int y,bool selected,bool playable,int number,int fee=-2,int stars=-2,int instance=-1,int deck=-1,const EventCardFacts*facts=nullptr){
    if(selected){c.rect(x-2,y-2,51,70,rgb(91,74,47));c.box(x-2,y-2,51,70,Gold);}
    draw_card_face(c,card,x,y,47,66,fee,stars,instance,deck,facts);
    if(number)c.num(number,x+36,y+53,8,Muted);
    if(!playable)c.dim(x,y,47,66,125);
}
static void log_line(App&a,Log l,char*b,size_t n){
    b[0]=0;TextBuffer t{b,n,0,&a.content};switch(l.kind){
    case LogKind::Turn:t.text("回合 ");t.number(l.a);break;
    case LogKind::Play:t.text("打出 ");t.text(a.content.card_string(l.a));if(l.b)t.ch('+');break;
    case LogKind::Damage:t.text("对敌人 ");t.number(l.b+1);t.text(" 造成 ");t.number(l.a);t.text(" 点伤害（含格挡）");break;
    case LogKind::Block:t.text("获得格挡 ");t.number(l.a);break;
    case LogKind::Exhaust:t.text("消耗 ");t.text(a.content.card_string(l.a));break;
    case LogKind::Draw:t.text("抽牌 ");t.number(l.a);break;
    case LogKind::Shuffle:t.text("弃牌堆洗回抽牌堆");break;
    case LogKind::Enemy:t.text("敌人 ");t.number(l.a+1);t.text(l.b==0?"：撞击":l.b==1?"：切击 + 格挡":"：嘶鸣 + 力量");break;
    case LogKind::Strength:t.text("获得力量 ");t.number(l.a);break;
    case LogKind::Pain:t.text("无惧疼痛 ");t.number(l.a);break;
    case LogKind::Embrace:t.text("黑暗之拥 ");t.number(l.a);break;
    case LogKind::Heal:t.text("恢复生命 ");t.number(l.a);break;
    case LogKind::Reward:case LogKind::Buy:t.text(l.kind==LogKind::Buy?"购买 ":"获得 ");t.text(a.content.card_string(l.a));break;
    case LogKind::Upgrade:t.text("升级 ");t.text(a.content.card_string(l.a));break;
    case LogKind::Gold:t.text("获得金币 ");t.number(l.a);break;
    case LogKind::Remove:t.text("移除 ");t.text(a.content.card_string(l.a));break;
    case LogKind::Transform:t.text("变化 ");t.text(a.content.card_string(l.a));t.text(" → ");t.text(a.content.card_string(l.b));break;
    case LogKind::Event:t.text("事件选择已结算");break;
    case LogKind::Relic:t.text("获得遗物：");if(auto*d=relic_definition((uint32_t)l.a))t.text(d->name);break;
    case LogKind::RelicIndex:t.text("遗物：");if(l.a>=0&&l.a<WorldRelicKinds)t.text(a.relic_title(RelicId(l.a)));break;
    case LogKind::Potion:t.text("使用药水");break;
    case LogKind::LoseHp:t.text("失去生命 ");t.number(l.a);break;
    case LogKind::Energy:t.text("获得能量 ");t.number(l.a);break;
    case LogKind::NoDraw:t.text("本回合不能抽牌");break;
    case LogKind::Weak:t.text("给予虚弱 ");t.number(l.a);break;
    case LogKind::PoisonDamage:t.text("中毒伤害 ");t.number(l.a);break;
    case LogKind::Poison:t.text("给予中毒 ");t.number(l.a);break;
    case LogKind::Discard:t.text("丢弃 ");t.text(a.content.card_string(l.a));break;
    case LogKind::Create:t.text("生成 ");t.text(a.content.card_string(l.b));t.text(" × ");t.number(l.a);break;
    case LogKind::Dexterity:t.text("获得敏捷 ");t.number(l.a);break;
    case LogKind::Noxious:t.text("毒雾 ");t.number(l.a);break;
    case LogKind::Accuracy:t.text("精准 ");t.number(l.a);break;
    default:t.text("容量上限：本局已安全停止");break;}
}
#include "relic_ui.inc"
#include "ui_picture_transform.inc"
#include "card_grid_ui.inc"
#include "deck_selection_ui.inc"
#include "hud_ui6.inc"

#include "rooms_ui.inc"
#include "status_ui.inc"
#include "world_intent_ui.inc"
#include "combat_ui.inc"
static void large_card(Canvas&c,Card card,int x,int y,bool selected,int fee=-2,int stars=-2,int instance=-1,int deck_index=-1,const EventCardFacts*facts=nullptr){
    if(selected){c.box(x-2,y-2,84,116,Gold);c.box(x-1,y-1,82,114,Gold);}
    draw_card_face(c,card,x,y,80,112,fee,stars,instance,deck_index,facts);
}
#include "world_reward_ui.inc"
#include "reward_presentation_ui.inc"
static void reward_screen(Canvas&c){
    if(c.a.game.world.active){native_reward_screen(c);return;}
    auto&a=c.a;auto&s=a.game;c.ui(U_BattleBackground,0,22);c.actor(0,54,130);c.dim(0,22,320,218,155);
    if(a.grimoire_select&&s.necro.grimoire_pending){deck_select(c);return;}
    if(a.reward_cards){native_card_reward_body(c,nullptr,3,true);return;}
    const bool grimoire=s.necro.grimoire_pending!=0;
    c.ui(U_RewardPanel,50,34,220,192);c.ui(U_RewardBanner,38,32,244,54);c.centered("战利品",60,47,200,20,Gold);
    for(int i=0;i<3;i++){
        int y=(grimoire?82:98)+i*(grimoire?30:33);bool claimed=s.reward_claimed&(1<<i);
        bool available=i!=2||s.reward_potion!=Potion::Empty;
        c.ui(U_RewardButton,70,y,180,29,(claimed||!available)?110:255);
        if(a.cursor==i&&!claimed&&available)c.box(68,y-1,184,31,Gold);
        if(i==0)c.ui(U_RewardGold,76,y+3,23,23);
        if(i==1)c.ui(U_RewardCard,76,y+3,23,23);
        if(i==2&&available)c.picture(a.content.potion_image(s.reward_potion),78,y+2,20,25);
        char text[96]={};TextBuffer t{text,sizeof text,0,&c.a.content};
        if(claimed)t.text("已领取");
        else if(i==0){t.number(s.last_gold);t.text(" 金币");}
        else if(i==1)t.text("选择一张牌");
        else if(available)t.text(potion_definition(s.reward_potion)->name);
        else t.text("没有药水掉落");
        c.text(text,107,y+6,12,claimed?Muted:White,137);
        const UiAction actions[]={UiAction::RewardGold,UiAction::RewardCards,UiAction::RewardPotion};
        c.action(68,y-1,184,31,actions[i],!claimed&&available);
    }
    if(grimoire){
        c.ui(U_RewardButton,70,172,180,29);if(a.cursor==3)c.box(68,171,184,31,Gold);
        c.ui(U_NecroPowerStart+(int)NPower::ForbiddenGrimoire,76,175,23,23);
        char label[80]={};TextBuffer t{label,sizeof label,0,&c.a.content};t.text("魔典移牌 × ");t.number(s.necro.grimoire_pending);
        c.text(label,107,178,12,White,137);c.action(68,171,184,31,UiAction::RewardGrimoire);
    }
    native_proceed(c,"前进",true,a.cursor==(grimoire?4:3));native_proceed_action(c,UiAction::LeaveRewards);
}

#include "world_camp_ui.inc"
static void camp_screen(Canvas&c){
    if(c.a.game.world.active){native_camp_screen(c);return;}
    auto&a=c.a;c.picture(A_CampBackground,0,22,320,218);c.clip(character_camp_animation(a.game.character),188,61,a.animation*33,false);
    if(a.game.character==Character::Necrobinder)c.clip(AnimOstyCamp,247,100,a.animation*33,false);
    c.picture(a.content.ui_image(U_CampFlames)+(int)((a.animation*33/125)%160),130,76,46,58);
    c.picture(A_CampFire,128,111,55,34,(unsigned)(225+(a.animation%16<8?a.animation%8:15-a.animation%16)*4));
    c.centered("休息处",0,43,320,20,Gold);
    const int xs[2]={54,189};
    for(int i=0;i<2;i++){
        bool enabled=!i||has_upgrade(a.game);int x=xs[i];
        if(a.cursor==i)c.ui(U_RestOutline,x-4,137,88,62);
        c.ui(i?U_RestSmith:U_RestHeal,x,140,80,57,enabled?255:100);
        c.centered(i?"锻造":"休息",x-4,193,88,16,a.cursor==i?Gold:White);
        c.hit(x-9,136,98,80,HitKind::Choice,i,enabled);
    }
    c.dim(0,219,320,21,200);char info[128]={};TextBuffer t{info,sizeof info,0,&c.a.content};
    if(a.cursor==0){t.text("回复 ");t.number(rest_heal_amount(a.game));t.text((a.game.relics&RegalPillow)?" 点生命（包含皇家枕头）":" 点生命（最大生命的 30%）");}
    else t.text("升级牌组中的一张牌。");
    c.centered(info,0,221,320,10,White);
}

static void choice_room_underlay(Canvas&c){
    auto&a=c.a;const auto&s=a.game;
    // NOverlayStack retains its room. Painting it must not clamp the modal's
    // independent cursor/scroll or leave clickable controls behind it.
    const int cursor=a.cursor,scroll=a.scroll,hits=a.hit_count;const bool mouse=a.pointer.visible;
    a.pointer.visible=false;
    const Phase parent=s.phase==Phase::WorldRelicChoice?Phase(s.world_relics.return_phase):s.phase;
    if(world_room_is_combat(s)||parent==Phase::Reward||combat_phase(parent)){
        if(combat_phase(parent))combat_screen(c);else combat_reward_room(c);
    }else if(s.original_events.active)event_screen(c);
    else if(s.world.active){
        switch(WorldRoom(s.world.room_type)){
        case WorldRoom::Shop:shop_screen(c);break;
        case WorldRoom::Rest:camp_screen(c);break;
        case WorldRoom::Treasure:treasure_screen(c);break;
        case WorldRoom::Map:map_screen(c,true);break;
        default:c.picture(a.combat_background(),0,22,320,218);break;
        }
    }else if(parent==Phase::Shop)shop_screen(c);
    else if(parent==Phase::Camp||s.phase==Phase::Upgrade)camp_screen(c);
    else if(parent==Phase::Treasure)treasure_screen(c);
    else c.picture(a.combat_background(),0,22,320,218);
    a.cursor=cursor;a.scroll=scroll;a.hit_count=hits;a.pointer.visible=mouse;
}

#include "potion_ui.inc"
#include "pile_ui6.inc"
static void panels(Canvas&c){
    auto&a=c.a;auto&s=a.game;if(a.panel==4){potion_popup_screen(c);return;}if(a.panel==1){pile_screen(c);return;}a.hit_count=0;if(a.panel==3){map_screen(c,true);c.close_button();return;}
    c.rect(4,24,312,212,Panel);c.box(4,24,312,212,Gold);
    if(a.panel==2){
        c.centered("本局记录（最近 24 条）",5,27,310,12,Gold);a.scroll=mn(a.scroll,mx(0,s.log_count-10));int first=mx(0,s.log_count-10-a.scroll);
        for(int i=first;i<mn(s.log_count,first+10);i++){char b[160]={};log_line(a,s.logs[i],b,sizeof b);c.text(b,13,49+(i-first)*16,10,White,295);}
    }
    c.close_button();
    if(a.panel==2){c.text("↑",299,53,12,Gold);c.hit(294,50,23,24,HitKind::Scroll,-1);c.text("↓",299,157,12,Gold);c.hit(294,153,23,24,HitKind::Scroll,1);}
}
int App::detail_energy()const{
    if(detail_card<0)return -2;
    Card card=card_from_token(detail_card);
    const EventCardFacts*facts=detail_has_facts?&detail_facts:event_card_facts(game,card,-1,detail_deck_index);
    int energy=detail_instance>=0?card_preview_cost(game,card,detail_instance):detail_cost_override==-2?card_offer_cost(card,facts):detail_cost_override;
    if(detail_instance>=0&&choice_free_energy(game)&&cost(card)>=0){
        for(unsigned i=0;i<game.expansion.choices.count;i++)if(game.expansion.choices.ids[i]==detail_instance){energy=card_free_preview_cost(game,card,detail_instance);break;}
    }
    return energy;
}
int App::detail_stars()const{
    if(detail_card<0)return 0;
    Card card=card_from_token(detail_card);int stars=card_star_cost(game,card,library?-1:detail_instance);
    if(!library&&stars>=0&&choice_free_energy(game)){
        for(unsigned i=0;i<game.expansion.choices.count;i++){
            int token=game.expansion.choices.ids[i];
            if((detail_instance>=0&&token==detail_instance)||
               (detail_instance<0&&game.expansion.choice==XChoice::Offer&&token>=MaxCards&&token<MaxCards+3&&game.expansion.offers[token-MaxCards].kind==card.kind))return 0;
        }
    }
    return stars;
}
#include "card_inspect_ui.inc"
#include "reference_map_ui.inc"
#include "ui_frontend.inc"
#include "startup_logo_ui.inc"
#include "bestiary_ui7.inc"
#include "ui_progress_ui.inc"
#include "run_modes_ui.inc"
#include "card_choice_ui.inc"
#include "combat_peek_ui.inc"
#include "world_choice_ui.inc"
#include "character_death_ui.inc"
#include "world_game_over_ui.inc"
#include "deck_confirm_ui.inc"
#include "keyword_tooltip_ui.inc"
#include "auxiliary_ui.inc"
#include "ftue_ui.inc"
#include "pointer_ui6.inc"

void App::render(){
    if(startup_logo.active()){Canvas c{*this};startup_logo_screen(c);return;}
#if SPIRE_DEBUG_COMMANDS
    if(debug_open){
        // Retain the current scene under the console. Rendering it with mouse
        // focus suppressed cannot expose any background control to input.
        const bool mouse=pointer.visible;debug_open=false;pointer.visible=false;render();debug_open=true;pointer.visible=mouse;
        Canvas c{*this};debug_screen(c);pointer_overlay(c);return;
    }
#endif
    begin_card_face_frame();
    popup_backdrop.observe(frontend_confirm!=0,map_clock_ms);
    if(!paused)pause_visible=false;
    if(auxiliary.screen!=AuxiliaryScreen::None){Canvas c{*this};auxiliary_screen(c);pointer_overlay(c);return;}
    if(bestiary.open){Canvas c{*this};const bool mouse=pointer.visible;if(frontend_confirm)pointer.visible=false;bestiary_screen(c);pointer.visible=mouse;if(frontend_confirm)frontend_confirm_screen(c);pointer_overlay(c);return;}
    sync_reward_presentation();
    sync_combat_peek();
    sync_map_window();
    if(!character_select)character_select_presentation.observe(false,0,-1,map_clock_ms);
    sync_combat_preparation();
    if(combat_preparation.active){
        // Keep the original room transition black while its required poses
        // are prepared. A loading illustration/label is not part of the
        // original room scene and must not interrupt its fade-out/fade-in.
        hit_count=0;combat_preparation.shown=true;Canvas c{*this};c.rect(0,0,ScreenW,ScreenH,0);
        if(paused)native_pause_screen(c);if(frontend_confirm)frontend_confirm_screen(c);pointer_overlay(c);return;
    }
    map_visuals.update(game,map_clock_ms);
    // A capstone overlay hides the map without closing it. Its marker
    // lifecycle must not replay when the deck or compendium is dismissed.
    map_visuals.set_open(game,game.world.active&&!title&&!character_select&&!map_preview.open&&
        (game.phase==Phase::Map||map_window.is_open||panel==3),map_clock_ms);
    sync_game_over_presentation();Canvas c{*this};sync_combat_feedback();sync_relic_feedback();sync_character_feedback();sync_enemy_feedback();sync_turn_banner();sync_health_bars();sync_map_reticles();sync_ftues();hit_count=0;c.rect(0,0,320,240,Ink);
    // Opaque screens do not redraw the hidden title/combat scene or its clips.
    if(profiles_open){const bool mouse=pointer.visible;if(frontend_confirm)pointer.visible=false;profiles_screen(c);pointer.visible=mouse;if(frontend_confirm)frontend_confirm_screen(c);pointer_overlay(c);return;}
    if(run_mode.screen!=RunMenu::Closed){const bool mouse=pointer.visible;if(frontend_confirm||ftue.active())pointer.visible=false;run_modes_screen(c);pointer.visible=mouse;if(frontend_confirm)frontend_confirm_screen(c);ftue_screen(c);character_departure_screen(c);pointer_overlay(c);return;}
    if(map_preview.open){reference_map_screen(c);pointer_overlay(c);return;}
    if(library){const bool mouse=pointer.visible;if(frontend_confirm)pointer.visible=false;library_screen(c);if(detail_card>=0||card_inspect.closing(map_clock_ms))detail_screen(c);if(detail_item>=0||relic_inspect.closing(map_clock_ms))item_detail_screen(c);keyword_tooltips(c);pointer.visible=mouse;if(frontend_confirm)frontend_confirm_screen(c);pointer_overlay(c);return;}
    if(statistics_open||history_open){const bool mouse=pointer.visible;if(frontend_confirm)pointer.visible=false;if(statistics_open)statistics_screen(c);else history_screen(c);if(detail_card>=0||card_inspect.closing(map_clock_ms))detail_screen(c);if(detail_item>=0||relic_inspect.closing(map_clock_ms))item_detail_screen(c);keyword_tooltips(c);pointer.visible=mouse;if(frontend_confirm)frontend_confirm_screen(c);pointer_overlay(c);return;}
    if(compendium_open){const bool mouse=pointer.visible;if(frontend_confirm)pointer.visible=false;compendium_screen(c);pointer.visible=mouse;if(frontend_confirm)frontend_confirm_screen(c);pointer_overlay(c);return;}
    if(character_select){const bool mouse=pointer.visible;if(ftue.active())pointer.visible=false;character_screen(c);pointer.visible=mouse;ftue_screen(c);character_departure_screen(c);pointer_overlay(c);return;}
    const bool underlay_pointer_visible=pointer.visible;
    if(settings_open||frontend_confirm||help||ftue.active())pointer.visible=false;
    if(title){title_screen(c);
    }else{
        // The room beneath a modal can clamp its own text scroll. It must not
        // replace the active potion/deck/log/relic panel's independent cursor.
        const int panel_scroll=scroll;
        const int map_cursor=cursor;
        if(map_window.is_open){cursor=map_room_cursor;scroll=map_room_scroll;}
        const bool combat_linger=int32_t(combat_linger_until-animation)>0;
        if(combat_linger||(combat_feedback.active&&choice_phase(game.phase))||combat_peek_context())combat_screen(c);else switch(game.phase){
        case Phase::Map:if(!game.world.active||!map_window.visible)map_screen(c);break;
        case Phase::Shop:shop_screen(c);break;
        case Phase::Event:event_screen(c);break;
        case Phase::Camp:camp_screen(c);break;
        case Phase::Upgrade:case Phase::Remove:case Phase::Transform:sync_deck_selection();choice_room_underlay(c);deck_select(c);break;
        case Phase::Reward:reward_screen(c);break;
        case Phase::Combat:case Phase::ExhaustChoice:case Phase::DiscardChoice:case Phase::ExhaustManyChoice:case Phase::ReturnChoice:case Phase::ExtendedChoice:combat_screen(c);break;
        case Phase::Treasure:treasure_screen(c);break;
        case Phase::WorldRelicChoice:case Phase::OriginalEventChoice:choice_room_underlay(c);break;
        default:if(game.world.active&&(game.phase==Phase::Complete||game.phase==Phase::Dead)){native_game_over_screen(c);break;}c.picture(A_Title,0,0);c.dim(0,20,320,220,120);c.centered(game.phase==Phase::Complete?"远征完成":game.phase==Phase::Dead?"倒在了高塔下":"达到安全容量上限",0,78,320,20,Gold);c.centered("Enter 返回标题",0,150,320,12);c.hit(0,130,320,65,HitKind::Command,(int)Key::Confirm);if(!game.world.active)c.centered("原型路线结束，并非完整原版通关。",0,210,320,10,Muted);break;
        }
        // The underlying live room may clamp its own navigation while it is
        // hidden. Never let that overwrite the selected map column or a
        // covering capstone's scroll position.
        if(map_window.is_open){map_room_cursor=cursor;map_room_scroll=scroll;cursor=map_cursor;scroll=panel_scroll;}
        if(map_window.visible)world_map_window_screen(c);
        if(!combat_linger&&(!panel||panel==4)&&!library&&combat_choice_ready()&&!choice_peeking&&(game.phase==Phase::WorldRelicChoice||game.phase==Phase::OriginalEventChoice))world_choice_screen(c);
        else if(!combat_linger&&!panel&&!library&&combat_choice_ready()&&choice_phase(game.phase)&&!choice_peeking)card_choice_screen(c);
        combat_peek_button(c);
        potion_popup_exit_screen(c);
        if(panel&&!(panel==3&&map_window.visible)){scroll=panel_scroll;panels(c);}hud(c);
        if(!combat_linger||combat_feedback.active)turn_banner_screen(c);
        reward_pickup_screen(c);
        if(paused)native_pause_screen(c);

    }
    pointer.visible=underlay_pointer_visible&&!help&&!frontend_confirm;
    if(settings_open)frontend_settings_screen(c);
    pointer.visible=underlay_pointer_visible&&!help;
    if(frontend_confirm)frontend_confirm_screen(c);
    pointer.visible=underlay_pointer_visible;
    if(help){
        hit_count=0;c.hit(6,7,308,226,HitKind::Close);c.rect(6,7,308,226,Panel);c.box(6,7,308,226,Gold);c.centered("操作说明",6,13,308,16,Gold);
        const InputCommand commands[]={InputCommand::Select,InputCommand::Confirm,InputCommand::EndTurn,InputCommand::Map,InputCommand::Deck,
            InputCommand::Draw,InputCommand::Discard,InputCommand::Exhaust,InputCommand::Peek,InputCommand::Cancel};
        const unsigned bank=keyboard_only?1:0;
        for(unsigned i=0;i<10;++i){const unsigned command=unsigned(commands[i]);char label[16]{};input_key_label(input_bindings.keys[bank][command],label,sizeof label,calculator_keyboard);
            c.text_fitted(input_command_title(command),16,41+int(i)*17,204,11,White,8);c.text_fitted(label,225,41+int(i)*17,77,11,Gold,8,true);}
        c.centered(content.language()!=2?"Change bindings in Settings > Input":"在设置 > 输入中更改按键",0,216,320,10,Gold);
    }
    if(character_select)character_screen(c);
    if(library)library_screen(c);
    if(deck_candidate>=0&&!paused&&!help&&!panel)deck_confirm_screen(c);
    if(detail_card>=0||card_inspect.closing(map_clock_ms))detail_screen(c);
    if(detail_item>=0||relic_inspect.closing(map_clock_ms))item_detail_screen(c);
    if(status_actor>=0)status_screen(c);
    if(event_reader)event_reader_screen(c);
    if(map_preview.open)reference_map_screen(c);
    if(notice_ticks&&detail_item<0&&detail_card<0&&!panel&&game.phase!=Phase::Treasure&&game.phase!=Phase::Event&&game.phase!=Phase::Shop&&status_actor<0&&!map_preview.open&&!library&&!character_select&&deck_candidate<0&&!choice_phase(game.phase)){c.rect(2,219,316,20,Panel);c.box(2,219,316,20,Gold);c.centered(notice,3,221,314,10,Gold);}
    if(!ftue.active()){
    hud_hover_tip(c);
    potion_hover_tip(c);
    
    orb_hover_tip(c);
    keyword_tooltips(c);
    }
    ftue_screen(c);
    character_departure_screen(c);
    pointer_overlay(c);
}
} // namespace spire
