// The calculator keeps original credits; offline patch notes and feedback
// have no useful action and are deliberately unavailable.
void App::open_auxiliary(AuxiliaryScreen screen){
 if(screen==AuxiliaryScreen::None){close_auxiliary();return;}
 if(screen!=AuxiliaryScreen::Credits||frontend_confirm||binding_listening>=0)return;
 auto&v=auxiliary;if(v.screen==screen){close_auxiliary();return;}
 v.screen=screen;v.opened=map_clock_ms;v.closing=false;v.scroll.reset();
 v.credits_target=v.credits_position=0;v.credits_remainder=0;
 pointer_cancel();physical_keys_cancel();keyword_hover={};
}
void App::close_auxiliary(){
 auto&v=auxiliary;if(v.screen==AuxiliaryScreen::None||v.closing||map_clock_ms-v.opened<2000u)return;
 pointer_cancel();v.closed=map_clock_ms;v.closing=true;v.closing_at=map_clock_ms;
}
void App::advance_auxiliary(unsigned ms){
 auto&v=auxiliary;if(v.screen!=AuxiliaryScreen::Credits)return;
 if(v.closing&&map_clock_ms-v.closing_at>=1000u){v.screen=AuxiliaryScreen::None;v.closing=false;return;}
 // Keep the source scroll speed, with fractional LCD pixels accumulated.
 v.credits_remainder+=ms*80u*4u*256u;v.credits_target+=int(v.credits_remainder/21000u);v.credits_remainder%=21000u;
 const int delta=v.credits_target-v.credits_position,weight=mn(1000,int(ms)*20);
 v.credits_position+=int(int64_t(delta)*weight/1000);
}
void App::auxiliary_scroll(int rows){
 auto&v=auxiliary;if(v.closing||v.screen!=AuxiliaryScreen::Credits||!rows)return;
 v.credits_target=mx(0,v.credits_target+rows*16*256);
}
unsigned App::feedback_report(char*,unsigned)const{return 0;}
bool App::auxiliary_text(int){return false;}
void App::auxiliary_control(int control){
 if(control==AuxiliaryOpenCredits){open_auxiliary(AuxiliaryScreen::Credits);return;}
 if(auxiliary.screen!=AuxiliaryScreen::Credits||auxiliary.closing)return;
 if(control==AuxiliaryBack)close_auxiliary();
 else if(control==AuxiliaryScrollUp||control==AuxiliaryScrollDown)auxiliary_scroll(control==AuxiliaryScrollUp?-3:3);
}
bool App::auxiliary_input(Key key){
 if(auxiliary.screen!=AuxiliaryScreen::Credits)return false;
 if(auxiliary.closing)return true;
 if(key==Key::Back||key==Key::Confirm)close_auxiliary();
 else if(key==Key::Up||key==Key::Down)auxiliary_scroll(key==Key::Up?-3:3);
 return true;
}
