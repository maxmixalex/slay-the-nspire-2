static void combat_peek_button(Canvas&c){
 auto&a=c.a;if(!a.combat_peek_available())return;
 const bool hover=a.pointer.visible&&a.pointer.x>=19&&a.pointer.x<43&&a.pointer.y>=108&&a.pointer.y<132;
 if(a.choice_peeking){Canvas glow=c;const unsigned pulse=unsigned(160+CombatCardMotion::sine(int(a.map_clock_ms%1800u)*4096/1800)*80/1024);
  glow.picture(a.content.ui_image(I8_PeekFlash),15,104,31,31,pulse,presentation::Add);}
 if(a.content.ui_image(I8_Peek)>=0)c.ui(I8_Peek,19,108,24,24,hover?255:225);
 else{c.rect(19,108,24,24,Panel);c.box(19,108,24,24,a.choice_peeking||hover?Gold:Muted);c.centered("…",19,114,24,10,White);}
 c.hit(19,108,24,24,HitKind::CombatPeek);
}
