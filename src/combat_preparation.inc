void App::sync_combat_preparation(){
    if(title||character_select||!combat_phase(game.phase)||!content.current_profile())return;
    auto&p=combat_preparation;
    const int encounter=game.enemy_world.active?int(game.enemy_world.encounter):-1;
    if(p.initialized&&p.seed==game.seed&&p.floor==game.floor&&p.combat==game.world.combat_count&&p.encounter==encounter)return;
    p.reset();p.initialized=true;p.seed=game.seed;p.floor=game.floor;p.combat=game.world.combat_count;p.encounter=encounter;
    sync_enemy_feedback();
    struct Track {Clip clip;unsigned start;};Track tracks[CombatPreparation::FirstCapacity]{};unsigned count=0;
    auto track=[&](int id,uint32_t elapsed=0){
        const auto clip=content.clip(id);if(!clip.count||!clip.frame_ms)return;
        for(unsigned i=0;i<count;++i)if(tracks[i].clip.first==clip.first&&tracks[i].clip.count==clip.count)return;
        if(count<CombatPreparation::FirstCapacity)tracks[count++]={clip,(elapsed/clip.frame_ms)%clip.count};
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
    auto add=[&](int id){
        if(id<0)return true;
        for(unsigned i=0;i<p.count;++i)if(content.image_alias(int(p.images[i]),id))return true;
        const unsigned pages=content.image_cache_pages(id);
        if(p.count>=CombatPreparation::Capacity||pages>CombatPreparation::PageBudget-p.pages){p.bounded=true;return false;}
        p.images[p.count++]=uint32_t(id);p.pages+=pages;return true;
    };
    add(combat_background());
    // Reserve the first frame of every actor before extending any loop.
    for(unsigned i=0;i<count;++i){const unsigned id=tracks[i].clip.first+tracks[i].start;
        if(add(int(id))&&p.first_count<CombatPreparation::FirstCapacity)p.first[p.first_count++]=id;
    }
    // Extend all actors' windows together, then read in stored image order.
    // Large/boss clips stop at the shared budget, never preloading every move.
    for(unsigned frame=1;frame<4096;++frame){bool more=false,full=false;
        for(unsigned i=0;i<count;++i){const auto&t=tracks[i];if(frame>=t.clip.count)continue;more=true;
            if(!add(int(t.clip.first+(t.start+frame)%t.clip.count))){full=true;break;}}
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
