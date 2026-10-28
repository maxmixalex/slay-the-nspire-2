void App::sync_combat_preparation(){
    // A saved relic choice still belongs to its combat. Prepare that room
    // before displaying the choice, so confirming an enchantment cannot
    // start a new black preparation interval over its reveal animation.
    const Phase parent=game.phase==Phase::WorldRelicChoice?Phase(game.world_relics.return_phase):game.phase;
    if(title||character_select||!combat_phase(parent)||!content.current_profile())return;
    auto&p=combat_preparation;
    const int encounter=game.enemy_world.active?int(game.enemy_world.encounter):-1;
    if(p.initialized&&p.seed==game.seed&&p.floor==game.floor&&p.combat==game.world.combat_count&&p.encounter==encounter)return;
    p.reset();p.initialized=true;p.seed=game.seed;p.floor=game.floor;p.combat=game.world.combat_count;p.encounter=encounter;
    sync_enemy_feedback();
    struct Track {Clip clip;unsigned start;int id;};Track tracks[CombatPreparation::FirstCapacity]{};unsigned count=0;
    auto track=[&](int id,uint32_t elapsed=0){
        const auto clip=content.clip(id);if(!clip.count||!clip.frame_ms)return;
        for(unsigned i=0;i<count;++i)if(tracks[i].clip.first==clip.first&&tracks[i].clip.count==clip.count)return;
        if(count<CombatPreparation::FirstCapacity)tracks[count++]={clip,(elapsed/clip.frame_ms)%clip.count,id};
    };
    track(character_animation(game.character,AnimHeroIdle),animation*33);
    if(game.expansion.osty_hp>0)track(AnimOstyIdle,animation*33);
    for(int i=0;i<game.enemy_count;i++)if(game.enemies[i].hp>0){
        if(!game.enemy_world.active){track(AnimEnemyIdle,animation*33+unsigned(i+1)*347);continue;}
        const auto&m=enemy_presentation[i];track(m.clip,(animation-m.start)*33);
        const auto*node=world_enemy_state(m.model,m.state);
        for(unsigned depth=0;node&&!node->loop&&node->next>=0&&depth<8;++depth){
            node=world_enemy_state(m.model,node->next);if(!node)break;
            track(world_enemy_resolve_clip(m.model,node->clip,m.skin,-1,ui_enemy_fact,const_cast<EnemyVisualFacts*>(&m.facts)));
        }
        const int idle=world_enemy_resume(m.model,ui_enemy_fact,const_cast<EnemyVisualFacts*>(&m.facts));
        if(const auto*resume=world_enemy_state(m.model,idle))if(resume->loop)
            track(world_enemy_resolve_clip(m.model,resume->clip,m.skin,-1,ui_enemy_fact,const_cast<EnemyVisualFacts*>(&m.facts)));

    }
    // The already displayed intent names this move. Decode its registered
    // action tracks inside the existing transition, before it is played.
    // Actor poses keep priority: append these only after every current/idle
    // track, and retain the same track, image and page limits. Never advance
    // the move graph, resolve a random branch or simulate future power facts.
    if(game.enemy_world.active&&!ui_feedback_bound(*this))for(int i=0;i<game.enemy_count;i++)if(game.enemies[i].hp>0){
        const auto intent=enemy_intent(game,i);if(!intent.flags||(intent.flags&EnemyIntentUnknown))continue;
        const auto&m=enemy_presentation[i];const int selected=game.enemy_world.models[i].node;
        const auto*move=world_enemy_move(m.model,selected);if(!move)continue;
        auto facts=ui_enemy_facts(game,i);int projected=m.state;
        for(unsigned j=0;j<move->action_count;++j)if(const auto*action=world_enemy_action(*move,j)){
            if(action->direct)track(world_enemy_resolve_clip(m.model,action->name,m.skin,selected,ui_enemy_fact,&facts));
            else{
                const auto*before=world_enemy_state(m.model,projected);
                if(before&&!before->loop&&before->next>=0)projected=before->next;
                const int next=world_enemy_transition(m.model,projected,action->name,ui_enemy_fact,&facts);
                if(const auto*node=world_enemy_state(m.model,next)){
                    projected=next;track(world_enemy_resolve_clip(m.model,node->clip,m.skin,selected,ui_enemy_fact,&facts));
                }
            }
        }
    }
    auto add=[&](int id){
        if(id<0)return true;
        for(unsigned i=0;i<p.count;++i)if(content.image_alias(int(p.images[i]),id))return true;
        const unsigned pages=content.image_cache_pages(id);
        if(p.count>=CombatPreparation::Capacity||pages>CombatPreparation::PageBudget-p.pages){p.bounded=true;return false;}
        p.images[p.count++]=uint32_t(id);p.pages+=pages;return true;
    };
    auto reserve=[&](int id,bool first){
        if(!add(id))return false;
        if(first&&id>=0&&p.first_count<CombatPreparation::FirstCapacity){
            bool found=false;for(unsigned i=0;i<p.first_count;++i)found|=content.image_alias(int(p.first[i]),id);
            if(!found)p.first[p.first_count++]=unsigned(id);
        }
        return true;
    };
    auto frame_images=[&](const Track&t,unsigned frame,bool first){
        const int row=vantom_ui15_row(t.id);
        const int parameters=row<0?-1:content.ui_image(VantomUi15BaseAuxiliary+unsigned(row&127));
        if(parameters<0)return reserve(int(t.clip.first+frame),first);
        if(!reserve(parameters,first))return false;
        Canvas::VantomDraw layers[VantomUi15MaxLayers];unsigned layers_count=0;
        if(!Canvas{*this}.vantom_collect(t.id,0,0,frame*t.clip.frame_ms,1024,false,1024,
                                       t.clip.sprite_units,layers,layers_count,false))return false;
        for(unsigned i=0;i<layers_count;++i){if(!reserve(layers[i].image,first))return false;
            if(layers[i].mask>=0&&!reserve(layers[i].mask,first))return false;
            if(layers[i].geometry>=0&&!reserve(layers[i].geometry,first))return false;
            if(layers[i].continuation>=0&&!reserve(layers[i].continuation,first))return false;}
        return true;
    };
    add(combat_background());
    // Reserve the first frame of every actor before extending any loop.
    for(unsigned i=0;i<count;++i)frame_images(tracks[i],tracks[i].start,true);
    // Extend all actors' windows together, then read in stored image order.
    // Large/boss clips stop at the shared budget, never preloading every move.
    for(unsigned frame=1;frame<4096;++frame){bool more=false,full=false;
        for(unsigned i=0;i<count;++i){const auto&t=tracks[i];if(frame>=t.clip.count)continue;more=true;
            if(!frame_images(t,(t.start+frame)%t.clip.count,false)){full=true;break;}}
        if(!more||full)break;
    }
    for(unsigned i=1;i<p.count;++i){uint32_t id=p.images[i];unsigned j=i;while(j&&p.images[j-1]>id){p.images[j]=p.images[j-1];--j;}p.images[j]=id;}
    for(unsigned i=0;i<p.count;++i)if(!content.image_cached(int(p.images[i]))){p.active=true;break;}
    if(!p.active)p.cursor=p.count;
}
void App::step_combat_preparation(){
    auto&p=combat_preparation;if(!p.active||!p.shown||paused)return;
    ++p.updates;unsigned decoded_count=0;
    while(p.cursor<p.count){
        const int image=int(p.images[p.cursor]);
        if(content.image_cached(image)){++p.cursor;continue;}
        if(decoded_count>=CombatPreparation::ImagesPerUpdate)return;
        ++p.cursor;
        const auto before=content.read_stats();const auto decoded=content.image(image);const auto after=content.read_stats();
        p.read_calls+=after.callbacks-before.callbacks;p.read_bytes+=after.callback_bytes-before.callback_bytes;++p.cold_images;++decoded_count;
        if(!decoded.pixels){p.active=false;message("资源读取失败");return;}
        // Consecutive images commonly share a resident tile group. Process
        // a small bounded batch before yielding, instead of imposing one
        // complete 33 ms host interval on every cheap cache assembly.
    }
    // Touch already resident initial poses last. This cannot issue new I/O;
    // it protects them while the first visible frame loads its HUD images.
    for(unsigned i=0;i<p.first_count;++i)if(content.image_cached(int(p.first[i])))content.image(int(p.first[i]));
    p.active=false;
}
