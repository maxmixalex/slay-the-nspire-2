namespace neow_post {
static bool draw(Canvas&c,uint32_t clock){
 using namespace neow_post_ui23;
 if(c.clip_w!=320||c.clip_h!=240||c.pitch<320||c.coverage||c.opacity!=255||c.brightness!=255||
    c.tint_red!=255||c.tint_green!=255||c.tint_blue!=255||c.color_matrix||c.occlusion)return false;
 const auto generation=c.a.content.generation();
 if(c.a.content.ui_image(AuxiliaryFirst)!=int(FirstImage))return false;
 Table table;if(!table.read(c.a.content.image(FirstImage),c.a.content.reduced_animations()))return false;
 if(!c.a.content.healthy()||c.a.content.generation()!=generation)return false;
 // Source command49: after body, fire/fog/stars, lights and vignette; before
 // dialogue/HUD and the reduced scene cache publication. One view, no further
 // content operation while the existing transfer lease owns the snapshot.
 return apply(c.pixels,c.pitch,table,clock);
}
}
