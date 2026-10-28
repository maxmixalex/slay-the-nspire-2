static void bestiary_kaiser_supplement(Canvas&c,unsigned source,unsigned frame){
 if(c.a.content.version()<BestiaryKaiserContentVersion)return;
 const int directory=bestiary_kaiser::directory(source);if(directory<0)return;
 const auto patch=c.a.content.clip(directory),original=c.a.content.clip(int(source));
 // The descriptor identifies the precise world frame this supplement
 // completes. Never reinterpret its reserved source ID as sprite scale.
 if(patch.sprite_units!=source+1||patch.count!=original.count||patch.frame_ms!=original.frame_ms||frame>=patch.count)return;
 c.picture(int(patch.first)+int(frame),-patch.pivot_x,-patch.pivot_y);
}
