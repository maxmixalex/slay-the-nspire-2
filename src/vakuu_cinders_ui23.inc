namespace vakuu_cinders {
using namespace vakuu_cinders_ui23;
static bool draw(Canvas&c,uint32_t elapsed,uint32_t seed){
 const int ids[]={c.a.content.ui_image(Auxiliary),int(Curves),int(Sprite)};
 const auto own=c.a.content.image(ids[0]),curves=c.a.content.image(ids[1]),sprite=c.a.content.image(ids[2]);
 for(int id:ids)if(!c.a.content.image_cached(id))return false;
 Table table;const water_reflection::Texture texture(sprite,24,24);
 if(!table.geometry.read(own)||!table.source.read(curves)||!texture.valid())return false;
 // The same already-sized Character scene union owns this room's first24
 // records. bind() invalidates Tez/Vakuu transitions, language, seed andentry.
 // Remaining1752records are neither touched nor advanced in this mode.
 auto&state=tez_cinder_room(c.a).state;
 const uint32_t clock=c.a.content.reduced_animations()?0u:elapsed;
 state.advance(clock,seed,table.source_curve(0),[&](){c.poll();},tez_cinders_ui23::NoInheritance{},true);
 for(unsigned i=0;i<24;++i)tez_cinders::particle(c,texture,state.particles[i],table,0,0);
 return true;
}
}
