static void combat_peek_button(Canvas&c){
 auto&a=c.a;if(!a.combat_peek_available())return;
 constexpr int x=5,y=108,size=24;
 const bool hover=a.pointer.visible&&a.pointer.x>=x&&a.pointer.x<x+size&&a.pointer.y>=y&&a.pointer.y<y+size;
 if(a.choice_peeking){Canvas glow=c;const unsigned pulse=unsigned(160+CombatCardMotion::sine(int(a.map_clock_ms%1800u)*4096/1800)*80/1024);
  glow.picture(a.content.ui_image(I8_PeekFlash),x-4,y-4,31,31,pulse,presentation::Add);}
 if(a.content.ui_image(I8_Peek)>=0)c.ui(I8_Peek,x,y,size,size,hover?255:225);
 else{c.rect(x,y,size,size,Panel);c.box(x,y,size,size,a.choice_peeking||hover?Gold:Muted);c.centered("…",x,y+6,size,10,White);}
 c.hit(x,y,size,size,HitKind::CombatPeek);
}
